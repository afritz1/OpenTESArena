#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vector>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_SMART_HANDLE
#include "vulkan/vulkan.hpp"

#include "RendererSystem3D.h"

#include "components/utilities/Buffer.h"

class VulkanRenderer final : public RendererSystem3D
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

	bool isInited() const override;

	void resize(int width, int height) override;

	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex) override;
	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex) override;
	IndexBufferID createIndexBuffer(int indexCount) override;
	void populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions) override;
	void populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes) override;
	void populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices) override;
	void freeVertexPositionBuffer(VertexPositionBufferID id) override;
	void freeVertexAttributeBuffer(VertexAttributeBufferID id) override;
	void freeIndexBuffer(IndexBufferID id) override;

	ObjectTextureAllocator *getTextureAllocator() override;

	UniformBufferID createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement) override;
	void populateUniformBuffer(UniformBufferID id, Span<const std::byte> data) override;
	void populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData) override;
	void freeUniformBuffer(UniformBufferID id) override;

	RenderLightID createLight() override;
	void setLightPosition(RenderLightID id, const Double3 &worldPoint) override;
	void setLightRadius(RenderLightID id, double startRadius, double endRadius) override;
	void freeLight(RenderLightID id) override;

	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;

	Renderer3DProfilerData getProfilerData() const override;

	void submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings, const RenderCommandList &commandList, uint32_t *outputBuffer) override;
};

#endif
