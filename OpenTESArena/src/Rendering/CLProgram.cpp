#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"

#include "CLProgram.h"

#include "../Utilities/Debug.h"
#include "../Utilities/File.h"

const std::string CLProgram::PATH = "data/kernels/";
const std::string CLProgram::FILENAME = "kernel.cl";
const std::string CLProgram::ENTRY_FUNCTION = "render";

CLProgram::CLProgram(int width, int height)
{
	this->context = nullptr;
	this->commandQueue = nullptr;
	this->program = nullptr;
	this->kernel = nullptr;
	this->colorBuffer = nullptr;
	this->width = width;
	this->height = height;

	auto platforms = this->getPlatforms();
	Debug::check(platforms.size() > 0, "CLProgram", "No OpenCL platform found.");

	auto devices = this->getDevices(platforms.at(0), CL_DEVICE_TYPE_GPU);
	Debug::check(devices.size() > 0, "CLProgram", "No OpenCL device found.");

	this->device = devices.at(0);

	auto status = cl_int(CL_SUCCESS);
	this->context = cl::Context(this->device, false, nullptr, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Context.");

	this->commandQueue = cl::CommandQueue(this->context, this->device, 0, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::CommandQueue.");

	auto source = File::toString(CLProgram::PATH + CLProgram::FILENAME);
	auto options = std::string("-cl-fast-relaxed-math -cl-strict-aliasing");
	this->program = cl::Program(this->context, source, false, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Program.");

	status = this->program.build(devices, options.c_str());
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Program::build (" +
		this->getErrorString(status) + ").");

	this->kernel = cl::Kernel(this->program, CLProgram::ENTRY_FUNCTION.c_str(), &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Kernel.");

	this->colorBuffer = cl::Buffer(this->context, CL_MEM_WRITE_ONLY, 
		sizeof(cl_uint) * width * height, nullptr, &status);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::Buffer colorBuffer.");

	this->kernel.setArg(0, this->colorBuffer);

	assert(this->device.get() != nullptr);
	assert(this->context.get() != nullptr);
	assert(this->commandQueue.get() != nullptr);
	assert(this->program.get() != nullptr);
	assert(this->kernel.get() != nullptr);
	assert(this->colorBuffer.get() != nullptr);
	assert(this->width == width);
	assert(this->height == height);
}

CLProgram::~CLProgram()
{
	
}

std::vector<cl::Platform> CLProgram::getPlatforms()
{
	auto platforms = std::vector<cl::Platform>();

	auto status = cl::Platform::get(&platforms);
	Debug::check(status == CL_SUCCESS, "CLProgram", "CLProgram::getPlatforms().");

	return platforms;
}

std::vector<cl::Device> CLProgram::getDevices(const cl::Platform &platform,
	cl_device_type type)
{
	auto devices = std::vector<cl::Device>();

	auto status = platform.getDevices(type, &devices);
	Debug::check((status == CL_SUCCESS) || (status == CL_DEVICE_NOT_FOUND), "CLProgram",
		"CLProgram::getDevices().");

	return devices;
}

std::string CLProgram::getBuildReport() const
{
	assert(this->device.get() != nullptr);
	assert(this->program.get() != nullptr);

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

void CLProgram::render(SDL_Surface *dst)
{
	assert(dst != nullptr);
	assert(this->width == dst->w);
	assert(this->height == dst->h);

	auto status = this->commandQueue.enqueueNDRangeKernel(this->kernel, 
		cl::NullRange, cl::NDRange(this->width, this->height), cl::NullRange,
		nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::CommandQueue::enqueueNDRangeKernel.");

	status = this->commandQueue.finish();
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::CommandQueue::finish.");

	status = this->commandQueue.enqueueReadBuffer(this->colorBuffer, true, 0,
		sizeof(cl_uint) * this->width * this->height, dst->pixels, nullptr, nullptr);
	Debug::check(status == CL_SUCCESS, "CLProgram", "cl::CommandQueue::enqueueReadBuffer.");
}
