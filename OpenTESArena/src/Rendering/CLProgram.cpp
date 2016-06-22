#include <cassert>
#include <cmath>

#include "SDL.h"

#include "CLProgram.h"

#include "../Entities/Directable.h"
#include "../Math/Constants.h"
#include "../Math/Float2.h"
#include "../Math/Float3.h"
#include "../Math/Float4.h"
#include "../Math/Triangle.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"

namespace
{
	// These sizes are intended to match those of the .cl file structs. OpenCL 
	// aligns structs to multiples of 8 bytes. Additional padding is sometimes 
	// necessary to match struct alignment.
	const cl::size_type SIZEOF_CAMERA = (sizeof(cl_float3) * 4) + sizeof(cl_float) + 12;
	const cl::size_type SIZEOF_LIGHT = (sizeof(cl_float3) * 2);
	const cl::size_type SIZEOF_LIGHT_REF = sizeof(cl_int) * 2;
	const cl::size_type SIZEOF_SPRITE_REF = sizeof(cl_int) * 2;
	const cl::size_type SIZEOF_TEXTURE_REF = sizeof(cl_int) + (sizeof(cl_short) * 2);
	const cl::size_type SIZEOF_TRIANGLE = (sizeof(cl_float3) * 4) +
		(sizeof(cl_float2) * 3) + SIZEOF_TEXTURE_REF;
	const cl::size_type SIZEOF_VOXEL_REF = sizeof(cl_int) * 2;
}

const std::string CLProgram::PATH = "data/kernels/";
const std::string CLProgram::FILENAME = "kernel.cl";
const std::string CLProgram::TEST_KERNEL = "test";
const std::string CLProgram::INTERSECT_KERNEL = "intersect";
const std::string CLProgram::AMBIENT_OCCLUSION_KERNEL = "ambientOcclusion";
const std::string CLProgram::RAY_TRACE_KERNEL = "rayTrace";
const std::string CLProgram::ANTI_ALIAS_KERNEL = "antiAlias";
const std::string CLProgram::POST_PROCESS_KERNEL = "postProcess";
const std::string CLProgram::CONVERT_TO_RGB_KERNEL = "convertToRGB";

