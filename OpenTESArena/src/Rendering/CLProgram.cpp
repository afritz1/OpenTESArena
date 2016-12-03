#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

#include "CLProgram.h"

#include "../Entities/Directable.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/BufferView.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"

namespace
{
	// These sizes are intended to match those of the .cl file structs. OpenCL 
	// aligns structs to multiples of 8 bytes. Additional padding is sometimes 
	// necessary to match struct alignment.
	const cl::size_type SIZEOF_CAMERA = (sizeof(cl_float3) * 4) + sizeof(cl_float) + 12;
	const cl::size_type SIZEOF_OWNER_REF = sizeof(cl_int) * 2;
	const cl::size_type SIZEOF_LIGHT = (sizeof(cl_float3) * 2) + SIZEOF_OWNER_REF + sizeof(cl_float) + 4;
	const cl::size_type SIZEOF_LIGHT_REF = sizeof(cl_int) * 2;
	const cl::size_type SIZEOF_SPRITE_REF = sizeof(cl_int) * 2;
	const cl::size_type SIZEOF_TEXTURE_REF = sizeof(cl_int) + (sizeof(cl_short) * 2);
	const cl::size_type SIZEOF_RECTANGLE = (sizeof(cl_float3) * 6) + SIZEOF_TEXTURE_REF + 8;
	const cl::size_type SIZEOF_VOXEL_REF = sizeof(cl_int) * 2;
}

const std::string CLProgram::PATH = "data/kernels/";
const std::string CLProgram::FILENAME = "kernel.cl";
const std::string CLProgram::INTERSECT_KERNEL = "intersect";
const std::string CLProgram::RAY_TRACE_KERNEL = "rayTrace";
const std::string CLProgram::POST_PROCESS_KERNEL = "postProcess";
const std::string CLProgram::CONVERT_TO_RGB_KERNEL = "convertToRGB";

