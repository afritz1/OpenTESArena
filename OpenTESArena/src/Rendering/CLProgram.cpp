#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "CLProgram.h"

#include "../Entities/Directable.h"
#include "../Interface/Surface.h"
#include "../Math/Constants.h"
#include "../Math/Float2.h"
#include "../Math/Float3.h"
#include "../Math/Float4.h"
#include "../Math/Int2.h"
#include "../Math/Random.h"
#include "../Math/Rect3D.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"

namespace
{
	// These sizes are intended to match those of the .cl file structs. OpenCL 
	// aligns structs to multiples of 8 bytes. Additional padding is sometimes 
	// necessary to match struct alignment.
	const cl::size_type SIZEOF_CAMERA = (sizeof(cl_float3) * 4) + sizeof(cl_float) + 12;
	const cl::size_type SIZEOF_LIGHT = sizeof(cl_float3) * 2;
	const cl::size_type SIZEOF_LIGHT_REF = sizeof(cl_int) * 2;
	const cl::size_type SIZEOF_SPRITE_REF = sizeof(cl_int) * 2;
	const cl::size_type SIZEOF_TEXTURE_REF = sizeof(cl_int) + (sizeof(cl_short) * 2);
	const cl::size_type SIZEOF_RECTANGLE = (sizeof(cl_float3) * 6) + SIZEOF_TEXTURE_REF + 8;
	const cl::size_type SIZEOF_VOXEL_REF = sizeof(cl_int) * 2;
}

const std::string CLProgram::PATH = "data/kernels/";
const std::string CLProgram::FILENAME = "kernel.cl";
const std::string CLProgram::INTERSECT_KERNEL = "intersect";
const std::string CLProgram::AMBIENT_OCCLUSION_KERNEL = "ambientOcclusion";
const std::string CLProgram::RAY_TRACE_KERNEL = "rayTrace";
const std::string CLProgram::ANTI_ALIAS_KERNEL = "antiAlias";
const std::string CLProgram::POST_PROCESS_KERNEL = "postProcess";
const std::string CLProgram::CONVERT_TO_RGB_KERNEL = "convertToRGB";

