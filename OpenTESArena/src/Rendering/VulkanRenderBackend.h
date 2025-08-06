#ifndef VULKAN_RENDER_BACKEND_H
#define VULKAN_RENDER_BACKEND_H

#include <vector>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_SMART_HANDLE
#include "vulkan/vulkan.hpp"

#include "RenderBackend.h"

#include "components/utilities/Buffer.h"

class VulkanRenderBackend final : public RenderBackend
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
	bool init(const RenderInitSettings &initSettings) override;
	void shutdown() override;

	void resize(int windowWidth, int windowHeight, int internalWidth, int internalHeight) override;
	void handleRenderTargetsReset(int windowWidth, int windowHeight, int internalWidth, int internalHeight) override;

	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex) override;
	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex) override;
	IndexBufferID createIndexBuffer(int indexCount) override;
	void populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions) override;
	void populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes) override;
	void populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices) override;
	void freeVertexPositionBuffer(VertexPositionBufferID id) override;
	void freeVertexAttributeBuffer(VertexAttributeBufferID id) override;
	void freeIndexBuffer(IndexBufferID id) override;

	ObjectTextureAllocator *getObjectTextureAllocator() override;
	UiTextureAllocator *getUiTextureAllocator() override;
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;
	std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const override;

	UniformBufferID createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement) override;
	void populateUniformBuffer(UniformBufferID id, Span<const std::byte> data) override;
	void populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData) override;
	void freeUniformBuffer(UniformBufferID id) override;

	RenderLightID createLight() override;
	void setLightPosition(RenderLightID id, const Double3 &worldPoint) override;
	void setLightRadius(RenderLightID id, double startRadius, double endRadius) override;
	void freeLight(RenderLightID id) override;

	Renderer3DProfilerData getProfilerData() const override;

	Surface getScreenshot() const override;

	void submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
		const RenderCamera &camera, const RenderFrameSettings &frameSettings) override;
};

#endif