CLProgram::CLProgram(int width, int height)
{
	assert(width > 0);
	assert(height > 0);

	this->width = width;
	this->height = height;

	// Get the OpenCL platforms (i.e., AMD, Intel, Nvidia) available on the machine.
	auto platforms = CLProgram::getPlatforms();
	Debug::check(platforms.size() > 0, "CLProgram", "No OpenCL platform found.");

	// Look at the first platform. Most computers shouldn't have more than one.
	// More robust code can check for multiple platforms in the future.
	const auto &platform = platforms.at(0);

	// Mention some version information about the platform (it should be okay if the 
	// platform version is higher than the device version).
	Debug::mention("CLProgram", "Platform version is \"" +
		platform.getInfo<CL_PLATFORM_VERSION>() + "\".");

	// Check for all possible devices on the platform, starting with GPUs.
	auto devices = CLProgram::getDevices(platform, CL_DEVICE_TYPE_GPU);
	if (devices.size() == 0)
	{
		Debug::mention("CLProgram", "No OpenCL GPU device found. Trying CPUs.");
		devices = CLProgram::getDevices(platforms.at(0), CL_DEVICE_TYPE_CPU);
		if (devices.size() == 0)
		{
			Debug::mention("CLProgram", "No OpenCL CPU device found. Trying accelerators.");
			devices = CLProgram::getDevices(platforms.at(0), CL_DEVICE_TYPE_ACCELERATOR);
			Debug::check(devices.size() > 0, "CLProgram", "No OpenCL devices found.");
		}
	}

	// Choose the first available device. Users with multiple GPUs might prefer an option.
	this->device = devices.at(0);

	// Create an OpenCL context.
	cl_int status = CL_SUCCESS;
	this->context = cl::Context(this->device, nullptr, nullptr, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Context.");

	// Create an OpenCL command queue.
	this->commandQueue = cl::CommandQueue(this->context, this->device, 0, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::CommandQueue.");

	// Read the kernel source from file.
	auto source = File::toString(CLProgram::PATH + CLProgram::FILENAME);

	// Make some #defines to add to the kernel source.
	auto defines = std::string("#define SCREEN_WIDTH ") + std::to_string(width) +
		std::string("\n") + std::string("#define SCREEN_HEIGHT ") +
		std::to_string(height) + std::string("\n") + std::string("#define ASPECT_RATIO ") +
		std::to_string(static_cast<double>(width) / static_cast<double>(height)) +
		std::string("f\n") + // The "f" is for "float". OpenCL complains if it's a double.
		// The world dimensions will be placeholders for now.
		std::string("#define WORLD_WIDTH ") + std::to_string(1) + std::string("\n") +
		std::string("#define WORLD_HEIGHT ") + std::to_string(1) + std::string("\n") +
		std::string("#define WORLD_DEPTH ") + std::to_string(1) + std::string("\n");

	// Add some kernel compilation switches.
	std::string options("-cl-fast-relaxed-math -cl-strict-aliasing");

	// Put the kernel source in a program object within the OpenCL context.
	this->program = cl::Program(this->context, defines + source, false, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Program.");

	// Build the program into something executable. If compilation fails, the program stops.
	status = this->program.build(devices, options.c_str());
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Program::build (" +
		this->getErrorString(status) + ").");

	// Create the kernels and set their entry function to be a __kernel in the program.
	this->kernel = cl::Kernel(this->program, CLProgram::TEST_KERNEL.c_str(), &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel.");

	// Create the OpenCL buffers for reading and writing.
	this->cameraBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_CAMERA, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer cameraBuffer.");

	this->voxelRefBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_VOXEL_REF /* Placeholder size */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer voxelRefBuffer.");

	this->spriteRefBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_SPRITE_REF /* Placeholder size */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer spriteRefBuffer.");

	this->lightRefBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_LIGHT_REF /* Placeholder size */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer lightRefBuffer.");

	this->textureRefBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_TEXTURE_REF /* Placeholder size */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer textureRefBuffer.");

	this->triangleBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_TRIANGLE /* Placeholder size */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer triangleBuffer.");

	this->textureBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		sizeof(cl_float4) * 3 /* Placeholder size */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer textureBuffer.");

	this->gameTimeBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		sizeof(cl_float), nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer gameTimeBuffer.");

	this->outputBuffer = cl::Buffer(this->context, CL_MEM_WRITE_ONLY,
		sizeof(cl_int) * width * height, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer outputBuffer.");

	// Tell the kernel arguments where their data lives.
	// Maybe there should be methods which set kernel arguments for each kernel object.
	// If only one kernel member is used, then update the targets for it on-the-fly.
	status = this->kernel.setArg(0, this->cameraBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg cameraBuffer.");

	status = this->kernel.setArg(1, this->voxelRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg voxelRefBuffer.");

	status = this->kernel.setArg(2, this->spriteRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg spriteRefBuffer.");

	status = this->kernel.setArg(3, this->lightRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg lightRefBuffer.");

	status = this->kernel.setArg(4, this->textureRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg textureRefBuffer.");

	status = this->kernel.setArg(5, this->triangleBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg triangleBuffer.");

	status = this->kernel.setArg(6, this->textureBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg textureBuffer.");

	status = this->kernel.setArg(7, this->gameTimeBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg gameTimeBuffer.");

	status = this->kernel.setArg(8, this->outputBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg outputBuffer.");

	// --- TESTING PURPOSES ---
	// The following code is for some tests.

	this->makeTestWorld();

	// --- END TESTING ---
}

CLProgram::~CLProgram()
{

}

std::vector<cl::Platform> CLProgram::getPlatforms()
{
	std::vector<cl::Platform> platforms;
	cl_int status = cl::Platform::get(&platforms);
	Debug::check(status == CL_SUCCESS, "CLProgram", "CLProgram::getPlatforms.");

	return platforms;
}

std::vector<cl::Device> CLProgram::getDevices(const cl::Platform &platform,
	cl_device_type type)
{
	std::vector<cl::Device> devices;
	cl_int status = platform.getDevices(type, &devices);
	Debug::check((status == CL_SUCCESS) || (status == CL_DEVICE_NOT_FOUND),
		"CLProgram", "CLProgram::getDevices.");

	return devices;
}

std::string CLProgram::getBuildReport() const
{
	auto buildLog = this->program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(this->device);
	return buildLog;
}

std::string CLProgram::getErrorString(cl_int error) const
{
	switch (error)
	{
		// Run-time and JIT compiler errors.
	case 0: return "CL_SUCCESS";
	case -1: return "CL_DEVICE_NOT_FOUND";
	case -2: return "CL_DEVICE_NOT_AVAILABLE";
	case -3: return "CL_COMPILER_NOT_AVAILABLE";
	case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case -5: return "CL_OUT_OF_RESOURCES";
	case -6: return "CL_OUT_OF_HOST_MEMORY";
	case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case -8: return "CL_MEM_COPY_OVERLAP";
	case -9: return "CL_IMAGE_FORMAT_MISMATCH";
	case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case -11: return this->getBuildReport();
	case -12: return "CL_MAP_FAILURE";
	case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case -15: return "CL_COMPILE_PROGRAM_FAILURE";
	case -16: return "CL_LINKER_NOT_AVAILABLE";
	case -17: return "CL_LINK_PROGRAM_FAILURE";
	case -18: return "CL_DEVICE_PARTITION_FAILED";
	case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		// Compile-time errors.
	case -30: return "CL_INVALID_VALUE";
	case -31: return "CL_INVALID_DEVICE_TYPE";
	case -32: return "CL_INVALID_PLATFORM";
	case -33: return "CL_INVALID_DEVICE";
	case -34: return "CL_INVALID_CONTEXT";
	case -35: return "CL_INVALID_QUEUE_PROPERTIES";
	case -36: return "CL_INVALID_COMMAND_QUEUE";
	case -37: return "CL_INVALID_HOST_PTR";
	case -38: return "CL_INVALID_MEM_OBJECT";
	case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case -40: return "CL_INVALID_IMAGE_SIZE";
	case -41: return "CL_INVALID_SAMPLER";
	case -42: return "CL_INVALID_BINARY";
	case -43: return "CL_INVALID_BUILD_OPTIONS";
	case -44: return "CL_INVALID_PROGRAM";
	case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
	case -46: return "CL_INVALID_KERNEL_NAME";
	case -47: return "CL_INVALID_KERNEL_DEFINITION";
	case -48: return "CL_INVALID_KERNEL";
	case -49: return "CL_INVALID_ARG_INDEX";
	case -50: return "CL_INVALID_ARG_VALUE";
	case -51: return "CL_INVALID_ARG_SIZE";
	case -52: return "CL_INVALID_KERNEL_ARGS";
	case -53: return "CL_INVALID_WORK_DIMENSION";
	case -54: return "CL_INVALID_WORK_GROUP_SIZE";
	case -55: return "CL_INVALID_WORK_ITEM_SIZE";
	case -56: return "CL_INVALID_GLOBAL_OFFSET";
	case -57: return "CL_INVALID_EVENT_WAIT_LIST";
	case -58: return "CL_INVALID_EVENT";
	case -59: return "CL_INVALID_OPERATION";
	case -60: return "CL_INVALID_GL_OBJECT";
	case -61: return "CL_INVALID_BUFFER_SIZE";
	case -62: return "CL_INVALID_MIP_LEVEL";
	case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
	case -64: return "CL_INVALID_PROPERTY";
	case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
	case -66: return "CL_INVALID_COMPILER_OPTIONS";
	case -67: return "CL_INVALID_LINKER_OPTIONS";
	case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

		// Extension errors.
	case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
	case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
	case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
	case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
	case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
	case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
	default: return "Unknown OpenCL error \"" + std::to_string(error) + "\"";
	}
}

void CLProgram::makeTestWorld()
{
	Debug::mention("CLProgram", "Making test world.");

	// This code is not using an acceleration structure yet. It will be very slow if 
	// too many triangles are used!

	// Fill the triangle buffer with some test values. Remove these once actual data 
	// are used. Do nothing with voxel, sprite, light, and texture references yet.

	// Some test dimensions.
	const int width = 4;
	const int depth = 4;

	// Overwrite the existing triangle buffer.
	cl_int status = CL_SUCCESS;
	cl::size_type bufferSize = SIZEOF_TRIANGLE * width * depth * 2;
	this->triangleBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		bufferSize, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer triangleBuffer.");

	// Remind the kernel where to look for the new triangle buffer.
	status = this->kernel.setArg(5, this->triangleBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel::setArg test triangleBuffer.");

	std::vector<char> buffer(bufferSize);
	cl_char *bufPtr = reinterpret_cast<cl_char*>(buffer.data());

	// Lambda for writing triangle data to the local buffer.
	int trianglesWritten = 0;
	auto writeTriangle = [bufPtr, &trianglesWritten](const Triangle &triangle)
	{
		// Offset in the local buffer.
		cl_char *ptr = bufPtr + (trianglesWritten * SIZEOF_TRIANGLE);

		cl_float *p1Ptr = reinterpret_cast<cl_float*>(ptr);
		*(p1Ptr + 0) = static_cast<cl_float>(triangle.getP1().getX());
		*(p1Ptr + 1) = static_cast<cl_float>(triangle.getP1().getY());
		*(p1Ptr + 2) = static_cast<cl_float>(triangle.getP1().getZ());

		cl_float *p2Ptr = reinterpret_cast<cl_float*>(ptr + sizeof(cl_float3));
		*(p2Ptr + 0) = static_cast<cl_float>(triangle.getP2().getX());
		*(p2Ptr + 1) = static_cast<cl_float>(triangle.getP2().getY());
		*(p2Ptr + 2) = static_cast<cl_float>(triangle.getP2().getZ());

		cl_float *p3Ptr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 2));
		*(p3Ptr + 0) = static_cast<cl_float>(triangle.getP3().getX());
		*(p3Ptr + 1) = static_cast<cl_float>(triangle.getP3().getY());
		*(p3Ptr + 2) = static_cast<cl_float>(triangle.getP3().getZ());

		cl_float *normalPtr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 3));
		*(normalPtr + 0) = static_cast<cl_float>(triangle.getNormal().getX());
		*(normalPtr + 1) = static_cast<cl_float>(triangle.getNormal().getY());
		*(normalPtr + 2) = static_cast<cl_float>(triangle.getNormal().getZ());

		cl_float *uv1Ptr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 4));
		*(uv1Ptr + 0) = static_cast<cl_float>(triangle.getUV1().getX());
		*(uv1Ptr + 1) = static_cast<cl_float>(triangle.getUV1().getY());

		cl_float *uv2Ptr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 4) + 
			sizeof(cl_float2));
		*(uv2Ptr + 0) = static_cast<cl_float>(triangle.getUV2().getX());
		*(uv2Ptr + 1) = static_cast<cl_float>(triangle.getUV2().getY());

		cl_float *uv3Ptr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 4) + 
			(sizeof(cl_float2) * 2));
		*(uv3Ptr + 0) = static_cast<cl_float>(triangle.getUV3().getX());
		*(uv3Ptr + 1) = static_cast<cl_float>(triangle.getUV3().getY());

		trianglesWritten++;
	};

	// Write some ground triangles to the buffer.
	for (int k = 0; k < depth; ++k)
	{
		for (int i = 0; i < width; ++i)
		{
			double x = (static_cast<double>(width) * 0.5) - static_cast<double>(i);
			double y = -1.0;
			double z = (static_cast<double>(depth) * 0.5) - static_cast<double>(k);

			Triangle t1(
				Float3d(x, y, z),
				Float3d(x, y, z - 1.0),
				Float3d(x - 1.0, y, z - 1.0),
				Float2d(0.0, 0.0),
				Float2d(0.0, 1.0),
				Float2d(1.0, 1.0));

			Triangle t2(
				Float3d(x - 1.0, y, z - 1.0),
				Float3d(x - 1.0, y, z),
				Float3d(x, y, z),
				Float2d(1.0, 1.0),
				Float2d(1.0, 0.0),
				Float2d(0.0, 0.0));

			writeTriangle(t1);
			writeTriangle(t2);
		}
	}

	// Write the buffer to device memory.
	status = this->commandQueue.enqueueWriteBuffer(this->triangleBuffer,
		CL_TRUE, 0, buffer.size(), static_cast<const void*>(bufPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer test");
}

void CLProgram::updateCamera(const Float3d &eye, const Float3d &direction, double fovY)
{
	// Do not scale the direction beforehand.
	assert(direction.isNormalized());

	std::vector<char> buffer(SIZEOF_CAMERA);

	cl_char *bufPtr = reinterpret_cast<cl_char*>(buffer.data());

	// Write the components of the camera to the local buffer.
	// Correct spacing is very important.
	auto *eyePtr = reinterpret_cast<cl_float*>(bufPtr);
	*(eyePtr + 0) = static_cast<cl_float>(eye.getX());
	*(eyePtr + 1) = static_cast<cl_float>(eye.getY());
	*(eyePtr + 2) = static_cast<cl_float>(eye.getZ());

	auto *forwardPtr = reinterpret_cast<cl_float*>(bufPtr + sizeof(cl_float3));
	*(forwardPtr + 0) = static_cast<cl_float>(direction.getX());
	*(forwardPtr + 1) = static_cast<cl_float>(direction.getY());
	*(forwardPtr + 2) = static_cast<cl_float>(direction.getZ());

	auto right = direction.cross(Directable::getGlobalUp()).normalized();
	auto *rightPtr = reinterpret_cast<cl_float*>(bufPtr + (sizeof(cl_float3) * 2));
	*(rightPtr + 0) = static_cast<cl_float>(right.getX());
	*(rightPtr + 1) = static_cast<cl_float>(right.getY());
	*(rightPtr + 2) = static_cast<cl_float>(right.getZ());

	auto up = right.cross(direction).normalized();
	auto *upPtr = reinterpret_cast<cl_float*>(bufPtr + (sizeof(cl_float3) * 3));
	*(upPtr + 0) = static_cast<cl_float>(up.getX());
	*(upPtr + 1) = static_cast<cl_float>(up.getY());
	*(upPtr + 2) = static_cast<cl_float>(up.getZ());

	// Zoom is a function of field of view.
	double zoom = 1.0 / std::tan(fovY * 0.5 * DEG_TO_RAD);
	auto *zoomPtr = reinterpret_cast<cl_float*>(bufPtr + (sizeof(cl_float3) * 4));
	*zoomPtr = static_cast<cl_float>(zoom);

	// Write the buffer to device memory.
	cl_int status = this->commandQueue.enqueueWriteBuffer(this->cameraBuffer,
		CL_TRUE, 0, buffer.size(), static_cast<const void*>(bufPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer updateCamera");
}

void CLProgram::updateGameTime(double gameTime)
{
	assert(gameTime >= 0.0);

	std::vector<char> buffer(sizeof(cl_float));

	cl_char *bufPtr = reinterpret_cast<cl_char*>(buffer.data());

	auto *timePtr = reinterpret_cast<cl_float*>(bufPtr);
	*timePtr = static_cast<cl_float>(gameTime);

	// Write the buffer to device memory.
	cl_int status = this->commandQueue.enqueueWriteBuffer(this->gameTimeBuffer,
		CL_TRUE, 0, buffer.size(), static_cast<const void*>(bufPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer updateGameTime");
}

void CLProgram::render(SDL_Surface *dst)
{
	assert(dst != nullptr);
	assert(this->width == dst->w);
	assert(this->height == dst->h);

	// Run the kernel with the given dimensions.
	cl_int status = this->commandQueue.enqueueNDRangeKernel(this->kernel,
		cl::NullRange, cl::NDRange(this->width, this->height), cl::NullRange,
		nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::CommandQueue::enqueueNDRangeKernel.");

	// Copy the output buffer into the destination pixel buffer.
	// The CL_TRUE means this is a blocking command.
	status = this->commandQueue.enqueueReadBuffer(this->outputBuffer, CL_TRUE, 0,
		static_cast<cl::size_type>(sizeof(cl_int) * this->width * this->height),
		dst->pixels, nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::CommandQueue::enqueueReadBuffer.");
}
