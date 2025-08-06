#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vector>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_SMART_HANDLE
#include "vulkan/vulkan.hpp"

#include "RendererSystem3D.h"

#include "components/utilities/Buffer.h"

class VulkanRenderer // final : public RendererSystem3D
{
private:
	vk::Instance instance;
	vk::SurfaceKHR surface;

	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::Device device;
	
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	vk::SwapchainKHR swapchain;
	Buffer<vk::ImageView> swapchainImageViews;

	vk::RenderPass renderPass;
	Buffer<vk::Framebuffer> swapchainFramebuffers;

	vk::ShaderModule vertexShaderModule;
	vk::ShaderModule fragmentShaderModule;

	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;

	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexBufferDeviceMemory;

	vk::Semaphore imageIsAvailableSemaphore;
	vk::Semaphore renderIsFinishedSemaphore;
	vk::Fence busyFence;
public:
	bool init(SDL_Window *window, const std::string &dataFolderPath);
	void shutdown();

	// @todo inherit RendererSystem3D and implement all those vertex buffer etc functions

	void update();
};

#endif
