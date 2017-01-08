#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include "cl2.hpp"

std::once_flag cl::Device::default_initialized_;
cl::Device cl::Device::default_;
cl_int cl::Device::default_error_ = CL_SUCCESS;

std::once_flag cl::Platform::default_initialized_;
cl::Platform cl::Platform::default_;
cl_int cl::Platform::default_error_ = CL_SUCCESS;

std::once_flag cl::Context::default_initialized_;
cl::Context cl::Context::default_;
cl_int cl::Context::default_error_ = CL_SUCCESS;

std::once_flag cl::CommandQueue::default_initialized_;
cl::CommandQueue cl::CommandQueue::default_;
cl_int cl::CommandQueue::default_error_ = CL_SUCCESS;
