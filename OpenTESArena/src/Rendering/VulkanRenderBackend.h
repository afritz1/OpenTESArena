#ifndef VULKAN_RENDER_BACKEND_H
#define VULKAN_RENDER_BACKEND_H

#include <vector>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_SMART_HANDLE
#include "vulkan/vulkan.hpp"

#include "RenderBackend.h"
#include "RenderTextureAllocator.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/RecyclablePool.h"

struct VulkanVertexPositionBuffer
{
	// @todo: vk::Buffer

	void init(int vertexCount, int componentsPerVertex);
};

struct VulkanVertexAttributeBuffer
{
	// @todo: vk::Buffer

	void init(int vertexCount, int componentsPerVertex);
};

struct VulkanIndexBuffer
{
	// @todo: vk::Buffer

	void init(int indexCount);
};

struct VulkanUniformBuffer
{
	// @todo vk::Buffer
	int elementCount;
	size_t sizeOfElement;
	size_t alignmentOfElement;

	VulkanUniformBuffer();

	void init(int elementCount, size_t sizeOfElement, size_t alignmentOfElement);
};

struct VulkanLight
{
	// @todo vk something for worldPoint/startRadius/endRadius
};

struct VulkanTexture
{
	Buffer<std::byte> texels;
	int width = 0;
	int height = 0;
	int bytesPerTexel = 0;

	// @todo vk::Image handle

	VulkanTexture();
	
	void init(int width, int height, int bytesPerTexel);
};

using VulkanVertexPositionBufferPool = RecyclablePool<VertexPositionBufferID, VulkanVertexPositionBuffer>;
using VulkanVertexAttributeBufferPool = RecyclablePool<VertexAttributeBufferID, VulkanVertexAttributeBuffer>;
using VulkanIndexBufferPool = RecyclablePool<IndexBufferID, VulkanIndexBuffer>;
using VulkanUniformBufferPool = RecyclablePool<UniformBufferID, VulkanUniformBuffer>;
using VulkanLightPool = RecyclablePool<RenderLightID, VulkanLight>;
using VulkanObjectTexturePool = RecyclablePool<ObjectTextureID, VulkanTexture>;
using VulkanUiTexturePool = RecyclablePool<UiTextureID, VulkanTexture>;

struct VulkanObjectTextureAllocator final : public ObjectTextureAllocator
{
	VulkanObjectTexturePool *pool;
	vk::Device device;

	VulkanObjectTextureAllocator();

	void init(VulkanObjectTexturePool *pool, vk::Device device);

	ObjectTextureID create(int width, int height, int bytesPerTexel) override;
	ObjectTextureID create(const TextureBuilder &textureBuilder) override;
	
	void free(ObjectTextureID textureID) override;

	LockedTexture lock(ObjectTextureID textureID) override;
	void unlock(ObjectTextureID textureID) override;
};

struct VulkanUiTextureAllocator final : public UiTextureAllocator
{
	VulkanUiTexturePool *pool;
	vk::Device device;

	VulkanUiTextureAllocator();

	void init(VulkanUiTexturePool *pool, vk::Device device);

	UiTextureID create(int width, int height) override;
	UiTextureID create(Span2D<const uint32_t> texels) override;
	UiTextureID create(Span2D<const uint8_t> texels, const Palette &palette) override;
	UiTextureID create(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager) override;

	void free(UiTextureID textureID) override;

	LockedTexture lock(UiTextureID textureID) override;
	void unlock(UiTextureID textureID) override;
};

class VulkanRenderBackend final : public RenderBackend
{
private:
	vk::Instance instance;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicalDevice;

	vk::Device device;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;

	vk::Extent2D swapchainExtent;
	vk::SwapchainKHR swapchain;
	Buffer<vk::ImageView> swapchainImageViews;
	vk::RenderPass renderPass;
	Buffer<vk::Framebuffer> swapchainFramebuffers;

	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;

	vk::ShaderModule vertexShaderModule;
	vk::ShaderModule fragmentShaderModule;

	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;

	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexBufferDeviceMemory;

	vk::Semaphore imageIsAvailableSemaphore;
	vk::Semaphore renderIsFinishedSemaphore;

	VulkanVertexPositionBufferPool vertexPositionBufferPool;
	VulkanVertexAttributeBufferPool vertexAttributeBufferPool;
	VulkanIndexBufferPool indexBufferPool;
	VulkanUniformBufferPool uniformBufferPool;
	VulkanLightPool lightPool;

	VulkanObjectTexturePool objectTexturePool;
	VulkanObjectTextureAllocator objectTextureAllocator;

	VulkanUiTexturePool uiTexturePool;
	VulkanUiTextureAllocator uiTextureAllocator;
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
