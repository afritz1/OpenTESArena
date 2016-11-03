#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "CLProgram.h"

#include "TextureReference.h"
#include "../Entities/Directable.h"
#include "../Math/Constants.h"
#include "../Math/Float3.h"
#include "../Math/Int2.h"
#include "../Math/Rect3D.h"
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

	// Some arbitrary limits until the code is more advanced.
	const int MAX_RECTS_PER_VOXEL = 6;
}

const std::string CLProgram::PATH = "data/kernels/";
const std::string CLProgram::FILENAME = "kernel.cl";
const std::string CLProgram::INTERSECT_KERNEL = "intersect";
const std::string CLProgram::RAY_TRACE_KERNEL = "rayTrace";
const std::string CLProgram::POST_PROCESS_KERNEL = "postProcess";
const std::string CLProgram::CONVERT_TO_RGB_KERNEL = "convertToRGB";

CLProgram::CLProgram(int worldWidth, int worldHeight, int worldDepth,
	Renderer &renderer, double resolutionScale)
{
	assert(worldWidth > 0);
	assert(worldHeight > 0);
	assert(worldDepth > 0);

	Debug::mention("CLProgram", "Initializing.");

	const int screenWidth = renderer.getWindowDimensions().getX();
	const int screenHeight = renderer.getWindowDimensions().getY();

	// Render dimensions for ray tracing. To prevent issues when the user shrinks
	// the window down too far, clamp them to at least 1.
	this->renderWidth = std::max(static_cast<int>(screenWidth * resolutionScale), 1);
	this->renderHeight = std::max(static_cast<int>(screenHeight * resolutionScale), 1);

	this->worldWidth = worldWidth;
	this->worldHeight = worldHeight;
	this->worldDepth = worldDepth;

	// Create the local output pixel buffer.
	const int renderPixelCount = this->renderWidth * this->renderHeight;
	this->outputData = std::vector<cl_char>(sizeof(cl_int) * renderPixelCount);

	// Create streaming texture to be used as the game world frame buffer.	
	this->texture = renderer.createTexture(Renderer::DEFAULT_PIXELFORMAT,
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
		SIZEOF_RECTANGLE * MAX_RECTS_PER_VOXEL * worldWidth * worldHeight * worldDepth
		/* This buffer size is actually very naive. Much of it will just be air.
		Make a mapping of 3D cell coordinates to rect counts. Buffer resizing will
		also need to be figured out. */,
		nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer rectangleBuffer.");

	this->lightBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		SIZEOF_LIGHT /* Some # of lights * world dims, Placeholder size */, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer lightBuffer.");

	this->textureBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
		static_cast<cl::size_type>(1 << 16) /* 65536 bytes; resized as necessary. */,
		nullptr, &status);
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
}