CLProgram::CLProgram(int worldWidth, int worldHeight, int worldDepth,
	int renderWidth, int renderHeight)
{
	assert(worldWidth > 0);
	assert(worldHeight > 0);
	assert(worldDepth > 0);

	Debug::mention("CLProgram", "Initializing.");

	// Render dimensions for ray tracing. To prevent issues when the user shrinks
	// the window down too far, clamp them to at least 1.
	this->renderWidth = renderWidth;
	this->renderHeight = renderHeight;

	this->worldWidth = worldWidth;
	this->worldHeight = worldHeight;
	this->worldDepth = worldDepth;

	// Create the local output pixel buffer.
	const int renderPixelCount = this->renderWidth * this->renderHeight;
	this->outputData = std::vector<uint8_t>(sizeof(cl_int) * renderPixelCount);

	// Initialize rectangle buffer view to empty.
	this->rectBufferView = std::unique_ptr<BufferView>(new BufferView());

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
	std::string defines("#define RENDER_WIDTH " + std::to_string(this->renderWidth) +
		"\n" + "#define RENDER_HEIGHT " + std::to_string(this->renderHeight) + "\n" +
		"#define WORLD_WIDTH " + std::to_string(worldWidth) + "\n" +
		"#define WORLD_HEIGHT " + std::to_string(worldHeight) + "\n" +
		"#define WORLD_DEPTH " + std::to_string(worldDepth) + "\n");

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
		static_cast<cl::size_type>(65536) /* Resized as necessary. */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer rectangleBuffer.");

	this->lightBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		static_cast<cl::size_type>(4096) /* Resized as necessary. */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer lightBuffer.");

	this->ownerBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		static_cast<cl::size_type>(4096) /* Resized as necessary. */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer ownerBuffer.");

	this->textureBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		static_cast<cl::size_type>(65536) /* Resized as necessary. */, nullptr, &status);
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

	// Make sure certain buffers are completely zeroed out. Some OpenCL implementations 
	// (like Nvidia) do not do this automatically at initialization, and can cause
	// out-of-bounds errors in the kernel if garbage values are read.
	this->zeroBuffer(this->voxelRefBuffer);
	this->zeroBuffer(this->spriteRefBuffer);
	this->zeroBuffer(this->lightRefBuffer);

	// Tell the intersect kernel arguments where their buffers live. Don't forget 
	// that many of these are also refreshed in CLProgram::resize() and
	// CLProgram::addTexture().
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

	status = this->rayTraceKernel.setArg(5, this->ownerBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel ownerBuffer.");

	status = this->rayTraceKernel.setArg(6, this->textureBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel textureBuffer.");

	status = this->rayTraceKernel.setArg(7, this->gameTimeBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel gameTimeBuffer.");

	status = this->rayTraceKernel.setArg(8, this->depthBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel depthBuffer.");

	status = this->rayTraceKernel.setArg(9, this->normalBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel normalBuffer.");

	status = this->rayTraceKernel.setArg(10, this->viewBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel viewBuffer.");

	status = this->rayTraceKernel.setArg(11, this->pointBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel pointBuffer.");

	status = this->rayTraceKernel.setArg(12, this->uvBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel uvBuffer.");

	status = this->rayTraceKernel.setArg(13, this->rectangleIndexBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel rectangleIndexBuffer.");

	status = this->rayTraceKernel.setArg(14, this->colorBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel colorBuffer.");

	// Tell the convertToRGB kernel arguments where their buffers live.
	status = this->convertToRGBKernel.setArg(0, this->colorBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg convertToRGBKernel colorBuffer.");

	status = this->convertToRGBKernel.setArg(1, this->outputBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg convertToRGBKernel outputBuffer.");
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

void CLProgram::resize(int renderWidth, int renderHeight)
{
	// Since the CL program is tightly coupled with the render resolution, nearly all
	// of the memory objects need to be refreshed. However, buffers with world data
	// just need to be referenced by the new kernels.

	// I don't really like this solution because it's basically 3/4 of the CLProgram
	// constructor, but I couldn't do this cleanly using the move assignment operator. 
	// There's probably a better way to do this, though.

	this->renderWidth = renderWidth;
	this->renderHeight = renderHeight;

	// Recreate the local output pixel buffer.
	const int renderPixelCount = this->renderWidth * this->renderHeight;
	this->outputData = std::vector<uint8_t>(sizeof(cl_int) * renderPixelCount);

	// Read the kernel source from file.
	std::string source = File::toString(CLProgram::PATH + CLProgram::FILENAME);

	// Make some #defines to add to the kernel source.
	std::string defines("#define RENDER_WIDTH " + std::to_string(this->renderWidth) +
		"\n" + "#define RENDER_HEIGHT " + std::to_string(this->renderHeight) + "\n" +
		"#define WORLD_WIDTH " + std::to_string(worldWidth) + "\n" +
		"#define WORLD_HEIGHT " + std::to_string(worldHeight) + "\n" +
		"#define WORLD_DEPTH " + std::to_string(worldDepth) + "\n");

	// Put the kernel source in a program object within the OpenCL context.
	cl_int status = CL_SUCCESS;
	this->program = cl::Program(this->context, defines + source, false, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "resize cl::Program.");

	// Add some kernel compilation switches.
	std::string options("-cl-fast-relaxed-math -cl-strict-aliasing");

	// Rebuild the program.
	status = this->program.build(std::vector<cl::Device>{ this->device }, options.c_str());
	Debug::check(status == CL_SUCCESS, "CLProgram", "resize cl::Program::build (" +
		this->getErrorString(status) + ").");

	// Recreate the kernels.
	this->intersectKernel = cl::Kernel(
		this->program, CLProgram::INTERSECT_KERNEL.c_str(), &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel resize intersectKernel.");

	this->rayTraceKernel = cl::Kernel(
		this->program, CLProgram::RAY_TRACE_KERNEL.c_str(), &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel resize rayTraceKernel.");

	this->convertToRGBKernel = cl::Kernel(
		this->program, CLProgram::CONVERT_TO_RGB_KERNEL.c_str(), &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel resize convertToRGBKernel.");

	// Recreate the buffers dependent on render resolution.
	this->depthBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resize depthBuffer.");

	this->normalBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float3) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resize normalBuffer.");

	this->viewBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float3) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resize viewBuffer.");

	this->pointBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float3) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resize pointBuffer.");

	this->uvBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float2) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resize uvBuffer.");

	this->rectangleIndexBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_int) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resize rectangleIndexBuffer.");

	this->colorBuffer = cl::Buffer(this->context, CL_MEM_READ_WRITE,
		sizeof(cl_float3) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resize colorBuffer.");

	this->outputBuffer = cl::Buffer(this->context, CL_MEM_WRITE_ONLY,
		sizeof(cl_int) * renderPixelCount, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resize outputBuffer.");

	// Tell the intersect kernel arguments where their buffers live.
	status = this->intersectKernel.setArg(0, this->cameraBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize cameraBuffer.");

	status = this->intersectKernel.setArg(1, this->voxelRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize voxelRefBuffer.");

	status = this->intersectKernel.setArg(2, this->spriteRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize spriteRefBuffer.");

	status = this->intersectKernel.setArg(3, this->rectangleBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize rectangleBuffer.");

	status = this->intersectKernel.setArg(4, this->textureBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize textureBuffer.");

	status = this->intersectKernel.setArg(5, this->depthBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize depthBuffer.");

	status = this->intersectKernel.setArg(6, this->normalBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize normalBuffer.");

	status = this->intersectKernel.setArg(7, this->viewBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize viewBuffer.");

	status = this->intersectKernel.setArg(8, this->pointBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize pointBuffer.");

	status = this->intersectKernel.setArg(9, this->uvBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize uvBuffer.");

	status = this->intersectKernel.setArg(10, this->rectangleIndexBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg intersectKernel resize rectangleIndexBuffer.");

	// Tell the rayTrace kernel arguments where their buffers live.
	status = this->rayTraceKernel.setArg(0, this->voxelRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize voxelRefBuffer.");

	status = this->rayTraceKernel.setArg(1, this->spriteRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize spriteRefBuffer.");

	status = this->rayTraceKernel.setArg(2, this->lightRefBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize lightRefBuffer.");

	status = this->rayTraceKernel.setArg(3, this->rectangleBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize rectangleBuffer.");

	status = this->rayTraceKernel.setArg(4, this->lightBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize lightBuffer.");

	status = this->rayTraceKernel.setArg(5, this->ownerBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize ownerBuffer.");

	status = this->rayTraceKernel.setArg(6, this->textureBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize textureBuffer.");

	status = this->rayTraceKernel.setArg(7, this->gameTimeBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize gameTimeBuffer.");

	status = this->rayTraceKernel.setArg(8, this->depthBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize depthBuffer.");

	status = this->rayTraceKernel.setArg(9, this->normalBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize normalBuffer.");

	status = this->rayTraceKernel.setArg(10, this->viewBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize viewBuffer.");

	status = this->rayTraceKernel.setArg(11, this->pointBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize pointBuffer.");

	status = this->rayTraceKernel.setArg(12, this->uvBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize uvBuffer.");

	status = this->rayTraceKernel.setArg(13, this->rectangleIndexBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize rectangleIndexBuffer.");

	status = this->rayTraceKernel.setArg(14, this->colorBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize colorBuffer.");

	// Tell the convertToRGB kernel arguments where their buffers live.
	status = this->convertToRGBKernel.setArg(0, this->colorBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg convertToRGBKernel resize colorBuffer.");

	status = this->convertToRGBKernel.setArg(1, this->outputBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg convertToRGBKernel resize outputBuffer.");
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

void CLProgram::zeroBuffer(cl::Buffer &buffer)
{
	cl_int status = CL_SUCCESS;

	// Get the current size of the buffer.
	const cl::size_type bufferSize = buffer.getInfo<CL_MEM_SIZE>(&status);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Buffer::getInfo CL_MEM_SIZE resizeBuffer.");

	std::vector<cl_char> zeroes(bufferSize);
	std::memset(zeroes.data(), 0, zeroes.size());
	status = this->commandQueue.enqueueWriteBuffer(buffer, CL_TRUE,
		0, bufferSize, static_cast<const void*>(zeroes.data()), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer zeroBuffer.");
}

void CLProgram::resizeBuffer(cl::Buffer &buffer, cl::size_type newSize)
{
	cl_int status = CL_SUCCESS;

	// Get the current size of the buffer.
	const cl::size_type bufferSize = buffer.getInfo<CL_MEM_SIZE>(&status);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Buffer::getInfo CL_MEM_SIZE resizeBuffer.");

	// Read the existing buffer into a temp data buffer.
	std::vector<cl_char> oldData(bufferSize);
	status = this->commandQueue.enqueueReadBuffer(buffer, CL_TRUE, 0,
		bufferSize, static_cast<void*>(oldData.data()), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueReadBuffer resizeBuffer.");

	// Flags for the original buffer (read-only, read-write, etc.).
	const cl_mem_flags bufferFlags = buffer.getInfo<CL_MEM_FLAGS>(&status);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Buffer::getInfo CL_MEM_FLAGS resizeBuffer.");

	// Allocate a new buffer in device memory, erasing the old one.
	buffer = cl::Buffer(this->context, bufferFlags, newSize, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer resizeBuffer.");

	// Write the old data into the new buffer. If the new buffer is smaller, then
	// truncate the old data.
	const cl::size_type bytesToWrite = std::min(newSize, bufferSize);
	status = this->commandQueue.enqueueWriteBuffer(buffer, CL_TRUE, 0,
		bytesToWrite, static_cast<const void*>(oldData.data()), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer resizeBuffer.");
}

void CLProgram::resizeRectBuffer(cl::size_type requiredSize)
{
	cl_int status = CL_SUCCESS;
	const cl::size_type rectBufferSize = this->rectangleBuffer.getInfo<CL_MEM_SIZE>(&status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer::getInfo resizeRectBuffer.");

	const size_t newSize = [requiredSize, rectBufferSize]()
	{
		// Make sure size is at least 1 before doubling.
		size_t size = std::max(rectBufferSize, static_cast<size_t>(1));
		while (size < requiredSize)
		{
			size *= 2;
		}
		return size;
	}();

	Debug::mention("CLProgram", "Resizing geometry memory from " +
		std::to_string(rectBufferSize) + " to " + std::to_string(newSize) + " bytes.");

	// Resize the rectangle buffer to the new size.
	this->resizeBuffer(this->rectangleBuffer, newSize);

	// Tell the kernels where to find the new rectangle buffer.
	status = this->intersectKernel.setArg(3, this->rectangleBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg resizeRectBuffer intersectKernel.");

	status = this->rayTraceKernel.setArg(3, this->rectangleBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg resizeRectBuffer rayTraceKernel.");
}

std::vector<Int3> CLProgram::getTouchedVoxels(const Rect3D &rect, bool clampBounds) const
{
	// Create an axis-aligned bounding box for the rectangle.
	const Float3f &p1 = rect.getP1();
	const Float3f &p2 = rect.getP2();
	const Float3f &p3 = rect.getP3();
	const Float3f p4 = p1 + (p3 - p2);
	const Float3f boxMin(
		std::min(p1.getX(), std::min(p2.getX(), std::min(p3.getX(), p4.getX()))),
		std::min(p1.getY(), std::min(p2.getY(), std::min(p3.getY(), p4.getY()))),
		std::min(p1.getZ(), std::min(p2.getZ(), std::min(p3.getZ(), p4.getZ()))));
	const Float3f boxMax(
		std::max(p1.getX(), std::max(p2.getX(), std::max(p3.getX(), p4.getX()))),
		std::max(p1.getY(), std::max(p2.getY(), std::max(p3.getY(), p4.getY()))),
		std::max(p1.getZ(), std::max(p2.getZ(), std::max(p3.getZ(), p4.getZ()))));

	// Lambda to convert a 3D point to a voxel coordinate, optionally clamping
	// within world bounds if requested.
	auto getVoxelCoordinate = [this, clampBounds](const Float3f &point)
	{
		const int pX = static_cast<int>(point.getX());
		const int pY = static_cast<int>(point.getY());
		const int pZ = static_cast<int>(point.getZ());

		if (clampBounds)
		{
			return Int3(
				std::max(0, std::min(this->worldWidth - 1, pX)),
				std::max(0, std::min(this->worldHeight - 1, pY)),
				std::max(0, std::min(this->worldDepth - 1, pZ)));
		}
		else
		{
			return Int3(pX, pY, pZ);
		}
	};

	// Voxel coordinates for the nearest and farthest corners from the origin.
	const Int3 voxelMin = getVoxelCoordinate(boxMin);
	const Int3 voxelMax = getVoxelCoordinate(boxMax);

	// Insert a 3D coordinate for every voxel the bounding box touches.
	std::vector<Int3> coordinates;
	for (int k = voxelMin.getZ(); k <= voxelMax.getZ(); ++k)
	{
		for (int j = voxelMin.getY(); j <= voxelMax.getY(); ++j)
		{
			for (int i = voxelMin.getX(); i <= voxelMax.getX(); ++i)
			{
				coordinates.push_back(Int3(i, j, k));
			}
		}
	}

	return coordinates;

	// This bounding box method sometimes gives false positives (resulting in 
	// wasted time checking an unrelated voxel) because when a sprite covers three 
	// voxels in an L, the bounding box will incorrectly cover a fourth voxel even 
	// though the sprite itself isn't touching it. It won't result in incorrect
	// behavior though; just less than optimal rectangle bounds.

	// A smarter method would look at the sprite from above, treating it like
	// a 2D line segment in the XZ plane and doing Bresenham's line algorithm,
	// then extruding that line by the height of the sprite across the Y axis to 
	// get all touched voxels.
}

std::vector<Int3> CLProgram::getTouchedVoxels(const Float3d &point, 
	double intensity, bool clampBounds) const
{
	// Quick and easy solution: just enclose the light's reach within a box and get 
	// all voxels touching that box. A bit of wasted effort when rendering, though.

	const double halfIntensity = intensity * 0.5;

	const Float3f boxMin(
		static_cast<float>(point.getX() - halfIntensity),
		static_cast<float>(point.getY() - halfIntensity),
		static_cast<float>(point.getZ() - halfIntensity));
	const Float3f boxMax(
		static_cast<float>(point.getX() + halfIntensity),
		static_cast<float>(point.getY() + halfIntensity),
		static_cast<float>(point.getZ() + halfIntensity));

	// Lambda to convert a 3D point to a voxel coordinate, optionally clamping
	// within world bounds if requested.
	auto getVoxelCoordinate = [this, clampBounds](const Float3f &point)
	{
		const int pX = static_cast<int>(point.getX());
		const int pY = static_cast<int>(point.getY());
		const int pZ = static_cast<int>(point.getZ());

		if (clampBounds)
		{
			return Int3(
				std::max(0, std::min(this->worldWidth - 1, pX)),
				std::max(0, std::min(this->worldHeight - 1, pY)),
				std::max(0, std::min(this->worldDepth - 1, pZ)));
		}
		else
		{
			return Int3(pX, pY, pZ);
		}
	};

	// Voxel coordinates for the nearest and farthest corners from the origin.
	const Int3 voxelMin = getVoxelCoordinate(boxMin);
	const Int3 voxelMax = getVoxelCoordinate(boxMax);

	// Insert a 3D coordinate for every voxel the bounding box touches.
	std::vector<Int3> coordinates;
	for (int k = voxelMin.getZ(); k <= voxelMax.getZ(); ++k)
	{
		for (int j = voxelMin.getY(); j <= voxelMax.getY(); ++j)
		{
			for (int i = voxelMin.getX(); i <= voxelMax.getX(); ++i)
			{
				coordinates.push_back(Int3(i, j, k));
			}
		}
	}

	return coordinates;

	// A smarter version would treat the light's reach like a sphere and get all
	// voxels touched by that sphere instead.
}

void CLProgram::writeRect(const Rect3D &rect, const TextureReference &textureRef,
	std::vector<cl_char> &buffer, size_t byteOffset) const
{
	cl_char *recPtr = buffer.data() + byteOffset;

	const Float3f &p1 = rect.getP1();
	cl_float *p1Ptr = reinterpret_cast<cl_float*>(recPtr);
	p1Ptr[0] = static_cast<cl_float>(p1.getX());
	p1Ptr[1] = static_cast<cl_float>(p1.getY());
	p1Ptr[2] = static_cast<cl_float>(p1.getZ());

	const Float3f &p2 = rect.getP2();
	cl_float *p2Ptr = reinterpret_cast<cl_float*>(recPtr + sizeof(cl_float3));
	p2Ptr[0] = static_cast<cl_float>(p2.getX());
	p2Ptr[1] = static_cast<cl_float>(p2.getY());
	p2Ptr[2] = static_cast<cl_float>(p2.getZ());

	const Float3f &p3 = rect.getP3();
	cl_float *p3Ptr = reinterpret_cast<cl_float*>(recPtr + (sizeof(cl_float3) * 2));
	p3Ptr[0] = static_cast<cl_float>(p3.getX());
	p3Ptr[1] = static_cast<cl_float>(p3.getY());
	p3Ptr[2] = static_cast<cl_float>(p3.getZ());

	const Float3f p1p2 = p2 - p1;
	cl_float *p1p2Ptr = reinterpret_cast<cl_float*>(recPtr + (sizeof(cl_float3) * 3));
	p1p2Ptr[0] = static_cast<cl_float>(p1p2.getX());
	p1p2Ptr[1] = static_cast<cl_float>(p1p2.getY());
	p1p2Ptr[2] = static_cast<cl_float>(p1p2.getZ());

	const Float3f p2p3 = p3 - p2;
	cl_float *p2p3Ptr = reinterpret_cast<cl_float*>(recPtr + (sizeof(cl_float3) * 4));
	p2p3Ptr[0] = static_cast<cl_float>(p2p3.getX());
	p2p3Ptr[1] = static_cast<cl_float>(p2p3.getY());
	p2p3Ptr[2] = static_cast<cl_float>(p2p3.getZ());

	const Float3f normal = rect.getNormal();
	cl_float *normalPtr = reinterpret_cast<cl_float*>(recPtr + (sizeof(cl_float3) * 5));
	normalPtr[0] = static_cast<cl_float>(normal.getX());
	normalPtr[1] = static_cast<cl_float>(normal.getY());
	normalPtr[2] = static_cast<cl_float>(normal.getZ());

	// Texture reference data.
	cl_int *offsetPtr = reinterpret_cast<cl_int*>(recPtr + (sizeof(cl_float3) * 6));
	offsetPtr[0] = static_cast<cl_int>(textureRef.offset);

	cl_short *dimPtr = reinterpret_cast<cl_short*>(recPtr +
		(sizeof(cl_float3) * 6) + sizeof(cl_int));
	dimPtr[0] = static_cast<cl_short>(textureRef.width);
	dimPtr[1] = static_cast<cl_short>(textureRef.height);
}

void CLProgram::writeLight(const Light &light, const OwnerReference &ownerRef, 
	std::vector<cl_char> &buffer, size_t byteOffset) const
{
	cl_char *dataPtr = buffer.data() + byteOffset;

	const Float3d &point = light.getPoint();
	cl_float *pointPtr = reinterpret_cast<cl_float*>(dataPtr);
	pointPtr[0] = static_cast<cl_float>(point.getX());
	pointPtr[1] = static_cast<cl_float>(point.getY());
	pointPtr[2] = static_cast<cl_float>(point.getZ());

	const Float3d &color = light.getColor();
	cl_float *colorPtr = reinterpret_cast<cl_float*>(dataPtr + sizeof(cl_float3));
	colorPtr[0] = static_cast<cl_float>(color.getX());
	colorPtr[1] = static_cast<cl_float>(color.getY());
	colorPtr[2] = static_cast<cl_float>(color.getZ());

	cl_int *ownerPtr = reinterpret_cast<cl_int*>(dataPtr + (sizeof(cl_float3) * 2));
	ownerPtr[0] = static_cast<cl_int>(ownerRef.offset);
	ownerPtr[1] = static_cast<cl_int>(ownerRef.count);

	const double intensity = light.getIntensity();
	cl_float *intensityPtr = reinterpret_cast<cl_float*>(dataPtr + 
		(sizeof(cl_float3) * 2) + SIZEOF_OWNER_REF);
	intensityPtr[0] = static_cast<cl_float>(intensity);
}

void CLProgram::updateCamera(const Float3d &eye, const Float3d &direction, double fovY)
{
	// Do not scale the direction beforehand.
	assert(direction.isNormalized());

	std::vector<cl_char> buffer(SIZEOF_CAMERA);
	cl_char *bufPtr = buffer.data();

	// Write the components of the camera to the local buffer.
	// Correct spacing is very important.
	auto *eyePtr = reinterpret_cast<cl_float*>(bufPtr);
	eyePtr[0] = static_cast<cl_float>(eye.getX());
	eyePtr[1] = static_cast<cl_float>(eye.getY());
	eyePtr[2] = static_cast<cl_float>(eye.getZ());

	auto *forwardPtr = reinterpret_cast<cl_float*>(bufPtr + sizeof(cl_float3));
	forwardPtr[0] = static_cast<cl_float>(direction.getX());
	forwardPtr[1] = static_cast<cl_float>(direction.getY());
	forwardPtr[2] = static_cast<cl_float>(direction.getZ());

	auto right = direction.cross(Directable::getGlobalUp()).normalized();
	auto *rightPtr = reinterpret_cast<cl_float*>(bufPtr + (sizeof(cl_float3) * 2));
	rightPtr[0] = static_cast<cl_float>(right.getX());
	rightPtr[1] = static_cast<cl_float>(right.getY());
	rightPtr[2] = static_cast<cl_float>(right.getZ());

	auto up = right.cross(direction).normalized();
	auto *upPtr = reinterpret_cast<cl_float*>(bufPtr + (sizeof(cl_float3) * 3));
	upPtr[0] = static_cast<cl_float>(up.getX());
	upPtr[1] = static_cast<cl_float>(up.getY());
	upPtr[2] = static_cast<cl_float>(up.getZ());

	// Zoom is a function of field of view.
	double zoom = 1.0 / std::tan(fovY * 0.5 * DEG_TO_RAD);
	auto *zoomPtr = reinterpret_cast<cl_float*>(bufPtr + (sizeof(cl_float3) * 4));
	zoomPtr[0] = static_cast<cl_float>(zoom);

	// Write the buffer to device memory.
	cl_int status = this->commandQueue.enqueueWriteBuffer(this->cameraBuffer,
		CL_TRUE, 0, buffer.size(), static_cast<const void*>(bufPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer updateCamera");
}

void CLProgram::updateGameTime(double gameTime)
{
	assert(gameTime >= 0.0);

	std::vector<cl_char> buffer(sizeof(cl_float));
	cl_char *bufPtr = buffer.data();

	auto *timePtr = reinterpret_cast<cl_float*>(bufPtr);
	timePtr[0] = static_cast<cl_float>(gameTime);

	// Write the buffer to device memory.
	cl_int status = this->commandQueue.enqueueWriteBuffer(this->gameTimeBuffer,
		CL_TRUE, 0, buffer.size(), static_cast<const void*>(bufPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer updateGameTime");
}

int CLProgram::addTexture(uint32_t *pixels, int width, int height)
{
	assert(pixels != nullptr);
	assert(width > 0);
	assert(height > 0);

	std::vector<cl_char> buffer(sizeof(cl_float4) * width * height);
	cl_char *bufPtr = buffer.data();

	// Convert each pixel to float4 format (16 bytes per pixel); fast for rendering 
	// at the expense of memory.
	const int pixelCount = width * height;
	for (int i = 0; i < pixelCount; ++i)
	{
		// Input pixel format is ARGB8888.
		const uint32_t pixel = pixels[i];
		const uint8_t a = static_cast<uint8_t>(pixel >> 24);
		const uint8_t r = static_cast<uint8_t>(pixel >> 16);
		const uint8_t g = static_cast<uint8_t>(pixel >> 8);
		const uint8_t b = static_cast<uint8_t>(pixel);

		// The kernel uses RGBA format, so move the components accordingly.
		cl_float *pixPtr = reinterpret_cast<cl_float*>(bufPtr + (sizeof(cl_float4) * i));
		pixPtr[0] = static_cast<cl_float>(static_cast<float>(r) / 255.0f);
		pixPtr[1] = static_cast<cl_float>(static_cast<float>(g) / 255.0f);
		pixPtr[2] = static_cast<cl_float>(static_cast<float>(b) / 255.0f);
		pixPtr[3] = static_cast<cl_float>(static_cast<float>(a) / 255.0f);
	}

	// Number of pixels to skip to access the new texture when rendering.
	const int pixelOffset = [this]()
	{
		int offset = 0;
		for (const auto &textureRef : this->textureRefs)
		{
			offset += textureRef.width * textureRef.height;
		}

		return offset;
	}();

	// Offset in texture memory where the new texture should be written.
	const size_t byteOffset = sizeof(cl_float4) * pixelOffset;

	// Size of the existing texture buffer in bytes.
	cl_int status = CL_SUCCESS;
	const size_t textureBufferSize = this->textureBuffer.getInfo<CL_MEM_SIZE>(&status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer::getInfo addTexture.");

	// Minimum size of texture memory required including the new texture.
	const size_t requiredSize = byteOffset + buffer.size();

	// See if the texture buffer needs to be resized.
	if (requiredSize > textureBufferSize)
	{
		const size_t newSize = [requiredSize, textureBufferSize]()
		{
			// Make sure size is at least 1 before doubling.
			size_t size = std::max(textureBufferSize, static_cast<size_t>(1));
			while (size < requiredSize)
			{
				size *= 2;
			}
			return size;
		}();

		Debug::mention("CLProgram", "Resizing texture memory from " +
			std::to_string(textureBufferSize) + " to " + std::to_string(newSize) + " bytes.");

		// Resize the texture buffer to the new size.
		this->resizeBuffer(this->textureBuffer, newSize);

		// Tell the kernels where to find the new texture buffer.
		status = this->intersectKernel.setArg(4, this->textureBuffer);
		Debug::check(status == CL_SUCCESS, "CLProgram",
			"cl::Kernel::setArg addTexture intersectKernel.");

		status = this->rayTraceKernel.setArg(6, this->textureBuffer);
		Debug::check(status == CL_SUCCESS, "CLProgram",
			"cl::Kernel::setArg addTexture rayTraceKernel.");
	}

	// Write the new texture into device memory.
	status = this->commandQueue.enqueueWriteBuffer(this->textureBuffer, CL_TRUE,
		byteOffset, buffer.size(), static_cast<const void*>(bufPtr), nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer addTexture.");

	// Add a new texture reference so rectangles can refer to this texture.
	this->textureRefs.push_back(TextureReference(pixelOffset, width, height));

	// Return the index to the texture reference just added.
	return static_cast<int>(this->textureRefs.size() - 1);
}

void CLProgram::queueVoxelRefUpdate(int x, int y, int z, int offset, int count)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);
	assert(x < this->worldWidth);
	assert(y < this->worldHeight);
	assert(z < this->worldDepth);
	assert(offset >= 0);
	assert(count >= 0);

	const Int3 voxel(x, y, z);

	// Refresh the voxel reference queue for this voxel.
	auto queueIter = this->voxelRefQueue.find(voxel);
	if (queueIter != this->voxelRefQueue.end())
	{
		queueIter->second = VoxelReference(offset, count);
	}
	else
	{
		this->voxelRefQueue.emplace(std::make_pair(
			voxel, VoxelReference(offset, count)));
	}
}

void CLProgram::queueSpriteRefUpdate(int x, int y, int z, int offset, int count)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);
	assert(x < this->worldWidth);
	assert(y < this->worldHeight);
	assert(z < this->worldDepth);
	assert(offset >= 0);
	assert(count >= 0);

	const Int3 voxel(x, y, z);

	// Refresh the sprite reference queue for this voxel.
	auto queueIter = this->spriteRefQueue.find(voxel);
	if (queueIter != this->spriteRefQueue.end())
	{
		queueIter->second = SpriteReference(offset, count);
	}
	else
	{
		this->spriteRefQueue.emplace(std::make_pair(
			voxel, SpriteReference(offset, count)));
	}
}

void CLProgram::queueLightRefUpdate(int x, int y, int z, int offset, int count)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);
	assert(x < this->worldWidth);
	assert(y < this->worldHeight);
	assert(z < this->worldDepth);
	assert(offset >= 0);
	assert(count >= 0);

	const Int3 voxel(x, y, z);

	// Refresh the light reference queue for this voxel.
	auto queueIter = this->lightRefQueue.find(voxel);
	if (queueIter != this->lightRefQueue.end())
	{
		queueIter->second = LightReference(offset, count);
	}
	else
	{
		this->lightRefQueue.emplace(std::make_pair(
			voxel, LightReference(offset, count)));
	}
}

void CLProgram::queueVoxelUpdate(int x, int y, int z, 
	const std::vector<Rect3D> &rects, const std::vector<int> &textureIndices)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);
	assert(x < this->worldWidth);
	assert(y < this->worldHeight);
	assert(z < this->worldDepth);
	assert(textureIndices.size() == rects.size());

	const Int3 voxel(x, y, z);

	// Free the existing allocation for the voxel if there is one.
	auto offsetIter = this->voxelOffsets.find(voxel);
	if (offsetIter != this->voxelOffsets.end())
	{
		const size_t offset = offsetIter->second;
		this->rectBufferView->deallocate(offset);
	}

	// If the new voxel has no rectangles, don't allocate anything. This means that
	// the previous rectangles are untouched but are now considered garbage because
	// no voxel reference should be pointing to them.
	if (rects.size() > 0)
	{
		const size_t bufferSize = SIZEOF_RECTANGLE * rects.size();

		// Refresh the voxel queue at this voxel.
		auto queueIter = this->voxelQueue.find(voxel);
		if (queueIter == this->voxelQueue.end())
		{
			// Add a new voxel mapping for the queue.
			queueIter = this->voxelQueue.emplace(std::make_pair(
				voxel, std::vector<RectData>())).first;
		}

		auto &workItem = queueIter->second;
		workItem.clear();
		
		for (size_t i = 0; i < rects.size(); ++i)
		{
			const Rect3D &rect = rects.at(i);
			const int textureID = textureIndices.at(i);
			
			workItem.push_back(RectData(rect, textureID));
		}

		// Allocate a region for the voxel's new rectangles.
		const cl::size_type offset = this->rectBufferView->allocate(bufferSize);

		// Make a new voxel offset mapping if one doesn't exist.
		if (offsetIter == this->voxelOffsets.end())
		{
			offsetIter = this->voxelOffsets.emplace(std::make_pair(
				voxel, offset)).first;
		}
		else
		{
			offsetIter->second = offset;
		}

		// Size of the existing rectangle buffer.
		cl_int status = CL_SUCCESS;
		const cl::size_type rectBufferSize = this->rectangleBuffer.getInfo<CL_MEM_SIZE>(&status);
		Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer::getInfo updateVoxel.");

		// Minimum size of geometry memory required for the new voxel geometry.
		const cl::size_type requiredSize = offset + bufferSize;

		if (requiredSize > rectBufferSize)
		{
			// Resize the rectangle buffer and reset its associated kernel arguments.
			this->resizeRectBuffer(requiredSize);
		}

		// Update the voxel reference at the given coordinate. The offset should never
		// be a fraction because only rectangles are allocated in the buffer view.
		this->queueVoxelRefUpdate(x, y, z, 
			static_cast<int>(offset / SIZEOF_RECTANGLE),
			static_cast<int>(rects.size()));
	}
	else
	{
		// No allocation is associated with the voxel reference, so erase the mapping.
		// (This could be cleaner; might not be necessary with different checks above).
		if (offsetIter != this->voxelOffsets.end())
		{
			this->voxelOffsets.erase(offsetIter);
		}

		// Set the voxel reference to empty.
		this->queueVoxelRefUpdate(x, y, z, 0, 0);
	}
}

void CLProgram::queueVoxelUpdate(int x, int y, int z,
	const std::vector<Rect3D> &rects, int textureIndex)
{
	// Blocks in Arena almost always share the same texture index on all sides, 
	// but there are some exceptions, so a vector of indices must be given. This
	// method is here for convenience.
	std::vector<int> textureIndices(rects.size());

	for (auto &index : textureIndices)
	{
		index = textureIndex;
	}

	this->queueVoxelUpdate(x, y, z, rects, textureIndices);
}

void CLProgram::addSpriteToVoxel(int spriteID, const Int3 &voxel)
{
	// Get the sprite group associated with the voxel and free the old sprite
	// group allocation if there is one.
	auto groupIter = this->spriteGroups.find(voxel);
	if (groupIter == this->spriteGroups.end())
	{
		// Add a mapping to a new group with a dummy byte offset of 0.
		groupIter = this->spriteGroups.emplace(std::make_pair(
			voxel, std::make_pair(0, std::vector<int>()))).first;
	}
	else
	{
		// Free the old sprite group allocation.
		const size_t offset = groupIter->second.first;
		this->rectBufferView->deallocate(offset);
	}

	std::vector<int> &spriteIDs = groupIter->second.second;

	// Add the new ID into the sprite group.
	spriteIDs.push_back(spriteID);

	// Refresh the sprite queue at this voxel.
	auto queueIter = this->spriteQueue.find(voxel);
	if (queueIter == this->spriteQueue.end())
	{
		// Add a new voxel mapping for the queue.
		queueIter = this->spriteQueue.emplace(std::make_pair(
			voxel, std::vector<RectData>())).first;
	}

	auto &workItem = queueIter->second;
	workItem.clear();

	for (const auto id : spriteIDs)
	{
		const auto &rectData = this->spriteData.at(id);
		const Rect3D &rect = rectData.getRect();
		const int textureID = rectData.getTextureID();

		workItem.push_back(RectData(rect, textureID));
	}

	// Allocate the new sprite group and get its byte offset in the rect buffer. 
	// The voxel is guaranteed to have at least one sprite, so there's no need to 
	// check for buffer.size() == 0 (unlike with voxel updating).
	const size_t bufferSize = SIZEOF_RECTANGLE * spriteIDs.size();
	size_t &offset = groupIter->second.first;
	offset = this->rectBufferView->allocate(bufferSize);

	// Size of the existing rectangle buffer.
	cl_int status = CL_SUCCESS;
	const cl::size_type rectBufferSize = this->rectangleBuffer.getInfo<CL_MEM_SIZE>(&status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer::getInfo addSpriteToVoxel.");

	// Minimum size of geometry memory required for the new sprite geometry.
	const cl::size_type requiredSize = offset + bufferSize;

	if (requiredSize > rectBufferSize)
	{
		// Resize the rectangle buffer and reset its associated kernel arguments.
		this->resizeRectBuffer(requiredSize);
	}

	// Update the sprite reference. The offset should never be a fraction because 
	// only rectangles are allocated in the buffer view.
	this->queueSpriteRefUpdate(voxel.getX(), voxel.getY(), voxel.getZ(),
		static_cast<int>(offset / SIZEOF_RECTANGLE),
		static_cast<int>(spriteIDs.size()));
}

void CLProgram::removeSpriteFromVoxel(int spriteID, const Int3 &voxel)
{
	// Get the list of sprite IDs associated with the voxel.
	const auto groupIter = this->spriteGroups.find(voxel);
	std::vector<int> &spriteIDs = groupIter->second.second;

	// Erase the sprite ID from the sprite group.
	const auto idIter = std::find(spriteIDs.begin(), spriteIDs.end(), spriteID);
	spriteIDs.erase(idIter);

	// Get the byte offset of the sprite group in the rect buffer and 
	// deallocate it.
	size_t &offset = groupIter->second.first;
	this->rectBufferView->deallocate(offset);

	// If there are still sprites in the voxel, reallocate the sprite group in the 
	// rect buffer (this isn't entirely necessary; it's just done for correctness
	// so a sprite group doesn't have unused memory).
	if (spriteIDs.size() > 0)
	{
		// Refresh the sprite queue at this voxel.
		auto queueIter = this->spriteQueue.find(voxel);
		if (queueIter == this->spriteQueue.end())
		{
			// Add a new voxel mapping for the queue.
			queueIter = this->spriteQueue.emplace(std::make_pair(
				voxel, std::vector<RectData>())).first;
		}

		auto &workItem = queueIter->second;
		workItem.clear();

		for (const auto id : spriteIDs)
		{
			const auto &rectData = this->spriteData.at(id);
			const Rect3D &rect = rectData.getRect();
			const int textureID = rectData.getTextureID();

			workItem.push_back(RectData(rect, textureID));
		}

		// Reallocate sprite group (no need to resize because the sprite group has shrunk;
		// if it doesn't find a better fit, it will find the deallocated space again).
		offset = this->rectBufferView->allocate(SIZEOF_RECTANGLE * spriteIDs.size());

		// Update the sprite reference. The offset should never be a fraction because 
		// only rectangles are allocated in the buffer view.
		this->queueSpriteRefUpdate(voxel.getX(), voxel.getY(), voxel.getZ(),
			static_cast<int>(offset / SIZEOF_RECTANGLE),
			static_cast<int>(spriteIDs.size()));
	}
	else
	{
		// No more sprites in the voxel, so erase the associated mapping.
		this->spriteGroups.erase(groupIter);

		// Set the sprite reference to zero.
		this->queueSpriteRefUpdate(voxel.getX(), voxel.getY(), voxel.getZ(), 0, 0);
	}
}

void CLProgram::updateSpriteInVoxel(int spriteID, const Int3 &voxel)
{
	// Get the sprite's rectangle and texture reference.
	const auto &rectData = this->spriteData.at(spriteID);
	const Rect3D &rect = rectData.getRect();
	const TextureReference &textureRef = this->textureRefs.at(rectData.getTextureID());

	// Get the sprite group for the voxel.
	const auto &spriteGroup = this->spriteGroups.at(voxel);
	const std::vector<int> &spriteIDs = spriteGroup.second;

	// Refresh the sprite queue at this voxel.
	// (Later, this code and pushUpdates() should calculate only necessary changes).
	auto queueIter = this->spriteQueue.find(voxel);
	if (queueIter == this->spriteQueue.end())
	{
		queueIter = this->spriteQueue.emplace(std::make_pair(
			voxel, std::vector<RectData>())).first;
	}

	auto &workItem = queueIter->second;
	workItem.clear();

	for (const auto id : spriteIDs)
	{
		const auto &rectData = this->spriteData.at(id);
		const Rect3D &rect = rectData.getRect();
		const int textureID = rectData.getTextureID();

		workItem.push_back(RectData(rect, textureID));
	}
}

void CLProgram::addLightToVoxel(int lightID, const Int3 &voxel)
{
	// Similar to addSpriteToVoxel()...
	// Owner buffer... buffer view...
	Debug::crash("CLProgram", "addLightToVoxel() not implemented.");
}

void CLProgram::removeLightFromVoxel(int lightID, const Int3 &voxel)
{
	// Similar to removeSpriteFromVoxel()...
	// Owner buffer... buffer view...
	Debug::crash("CLProgram", "removeLightFromVoxel() not implemented.");
}

void CLProgram::updateLightInVoxel(int lightID, const Int3 &voxel)
{
	// Similar to updateSpriteInVoxel()...
	// Get lightOwners mapping... refresh owner list based on sprite's touched voxels
	// if light has an owner...
	
	// This method can't tell whether its owner sprite (if it exists) has moved or not, 
	// but if this light is being updated, then its sprite has most likely moved.

	Debug::crash("CLProgram", "updateLightInVoxel() not implemented.");
}

void CLProgram::queueSpriteUpdate(int spriteID, const Rect3D &rect, int textureIndex)
{
	// See if the sprite already has a mapping for it.
	const auto rectIter = this->spriteData.find(spriteID);
	if (rectIter != this->spriteData.end())
	{
		// Sprite mapping exists, so overwrite its data.
		rectIter->second = RectData(rect, textureIndex);

		// Get previously touched voxels.
		const auto touchedIter = this->spriteTouchedVoxels.find(spriteID);
		const std::vector<Int3> &prevTouchedVoxels = touchedIter->second;

		// Get new touched voxels.
		std::vector<Int3> newTouchedVoxels = this->getTouchedVoxels(rect, true);

		// Update the rect and index for voxels that are in both lists.
		for (const auto &voxel : prevTouchedVoxels)
		{
			const auto newVoxelsIter = std::find(
				newTouchedVoxels.begin(), newTouchedVoxels.end(), voxel);

			if (newVoxelsIter != newTouchedVoxels.end())
			{
				this->updateSpriteInVoxel(spriteID, voxel);
			}
		}

		// Remove the sprite from voxels it's no longer touching.
		for (const auto &voxel : prevTouchedVoxels)
		{
			const auto newVoxelsIter = std::find(
				newTouchedVoxels.begin(), newTouchedVoxels.end(), voxel);

			if (newVoxelsIter == newTouchedVoxels.end())
			{
				this->removeSpriteFromVoxel(spriteID, voxel);
			}
		}

		// Add the sprite to voxels it's now touching.
		for (const auto &voxel : newTouchedVoxels)
		{
			const auto oldVoxelsIter = std::find(
				prevTouchedVoxels.begin(), prevTouchedVoxels.end(), voxel);

			if (oldVoxelsIter == prevTouchedVoxels.end())
			{
				this->addSpriteToVoxel(spriteID, voxel);
			}
		}

		// Overwrite the sprite's touched voxels.
		touchedIter->second = newTouchedVoxels;
	}
	else
	{
		// No sprite mapping exists, so insert new sprite data.
		this->spriteData.emplace(std::make_pair(
			spriteID, RectData(rect, textureIndex)));

		// Add the voxels that the sprite touches.
		const auto touchedIter = this->spriteTouchedVoxels.emplace(std::make_pair(
			spriteID, this->getTouchedVoxels(rect, true))).first;
		const std::vector<Int3> &touchedVoxels = touchedIter->second;

		// Add the sprite to each voxel it's touching.
		for (const auto &voxel : touchedVoxels)
		{
			this->addSpriteToVoxel(spriteID, voxel);
		}
	}
}

void CLProgram::queueSpriteRemoval(int spriteID)
{
	// Remove the sprite's rectangle + texture ID mapping.
	const auto dataIter = this->spriteData.find(spriteID);
	this->spriteData.erase(dataIter);

	// Get the voxels touched by the sprite.
	const auto touchedIter = this->spriteTouchedVoxels.find(spriteID);
	const std::vector<Int3> &touchedVoxels = touchedIter->second;

	// Remove the sprite from each voxel it touches.
	for (const auto &voxel : touchedVoxels)
	{
		this->removeSpriteFromVoxel(spriteID, voxel);
	}

	// Remove the sprite's touched voxels mapping.
	this->spriteTouchedVoxels.erase(touchedIter);
}

void CLProgram::queueLightUpdate(int lightID, const Float3d &point, const Float3d &color, 
	const int *ownerID, double intensity)
{
	// See if the light already has a mapping for it.
	const auto lightIter = this->lightData.find(lightID);
	if (lightIter != this->lightData.end())
	{
		// Light mapping exists, so overwrite its data.
		lightIter->second = Light(point, color, intensity);

		// If the new and old owner IDs don't match, refresh the mapping.
		const auto ownerIter = this->lightOwners.find(lightID);
		if (ownerIter != this->lightOwners.end())
		{
			// A mapping exists. See if the owner ID is not null.
			if (ownerID != nullptr)
			{
				// Overwrite the old owner ID.
				ownerIter->second = *ownerID;
			}
			else
			{
				// Erase the old owner ID.
				this->lightOwners.erase(ownerIter);
			}
		}
		else
		{
			// No mapping exists. Only add one if the new owner ID is not null.
			if (ownerID != nullptr)
			{
				this->lightOwners.emplace(std::make_pair(lightID, *ownerID)).first;
			}
		}

		// Get previously touched voxels.
		const auto touchedIter = this->lightTouchedVoxels.find(lightID);
		const std::vector<Int3> &prevTouchedVoxels = touchedIter->second;

		// Get new touched voxels (those within the light's range).
		std::vector<Int3> newTouchedVoxels = this->getTouchedVoxels(point, intensity, true);

		// Update the light for voxels that are in both lists.
		for (const auto &voxel : prevTouchedVoxels)
		{
			const auto newVoxelsIter = std::find(
				newTouchedVoxels.begin(), newTouchedVoxels.end(), voxel);

			if (newVoxelsIter != newTouchedVoxels.end())
			{
				this->updateLightInVoxel(lightID, voxel);
			}
		}

		// Remove the light from voxels it's no longer touching.
		for (const auto &voxel : prevTouchedVoxels)
		{
			const auto newVoxelsIter = std::find(
				newTouchedVoxels.begin(), newTouchedVoxels.end(), voxel);

			if (newVoxelsIter == newTouchedVoxels.end())
			{
				this->removeLightFromVoxel(lightID, voxel);
			}
		}

		// Add the light to voxels it's now touching.
		for (const auto &voxel : newTouchedVoxels)
		{
			const auto oldVoxelsIter = std::find(
				prevTouchedVoxels.begin(), prevTouchedVoxels.end(), voxel);

			if (oldVoxelsIter == prevTouchedVoxels.end())
			{
				this->addLightToVoxel(lightID, voxel);
			}
		}

		// Overwrite the light's touched voxels.
		touchedIter->second = newTouchedVoxels;
	}
	else
	{
		// No light mapping exists, so insert new light data.
		this->lightData.emplace(std::make_pair(
			lightID, Light(point, color, intensity)));

		// Add all the voxels the light reaches.
		const auto touchedIter = this->lightTouchedVoxels.emplace(std::make_pair(
			lightID, this->getTouchedVoxels(point, intensity, true))).first;
		const std::vector<Int3> &touchedVoxels = touchedIter->second;

		// Add the light to each voxel it's touching.
		for (const auto &voxel : touchedVoxels)
		{
			this->addLightToVoxel(lightID, voxel);
		}
	}
}

void CLProgram::queueLightRemoval(int lightID)
{
	// Remove the light's mapping.
	const auto dataIter = this->lightData.find(lightID);
	this->lightData.erase(dataIter);

	// Get the voxels touched by the light.
	const auto touchedIter = this->lightTouchedVoxels.find(lightID);
	const std::vector<Int3> &touchedVoxels = touchedIter->second;

	// Remove the light from each voxel it touches.
	for (const auto &voxel : touchedVoxels)
	{
		this->removeLightFromVoxel(lightID, voxel);
	}

	// Remove the light's touched voxels mapping.
	this->lightTouchedVoxels.erase(touchedIter);
}

void CLProgram::pushUpdates()
{
	// Buffers with offsets for asynchronous writes to device memory.
	std::vector<std::pair<size_t, std::vector<cl_char>>> rectBuffers, lightBuffers,
		ownerBuffers, voxelRefBuffers, spriteRefBuffers, lightRefBuffers;

	// Lambda for adding a rectangle buffer to the queue.
	auto addRectBuffer = [this, &rectBuffers](
		const std::vector<RectData> &rectData, size_t offset)
	{
		std::vector<cl_char> buffer(SIZEOF_RECTANGLE * rectData.size());

		for (size_t i = 0; i < rectData.size(); ++i)
		{
			const auto &pair = rectData.at(i);
			const Rect3D &rect = pair.getRect();
			const TextureReference &textureRef = this->textureRefs.at(pair.getTextureID());
			this->writeRect(rect, textureRef, buffer, SIZEOF_RECTANGLE * i);
		}

		rectBuffers.push_back(std::make_pair(offset, std::move(buffer)));
	};

	// Lambda for adding a light buffer to the queue.
	auto addLightBuffer = [this, &lightBuffers](
		const std::vector<LightData> &lightData, size_t offset)
	{
		std::vector<cl_char> buffer(SIZEOF_LIGHT * lightData.size());

		for (size_t i = 0; i < lightData.size(); ++i)
		{
			const auto &pair = lightData.at(i);
			const Light &light = pair.getLight();
			const OwnerReference &ownerRef = pair.getOwnerRef();
			this->writeLight(light, ownerRef, buffer, SIZEOF_LIGHT * i);
		}

		lightBuffers.push_back(std::make_pair(offset, std::move(buffer)));
	};

	// Lambda for adding an owner buffer to the queue.
	auto addOwnerBuffer = [this, &ownerBuffers](
		const std::vector<int> &ownerData, size_t offset)
	{
		std::vector<cl_char> buffer(sizeof(cl_int) * ownerData.size());

		for (size_t i = 0; i < ownerData.size(); ++i)
		{
			const size_t offset = sizeof(cl_int) * i;
			cl_int *dataPtr = reinterpret_cast<cl_int*>(buffer.data() + offset);
			dataPtr[0] = ownerData.at(i);
		}

		ownerBuffers.push_back(std::make_pair(offset, std::move(buffer)));
	};

	// Lambda for adding a voxel/sprite/light reference buffer to the queue.
	auto addRefBuffer = [this](const Int3 &voxel, int offset, int count, size_t sizeOfRef, 
		std::vector<std::pair<size_t, std::vector<cl_char>>> &dstBuffers)
	{
		std::vector<cl_char> buffer(sizeOfRef);

		cl_int *offPtr = reinterpret_cast<cl_int*>(buffer.data());
		offPtr[0] = static_cast<cl_int>(offset);
		offPtr[1] = static_cast<cl_int>(count);

		const size_t bufferOffset = (voxel.getX() * sizeOfRef) +
			((voxel.getY() * this->worldWidth) * sizeOfRef) +
			((voxel.getZ() * this->worldWidth * this->worldHeight) * sizeOfRef);
		dstBuffers.push_back(std::make_pair(bufferOffset, std::move(buffer)));
	};

	// Add voxel queue buffers.
	for (const auto &group : this->voxelQueue)
	{
		const Int3 &voxel = group.first;
		const auto &rectData = group.second;
		const size_t byteOffset = this->voxelOffsets.at(voxel);
		addRectBuffer(rectData, byteOffset);
	}

	// Add sprite queue buffers.
	for (const auto &group : this->spriteQueue)
	{
		const Int3 &voxel = group.first;
		const auto &rectData = group.second;
		const size_t byteOffset = this->spriteGroups.at(voxel).first;
		addRectBuffer(rectData, byteOffset);
	}

	// Add light queue buffers.
	for (const auto &group : this->lightQueue)
	{
		const Int3 &voxel = group.first;
		const auto &lightData = group.second;
		const size_t byteOffset = this->lightGroups.at(voxel).first;
		addLightBuffer(lightData, byteOffset);
	}

	// Add owner queue buffers.
	for (const auto &group : this->ownerQueue)
	{
		const int lightID = group.first;
		const auto &ownerData = group.second;
		const size_t byteOffset = this->ownerGroups.at(lightID).first;
		addOwnerBuffer(ownerData, byteOffset);
	}

	// Add voxel reference queue buffers.
	for (const auto &group : this->voxelRefQueue)
	{
		const Int3 &voxel = group.first;
		const int offset = group.second.offset;
		const int count = group.second.count;
		addRefBuffer(voxel, offset, count, SIZEOF_VOXEL_REF, voxelRefBuffers);
	}

	// Add sprite reference queue buffers.
	for (const auto &group : this->spriteRefQueue)
	{
		const Int3 &voxel = group.first;
		const int offset = group.second.offset;
		const int count = group.second.count;
		addRefBuffer(voxel, offset, count, SIZEOF_SPRITE_REF, spriteRefBuffers);
	}

	// Add light reference queue buffers.
	for (const auto &group : this->lightRefQueue)
	{
		const Int3 &voxel = group.first;
		const int offset = group.second.offset;
		const int count = group.second.count;
		addRefBuffer(voxel, offset, count, SIZEOF_LIGHT_REF, lightRefBuffers);
	}

	// Lambda for asynchronously writing queue buffers to an OpenCL buffer.
	auto enqueueAsyncWrite = [this](
		const std::vector<std::pair<size_t, std::vector<cl_char>>> &buffers,
		cl::Buffer &dstBuffer)
	{
		for (const auto &pair : buffers)
		{
			const size_t offset = pair.first;
			const std::vector<cl_char> &buffer = pair.second;
			this->commandQueue.enqueueWriteBuffer(dstBuffer, CL_FALSE,
				offset, buffer.size(), static_cast<const void*>(buffer.data()),
				nullptr, nullptr);
		}
	};

	// Begin asynchronous writes to various buffers.
	enqueueAsyncWrite(rectBuffers, this->rectangleBuffer);
	enqueueAsyncWrite(lightBuffers, this->lightBuffer);
	enqueueAsyncWrite(ownerBuffers, this->ownerBuffer);
	enqueueAsyncWrite(voxelRefBuffers, this->voxelRefBuffer);
	enqueueAsyncWrite(spriteRefBuffers, this->spriteRefBuffer);
	enqueueAsyncWrite(lightRefBuffers, this->lightRefBuffer);

	// Erase the queues so they're empty for the next frame.
	this->voxelQueue.clear();
	this->spriteQueue.clear();
	this->lightQueue.clear();
	this->ownerQueue.clear();
	this->voxelRefQueue.clear();
	this->spriteRefQueue.clear();
	this->lightRefQueue.clear();

	// Make sure all writes have finished before continuing.
	cl_int status = this->commandQueue.finish();
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"pushUpdates(), " + this->getErrorString(status));
}

const void *CLProgram::render()
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
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::CommandQueue::enqueueReadBuffer " + this->getErrorString(status) + ".");

	return outputDataPtr;
}