CLProgram::CLProgram(int worldWidth, int worldHeight, int worldDepth, 
	TextureManager &textureManager, Renderer &renderer, double renderQuality)
	: textureManager(textureManager)
{
	assert(worldWidth > 0);
	assert(worldHeight > 0);
	assert(worldDepth > 0);

	Debug::mention("CLProgram", "Initializing.");

	const int screenWidth = renderer.getWindowDimensions().getX();
	const int screenHeight = renderer.getWindowDimensions().getY();

	// Render dimensions for ray tracing. To prevent issues when the user shrinks
	// the window down too far, clamp them to at least 1.
	this->renderWidth = std::max(static_cast<int>(screenWidth * renderQuality), 1);
	this->renderHeight = std::max(static_cast<int>(screenHeight * renderQuality), 1);

	this->worldWidth = worldWidth;
	this->worldHeight = worldHeight;
	this->worldDepth = worldDepth;

	// Create the local output pixel buffer.
	const int renderPixelCount = this->renderWidth * this->renderHeight;
	this->outputData = std::vector<char>(sizeof(cl_int) * renderPixelCount);
	
	// Create streaming texture to be used as the game world frame buffer.	
	this->texture = renderer.createTexture(SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, this->renderWidth, this->renderHeight);
	Debug::check(this->texture != nullptr, "CLProgram", "SDL_CreateTexture");

	// Get the OpenCL platforms (i.e., AMD, Intel, Nvidia) available on the machine.
	auto platforms = CLProgram::getPlatforms();
	Debug::check(platforms.size() > 0, "CLProgram", "No OpenCL platform found.");

	// Look at the first platform. Most computers shouldn't have more than one.
	// More robust code can check for multiple platforms in the future.
	const auto &platform = platforms.at(0);

	// Mention some version information about the platform (it should be okay if the 
	// platform version is higher than the device version).
	Debug::mention("CLProgram", "Platform version \"" +
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
	std::string source = File::toString(CLProgram::PATH + CLProgram::FILENAME);

	// Make some #defines to add to the kernel source.
	std::string defines = std::string("#define RENDER_WIDTH ") + std::to_string(this->renderWidth) +
		std::string("\n") + std::string("#define RENDER_HEIGHT ") +
		std::to_string(this->renderHeight) + std::string("\n") + 
		std::string("#define WORLD_WIDTH ") + std::to_string(worldWidth) + std::string("\n") +
		std::string("#define WORLD_HEIGHT ") + std::to_string(worldHeight) + std::string("\n") +
		std::string("#define WORLD_DEPTH ") + std::to_string(worldDepth) + std::string("\n");

	// Put the kernel source in a program object within the OpenCL context.
	this->program = cl::Program(this->context, defines + source, false, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Program.");

	// Add some kernel compilation switches.
	std::string options("-cl-fast-relaxed-math -cl-strict-aliasing");

	// Build the program into something executable. If compilation fails, the program stops.
	status = this->program.build(devices, options.c_str());
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Program::build (" +
		this->getErrorString(status) + ").");

	// Create the kernels and set their entry function to be a __kernel in the program.
	this->intersectKernel = cl::Kernel(
		this->program, CLProgram::INTERSECT_KERNEL.c_str(), &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel intersectKernel.");

	this->rayTraceKernel = cl::Kernel(
		this->program, CLProgram::RAY_TRACE_KERNEL.c_str(), &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel rayTraceKernel.");

	this->convertToRGBKernel = cl::Kernel(
		this->program, CLProgram::CONVERT_TO_RGB_KERNEL.c_str(), &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel convertToRGBKernel.");

	// Create the OpenCL buffers in the context for reading and/or writing.
	// NOTE: The size of some of these buffers is just a placeholder for now.
	this->cameraBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_CAMERA, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer cameraBuffer.");

	this->voxelRefBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_VOXEL_REF * worldWidth * worldHeight * worldDepth, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer voxelRefBuffer.");

	this->spriteRefBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_SPRITE_REF * worldWidth * worldHeight * worldDepth, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer spriteRefBuffer.");

	this->lightRefBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_LIGHT_REF * worldWidth * worldHeight * worldDepth, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer lightRefBuffer.");

	this->rectangleBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_RECTANGLE * 6 * worldWidth * worldHeight * worldDepth
		/* This buffer size is actually very naive. Much of it will just be air.
		Make a mapping of 3D cell coordinates to rectangles at some point to help. */,
		nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer rectangleBuffer.");

	this->lightBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_LIGHT /* Some # of lights * world dims, Placeholder size */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer lightBuffer.");

	this->textureBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		sizeof(cl_float4) * 64 * 64 * 32 /* Placeholder size, 32 textures */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer textureBuffer.");

	this->gameTimeBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		sizeof(cl_float), nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer gameTimeBuffer.");

	this->depthBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer depthBuffer.");

	this->normalBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float3) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer normalBuffer.");

	this->viewBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float3) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer viewBuffer.");

	this->pointBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float3) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer pointBuffer.");

	this->uvBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float2) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer uvBuffer.");

	this->rectangleIndexBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_int) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer rectangleIndexBuffer.");

	this->colorBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float3) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer colorBuffer.");

	this->outputBuffer = cl::Buffer(this->context, CL_MEM_WRITE_ONLY,
		sizeof(cl_int) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer outputBuffer.");

	// Tell the intersect kernel arguments where their buffers live.
	status = this->intersectKernel.setArg(0, this->cameraBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel cameraBuffer.");

	status = this->intersectKernel.setArg(1, this->voxelRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel voxelRefBuffer.");

	status = this->intersectKernel.setArg(2, this->spriteRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel spriteRefBuffer.");

	status = this->intersectKernel.setArg(3, this->rectangleBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel rectangleBuffer.");

	status = this->intersectKernel.setArg(4, this->textureBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel textureBuffer.");

	status = this->intersectKernel.setArg(5, this->depthBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel depthBuffer.");

	status = this->intersectKernel.setArg(6, this->normalBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel normalBuffer.");

	status = this->intersectKernel.setArg(7, this->viewBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel viewBuffer.");

	status = this->intersectKernel.setArg(8, this->pointBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel pointBuffer.");

	status = this->intersectKernel.setArg(9, this->uvBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel uvBuffer.");

	status = this->intersectKernel.setArg(10, this->rectangleIndexBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel rectangleIndexBuffer.");

	// Tell the rayTrace kernel arguments where their buffers live.
	status = this->rayTraceKernel.setArg(0, this->voxelRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel voxelRefBuffer.");

	status = this->rayTraceKernel.setArg(1, this->spriteRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel spriteRefBuffer.");

	status = this->rayTraceKernel.setArg(2, this->lightRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel lightRefBuffer.");

	status = this->rayTraceKernel.setArg(3, this->rectangleBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel rectangleBuffer.");

	status = this->rayTraceKernel.setArg(4, this->lightBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel lightBuffer.");

	status = this->rayTraceKernel.setArg(5, this->textureBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel textureBuffer.");

	status = this->rayTraceKernel.setArg(6, this->gameTimeBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel gameTimeBuffer.");

	status = this->rayTraceKernel.setArg(7, this->depthBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel depthBuffer.");

	status = this->rayTraceKernel.setArg(8, this->normalBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel normalBuffer.");

	status = this->rayTraceKernel.setArg(9, this->viewBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel viewBuffer.");

	status = this->rayTraceKernel.setArg(10, this->pointBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel pointBuffer.");

	status = this->rayTraceKernel.setArg(11, this->uvBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel uvBuffer.");

	status = this->rayTraceKernel.setArg(12, this->rectangleIndexBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel rectangleIndexBuffer.");

	status = this->rayTraceKernel.setArg(13, this->colorBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel colorBuffer.");

	// Tell the convertToRGB kernel arguments where their buffers live.
	status = this->convertToRGBKernel.setArg(0, this->colorBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg convertToRGBKernel colorBuffer.");

	status = this->convertToRGBKernel.setArg(1, this->outputBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg convertToRGBKernel outputBuffer.");

	// --- TESTING PURPOSES ---
	// The following code is for testing. Remove it once using actual world data.

	this->makeTestWorld();

	// --- END TESTING ---
}

CLProgram::~CLProgram()
{
	// Destroy the game world frame buffer.
	// The SDL_Renderer destroys this itself with SDL_DestroyRenderer(), too.
	SDL_DestroyTexture(this->texture);
}

CLProgram &CLProgram::operator=(CLProgram &&clProgram)
{
	// Is there a better way to do this?
	this->device = clProgram.device;
	this->context = clProgram.context;
	this->commandQueue = clProgram.commandQueue;
	this->program = clProgram.program;
	this->intersectKernel = clProgram.intersectKernel;
	this->rayTraceKernel = clProgram.rayTraceKernel;
	this->convertToRGBKernel = clProgram.convertToRGBKernel;
	this->cameraBuffer = clProgram.cameraBuffer;
	this->voxelRefBuffer = clProgram.voxelRefBuffer;
	this->spriteRefBuffer = clProgram.spriteRefBuffer;
	this->lightRefBuffer = clProgram.lightRefBuffer;
	this->rectangleBuffer = clProgram.rectangleBuffer;
	this->lightBuffer = clProgram.lightBuffer;
	this->textureBuffer = clProgram.textureBuffer;
	this->gameTimeBuffer = clProgram.gameTimeBuffer;
	this->depthBuffer = clProgram.depthBuffer;
	this->normalBuffer = clProgram.normalBuffer;
	this->viewBuffer = clProgram.viewBuffer;
	this->pointBuffer = clProgram.pointBuffer;
	this->uvBuffer = clProgram.uvBuffer;
	this->rectangleIndexBuffer = clProgram.rectangleIndexBuffer;
	this->colorBuffer = clProgram.colorBuffer;
	this->outputBuffer = clProgram.outputBuffer;
	this->outputData = clProgram.outputData;
	this->textureManager = std::move(clProgram.textureManager);
	this->renderWidth = clProgram.renderWidth;
	this->renderHeight = clProgram.renderHeight;
	this->worldWidth = clProgram.worldWidth;
	this->worldHeight = clProgram.worldHeight;
	this->worldDepth = clProgram.worldDepth;

	SDL_DestroyTexture(this->texture);
	this->texture = clProgram.texture;
	clProgram.texture = nullptr;

	return *this;
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

	// This method builds a simple test city with some blocks around.
	// It does nothing with sprites and lights yet.

	// Lambda for creating a vector of rectangles for a particular voxel.
	auto makeBlock = [](int cellX, int cellY, int cellZ)
	{
		float x = static_cast<float>(cellX);
		float y = static_cast<float>(cellY);
		float z = static_cast<float>(cellZ);
		const float sideLength = 1.0f;

		// Front.
		Rect3D r1(
			Float3f(x + sideLength, y + sideLength, z),
			Float3f(x + sideLength, y, z),
			Float3f(x, y, z));

		// Back.
		Rect3D r2(
			Float3f(x, y + sideLength, z + sideLength),
			Float3f(x, y, z + sideLength),
			Float3f(x + sideLength, y, z + sideLength));

		// Top.
		Rect3D r3(
			Float3f(x + sideLength, y + sideLength, z + sideLength),
			Float3f(x + sideLength, y + sideLength, z),
			Float3f(x, y + sideLength, z));

		// Bottom.
		Rect3D r4(
			Float3f(x + sideLength, y, z),
			Float3f(x + sideLength, y, z + sideLength),
			Float3f(x, y, z + sideLength));

		// Right.
		Rect3D r5(
			Float3f(x, y + sideLength, z),
			Float3f(x, y, z),
			Float3f(x, y, z + sideLength));

		// Left.
		Rect3D r6(
			Float3f(x + sideLength, y + sideLength, z + sideLength),
			Float3f(x + sideLength, y, z + sideLength),
			Float3f(x + sideLength, y, z));

		return std::vector<Rect3D>{ r1, r2, r3, r4, r5, r6 };
	};

	// Write some rectangles to a rectangle buffer.
	const int maxRectanglesPerVoxel = 6;
	size_t rectangleBufferSize = SIZEOF_RECTANGLE * maxRectanglesPerVoxel *
		this->worldWidth * this->worldHeight * this->worldDepth;
	std::vector<char> rectangleBuffer(rectangleBufferSize);
	cl_char *recPtr = reinterpret_cast<cl_char*>(rectangleBuffer.data());

	// Lambda for writing rectangle data to the local buffer.
	// - NOTE: using texture index here assumes that all textures are 64x64.
	auto writeRectangle = [this, maxRectanglesPerVoxel, recPtr](const Rect3D &rect,
		int cellX, int cellY, int cellZ, int rectangleOffset, int textureIndex)
	{
		assert(cellX >= 0);
		assert(cellY >= 0);
		assert(cellZ >= 0);
		assert(cellX < this->worldWidth);
		assert(cellY < this->worldHeight);
		assert(cellZ < this->worldDepth);

		// Only 6 rectangles max per block for now.
		assert(rectangleOffset < maxRectanglesPerVoxel);

		// Offset in the local buffer.
		cl_char *ptr = recPtr + (cellX * SIZEOF_RECTANGLE * maxRectanglesPerVoxel) +
			((cellY * this->worldWidth) * SIZEOF_RECTANGLE * maxRectanglesPerVoxel) +
			((cellZ * this->worldWidth * this->worldHeight) * SIZEOF_RECTANGLE *
				maxRectanglesPerVoxel) + (rectangleOffset * SIZEOF_RECTANGLE);

		cl_float *p1Ptr = reinterpret_cast<cl_float*>(ptr);
		*(p1Ptr + 0) = static_cast<cl_float>(rect.getP1().getX());
		*(p1Ptr + 1) = static_cast<cl_float>(rect.getP1().getY());
		*(p1Ptr + 2) = static_cast<cl_float>(rect.getP1().getZ());

		cl_float *p2Ptr = reinterpret_cast<cl_float*>(ptr + sizeof(cl_float3));
		*(p2Ptr + 0) = static_cast<cl_float>(rect.getP2().getX());
		*(p2Ptr + 1) = static_cast<cl_float>(rect.getP2().getY());
		*(p2Ptr + 2) = static_cast<cl_float>(rect.getP2().getZ());

		cl_float *p3Ptr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 2));
		*(p3Ptr + 0) = static_cast<cl_float>(rect.getP3().getX());
		*(p3Ptr + 1) = static_cast<cl_float>(rect.getP3().getY());
		*(p3Ptr + 2) = static_cast<cl_float>(rect.getP3().getZ());

		const Float3f p1p2 = rect.getP2() - rect.getP1();
		cl_float *p1p2Ptr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 3));
		*(p1p2Ptr + 0) = static_cast<cl_float>(p1p2.getX());
		*(p1p2Ptr + 1) = static_cast<cl_float>(p1p2.getY());
		*(p1p2Ptr + 2) = static_cast<cl_float>(p1p2.getZ());

		const Float3f p2p3 = rect.getP3() - rect.getP2();
		cl_float *p2p3Ptr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 4));
		*(p2p3Ptr + 0) = static_cast<cl_float>(p2p3.getX());
		*(p2p3Ptr + 1) = static_cast<cl_float>(p2p3.getY());
		*(p2p3Ptr + 2) = static_cast<cl_float>(p2p3.getZ());

		const Float3f normal = rect.getNormal();
		cl_float *normalPtr = reinterpret_cast<cl_float*>(ptr + (sizeof(cl_float3) * 5));
		*(normalPtr + 0) = static_cast<cl_float>(normal.getX());
		*(normalPtr + 1) = static_cast<cl_float>(normal.getY());
		*(normalPtr + 2) = static_cast<cl_float>(normal.getZ());

		cl_int *offsetPtr = reinterpret_cast<cl_int*>(ptr + (sizeof(cl_float3) * 6));
		*(offsetPtr + 0) = 64 * 64 * textureIndex; // Number of float4's to skip.

		cl_short *dimPtr = reinterpret_cast<cl_short*>(ptr + (sizeof(cl_float3) * 6) +
			sizeof(cl_int));
		*(dimPtr + 0) = 64;
		*(dimPtr + 1) = 64;
	};

	// Fill a voxel reference buffer with references to those rectangles.
	size_t voxelRefBufferSize = SIZEOF_VOXEL_REF * this->worldWidth * 
		this->worldHeight * this->worldDepth;
	std::vector<char> voxelRefBuffer(voxelRefBufferSize);
	cl_char *voxPtr = reinterpret_cast<cl_char*>(voxelRefBuffer.data());

	// Lambda for writing a voxel reference into the local buffer. This only works 
	// when using the naive rectangle array storage (i.e., *every* voxel has 6 
	// rectangle slots). Consider making the offset based on the number of rectangles
	// written, instead of an arbitrary XYZ coordinate.
	auto writeVoxelRef = [this, voxPtr](int cellX, int cellY, int cellZ, int count)
	{
		assert(cellX >= 0);
		assert(cellY >= 0);
		assert(cellZ >= 0);
		assert(cellX < this->worldWidth);
		assert(cellY < this->worldHeight);
		assert(cellZ < this->worldDepth);
		assert(count >= 0);

		cl_char *ptr = voxPtr + (cellX * SIZEOF_VOXEL_REF) +
			((cellY * this->worldWidth) * SIZEOF_VOXEL_REF) +
			((cellZ * this->worldWidth * this->worldHeight) * SIZEOF_VOXEL_REF);

		// Number of rectangles to skip in the rectangles array.
		int offset = 6 * (cellX + (cellY * this->worldWidth) +
			(cellZ * this->worldWidth * this->worldHeight));

		cl_int *offsetPtr = reinterpret_cast<cl_int*>(ptr);
		*(offsetPtr + 0) = offset;

		cl_int *countPtr = reinterpret_cast<cl_int*>(ptr + sizeof(cl_int));
		*(countPtr + 0) = count;
	};

	// Prepare some textures for a local float4 buffer.	
	this->textureManager.setPalette(PaletteName::Default);
	std::vector<const SDL_Surface*> textures =
	{
		this->textureManager.getSurface("T_CITYWL.IMG").getSurface(),
		this->textureManager.getSurface("T_NGRASS.IMG").getSurface(),
		this->textureManager.getSurface("T_NROAD.IMG").getSurface(),
		this->textureManager.getSurface("T_NSDWLK.IMG").getSurface(),
		this->textureManager.getSurface("T_GARDEN.IMG").getSurface(),
	};

	const int textureCount = static_cast<int>(textures.size());
	const int textureWidth = 64;
	const int textureHeight = 64;
	size_t textureBufferSize = sizeof(cl_float4) * textureWidth * 
		textureHeight * textureCount;
	std::vector<char> textureBuffer(textureBufferSize);
	cl_char *texPtr = reinterpret_cast<cl_char*>(textureBuffer.data());
	
	// Pack the texture data into the local buffer.
	for (int i = 0; i < textureCount; ++i)
	{
		const SDL_Surface *texture = textures.at(i);
		const uint32_t *pixels = static_cast<uint32_t*>(texture->pixels);

		int pixelOffset = sizeof(cl_float4) * textureWidth * textureHeight * i;
		cl_float4 *pixelPtr = reinterpret_cast<cl_float4*>(texPtr + pixelOffset);

		for (int y = 0; y < texture->h; ++y)
		{
			for (int x = 0; x < texture->w; ++x)
			{
				int index = x + (y * texture->w);

				// Convert from ARGB int to RGBA float4.
				Float4f color = Float4f::fromARGB(pixels[index]);

				cl_float *colorPtr = reinterpret_cast<cl_float*>(pixelPtr + index);
				*(colorPtr + 0) = static_cast<cl_float>(color.getX());
				*(colorPtr + 1) = static_cast<cl_float>(color.getY());
				*(colorPtr + 2) = static_cast<cl_float>(color.getZ());

				// Transparency depends on whether the pixel is black.
				*(colorPtr + 3) = static_cast<cl_float>(pixels[index] == 0 ? 0.0f : 1.0f);
			}
		}
	}

	// Zero out all the voxel references to start.
	for (int k = 0; k < this->worldDepth; ++k)
	{
		for (int j = 0; j < this->worldHeight; ++j)
		{
			for (int i = 0; i < this->worldWidth; ++i)
			{
				writeVoxelRef(i, j, k, 0);
			}
		}
	}

	// Use the same seed so it's not a new city on every screen resize.
	Random random(2);

	// Make the ground.
	for (int k = 0; k < this->worldDepth; ++k)
	{
		for (int i = 0; i < this->worldWidth; ++i)
		{
			std::vector<Rect3D> block = makeBlock(i, 0, k);
			int rectangleCount = static_cast<int>(block.size());

			int textureIndex = 1 + random.next(3);
			for (int index = 0; index < rectangleCount; ++index)
			{
				writeRectangle(block.at(index), i, 0, k, index, textureIndex);
			}

			writeVoxelRef(i, 0, k, 6);
		}
	}

	// Make the near X and far X walls.
	for (int j = 1; j < this->worldHeight; ++j)
	{
		for (int k = 0; k < this->worldDepth; ++k)
		{
			std::vector<Rect3D> block = makeBlock(0, j, k);
			int rectangleCount = static_cast<int>(block.size());

			for (int index = 0; index < rectangleCount; ++index)
			{
				writeRectangle(block.at(index), 0, j, k, index, 0);
			}

			block = makeBlock(this->worldWidth - 1, j, k);
			for (int index = 0; index < rectangleCount; ++index)
			{
				writeRectangle(block.at(index), this->worldWidth - 1, j, k, index, 0);
			}

			writeVoxelRef(0, j, k, 6);
			writeVoxelRef(this->worldWidth - 1, j, k, 6);
		}
	}

	// Make the near Z and far Z walls (ignoring existing corners).
	for (int j = 1; j < this->worldHeight; ++j)
	{
		for (int i = 1; i < (this->worldWidth - 1); ++i)
		{
			std::vector<Rect3D> block = makeBlock(i, j, 0);
			int rectangleCount = static_cast<int>(block.size());

			for (int index = 0; index < rectangleCount; ++index)
			{
				writeRectangle(block.at(index), i, j, 0, index, 0);
			}

			block = makeBlock(i, j, this->worldDepth - 1);
			for (int index = 0; index < rectangleCount; ++index)
			{
				writeRectangle(block.at(index), i, j, this->worldDepth - 1, index, 0);
			}

			writeVoxelRef(i, j, 0, 6);
			writeVoxelRef(i, j, this->worldDepth - 1, 6);
		}
	}

	// Add some random blocks around.
	for (int count = 0; count < 32; ++count)
	{
		int x = 1 + random.next(this->worldWidth - 2);
		int y = 1;
		int z = 1 + random.next(this->worldDepth - 2);

		std::vector<Rect3D> block = makeBlock(x, y, z);
		int rectangleCount = static_cast<int>(block.size());

		for (int index = 0; index < rectangleCount; ++index)
		{
			writeRectangle(block.at(index), x, y, z, index, 4);
		}

		writeVoxelRef(x, y, z, 6);
	}

	// Write the rectangle buffer to device memory.
	cl_int status = this->commandQueue.enqueueWriteBuffer(this->rectangleBuffer,
		CL_TRUE, 0, rectangleBufferSize, static_cast<const void*>(recPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer test rectangleBuffer");

	// Write the voxel reference buffer to device memory.
	status = this->commandQueue.enqueueWriteBuffer(this->voxelRefBuffer,
		CL_TRUE, 0, voxelRefBufferSize, static_cast<const void*>(voxPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer test voxelRefBuffer");

	// Write the texture buffer to device memory.
	status = this->commandQueue.enqueueWriteBuffer(this->textureBuffer,
		CL_TRUE, 0, textureBufferSize, static_cast<const void*>(texPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer test textureBuffer");
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

void CLProgram::render(Renderer &renderer)
{
	cl::NDRange workDims(this->renderWidth, this->renderHeight);

	// Run the intersect kernel.
	cl_int status = this->commandQueue.enqueueNDRangeKernel(this->intersectKernel,
		cl::NullRange, workDims, cl::NullRange, nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::CommandQueue::enqueueNDRangeKernel intersectKernel.");

	// Run the ray tracing kernel using the results from the intersect kernel.
	status = this->commandQueue.enqueueNDRangeKernel(this->rayTraceKernel,
		cl::NullRange, workDims, cl::NullRange, nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::CommandQueue::enqueueNDRangeKernel rayTraceKernel.");

	// Run the RGB conversion kernel using the results from ray tracing.
	status = this->commandQueue.enqueueNDRangeKernel(this->convertToRGBKernel,
		cl::NullRange, workDims, cl::NullRange, nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::CommandQueue::enqueueNDRangeKernel convertToRGBKernel.");

	// Copy the output buffer into the destination pixel buffer.
	void *outputDataPtr = static_cast<void*>(this->outputData.data());
	const int renderPixelCount = this->renderWidth * this->renderHeight;
	status = this->commandQueue.enqueueReadBuffer(this->outputBuffer, CL_TRUE, 0,
		static_cast<cl::size_type>(sizeof(cl_int) * renderPixelCount),
		outputDataPtr, nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::CommandQueue::enqueueReadBuffer.");

	// Update the frame buffer texture and draw to the renderer.
	const int pitch = this->renderWidth * sizeof(cl_int);
	SDL_UpdateTexture(this->texture, nullptr, outputDataPtr, pitch);
	renderer.fillNative(this->texture);
}