CLProgram::~CLProgram()
{
	// Destroy the game world frame buffer.
	// The SDL_Renderer destroys this itself with SDL_DestroyRenderer(), too.
	SDL_DestroyTexture(this->texture);
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

void CLProgram::resize(Renderer &renderer, double resolutionScale)
{
	// Since the CL program is tightly coupled with the render resolution, nearly all
	// of the memory objects need to be refreshed. However, buffers with world data
	// just need to be referenced by the new kernels.

	// I don't really like this solution because it's basically 3/4 of the CLProgram
	// constructor, but I couldn't do this cleanly using the move assignment operator. 
	// There's probably a better way to do this, though.

	const int screenWidth = renderer.getWindowDimensions().getX();
	const int screenHeight = renderer.getWindowDimensions().getY();

	this->renderWidth = std::max(static_cast<int>(screenWidth * resolutionScale), 1);
	this->renderHeight = std::max(static_cast<int>(screenHeight * resolutionScale), 1);

	// Recreate the local output pixel buffer.
	const int renderPixelCount = this->renderWidth * this->renderHeight;
	this->outputData = std::vector<cl_char>(sizeof(cl_int) * renderPixelCount);

	// Recreate streaming texture for the frame buffer.
	SDL_DestroyTexture(this->texture);
	this->texture = renderer.createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STREAMING, this->renderWidth, this->renderHeight);
	Debug::check(this->texture != nullptr, "CLProgram", "resize SDL_CreateTexture");

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

	status = this->rayTraceKernel.setArg(5, this->textureBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize textureBuffer.");

	status = this->rayTraceKernel.setArg(6, this->gameTimeBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize gameTimeBuffer.");

	status = this->rayTraceKernel.setArg(7, this->depthBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize depthBuffer.");

	status = this->rayTraceKernel.setArg(8, this->normalBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize normalBuffer.");

	status = this->rayTraceKernel.setArg(9, this->viewBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize viewBuffer.");

	status = this->rayTraceKernel.setArg(10, this->pointBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize pointBuffer.");

	status = this->rayTraceKernel.setArg(11, this->uvBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize uvBuffer.");

	status = this->rayTraceKernel.setArg(12, this->rectangleIndexBuffer);
	Debug::check(status == CL_SUCCESS, "CLProgram",
		"cl::Kernel::setArg rayTraceKernel resize rectangleIndexBuffer.");

	status = this->rayTraceKernel.setArg(13, this->colorBuffer);
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
		for (auto &textureRef : this->textureRefs)
		{
			offset += textureRef.getWidth() * textureRef.getHeight();
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

		// Read the existing texture memory into a temp data buffer.
		std::vector<cl_char> oldData(textureBufferSize);
		status = this->commandQueue.enqueueReadBuffer(this->textureBuffer, CL_TRUE, 0,
			textureBufferSize, static_cast<void*>(oldData.data()), nullptr, nullptr);
		Debug::check(status == CL_SUCCESS, "CLProgram",
			"cl::enqueueReadBuffer addTexture oldData.");

		// Allocate a new buffer in device memory, erasing the old one.
		this->textureBuffer = cl::Buffer(this->context, CL_MEM_READ_ONLY,
			newSize, nullptr, &status);
		Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer addTexture.");

		// Write the old data into the bigger texture buffer.
		status = this->commandQueue.enqueueWriteBuffer(this->textureBuffer, CL_TRUE, 0,
			textureBufferSize, static_cast<const void*>(oldData.data()), nullptr, nullptr);
		Debug::check(status == CL_SUCCESS, "CLProgram",
			"cl::enqueueWriteBuffer addTexture oldData.");

		// Tell the kernels where to find the new texture buffer.
		status = this->intersectKernel.setArg(4, this->textureBuffer);
		Debug::check(status == CL_SUCCESS, "CLProgram",
			"cl::Kernel::setArg addTexture intersectKernel.");

		status = this->rayTraceKernel.setArg(5, this->textureBuffer);
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

void CLProgram::updateVoxelRef(int x, int y, int z, int count)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);
	assert(x < this->worldWidth);
	assert(y < this->worldHeight);
	assert(z < this->worldDepth);
	assert(count >= 0);

	// Up to 6 rectangles may currently be in a voxel.
	assert(count <= MAX_RECTS_PER_VOXEL);

	std::vector<cl_char> buffer(SIZEOF_VOXEL_REF);
	cl_char *bufPtr = buffer.data();

	// Offset is the number of rectangles to skip. Currently relative to naive 
	// rectangle buffer size.
	const int offset = MAX_RECTS_PER_VOXEL * (x + (y * this->worldWidth) +
		(z * this->worldWidth * this->worldHeight));

	cl_int *offPtr = reinterpret_cast<cl_int*>(bufPtr);
	offPtr[0] = static_cast<cl_int>(offset);
	offPtr[1] = static_cast<cl_int>(count);

	// Write the buffer to device memory.
	const cl::size_type bufferOffset = (x * SIZEOF_VOXEL_REF) +
		((y * this->worldWidth) * SIZEOF_VOXEL_REF) +
		((z * this->worldWidth * this->worldHeight) * SIZEOF_VOXEL_REF);
	cl_int status = this->commandQueue.enqueueWriteBuffer(this->voxelRefBuffer,
		CL_TRUE, bufferOffset, buffer.size(), static_cast<const void*>(bufPtr),
		nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer updateVoxelRef");
}

void CLProgram::updateVoxel(int x, int y, int z, const std::vector<Rect3D> &rects,
	const std::vector<int> &textureIndices)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);
	assert(x < this->worldWidth);
	assert(y < this->worldHeight);
	assert(z < this->worldDepth);

	// Up to 6 rectangles may currently be in a voxel.
	assert(rects.size() <= MAX_RECTS_PER_VOXEL);
	assert(textureIndices.size() == rects.size());

	// No need to zero out the buffer in case of there being fewer than the max 
	// rectangles given since the voxel reference will never allow the device to 
	// read garbage rectangles.
	std::vector<cl_char> buffer(SIZEOF_RECTANGLE * rects.size());
	cl_char *bufPtr = buffer.data();

	// Write each rectangle with its texture index to the buffer.
	for (size_t i = 0; i < rects.size(); ++i)
	{
		const Rect3D &rect = rects.at(i);
		const TextureReference &textureRef = this->textureRefs.at(textureIndices.at(i));

		cl_char *recPtr = bufPtr + (SIZEOF_RECTANGLE * i);

		cl_float *p1Ptr = reinterpret_cast<cl_float*>(recPtr);
		p1Ptr[0] = static_cast<cl_float>(rect.getP1().getX());
		p1Ptr[1] = static_cast<cl_float>(rect.getP1().getY());
		p1Ptr[2] = static_cast<cl_float>(rect.getP1().getZ());

		cl_float *p2Ptr = reinterpret_cast<cl_float*>(recPtr + sizeof(cl_float3));
		p2Ptr[0] = static_cast<cl_float>(rect.getP2().getX());
		p2Ptr[1] = static_cast<cl_float>(rect.getP2().getY());
		p2Ptr[2] = static_cast<cl_float>(rect.getP2().getZ());

		cl_float *p3Ptr = reinterpret_cast<cl_float*>(recPtr + (sizeof(cl_float3) * 2));
		p3Ptr[0] = static_cast<cl_float>(rect.getP3().getX());
		p3Ptr[1] = static_cast<cl_float>(rect.getP3().getY());
		p3Ptr[2] = static_cast<cl_float>(rect.getP3().getZ());

		const Float3f p1p2 = rect.getP2() - rect.getP1();
		cl_float *p1p2Ptr = reinterpret_cast<cl_float*>(recPtr + (sizeof(cl_float3) * 3));
		p1p2Ptr[0] = static_cast<cl_float>(p1p2.getX());
		p1p2Ptr[1] = static_cast<cl_float>(p1p2.getY());
		p1p2Ptr[2] = static_cast<cl_float>(p1p2.getZ());

		const Float3f p2p3 = rect.getP3() - rect.getP2();
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
		offsetPtr[0] = static_cast<cl_int>(textureRef.getOffset());

		cl_short *dimPtr = reinterpret_cast<cl_short*>(recPtr +
			(sizeof(cl_float3) * 6) + sizeof(cl_int));
		dimPtr[0] = static_cast<cl_short>(textureRef.getWidth());
		dimPtr[1] = static_cast<cl_short>(textureRef.getHeight());
	}

	// Write the buffer to device memory. Currently using naive rectangle buffer size.
	const int bytesPerVoxel = SIZEOF_RECTANGLE * MAX_RECTS_PER_VOXEL;
	const cl::size_type bufferOffset = (x * bytesPerVoxel) +
		((y * this->worldWidth) * bytesPerVoxel) +
		((z * this->worldWidth * this->worldHeight) * bytesPerVoxel);
	cl_int status = this->commandQueue.enqueueWriteBuffer(this->rectangleBuffer,
		CL_TRUE, bufferOffset, buffer.size(), static_cast<const void*>(bufPtr),
		nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::enqueueWriteBuffer updateVoxel");

	// Update the voxel reference at the given coordinate.
	this->updateVoxelRef(x, y, z, static_cast<int>(rects.size()));
}

void CLProgram::updateVoxel(int x, int y, int z,
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

	this->updateVoxel(x, y, z, rects, textureIndices);
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
