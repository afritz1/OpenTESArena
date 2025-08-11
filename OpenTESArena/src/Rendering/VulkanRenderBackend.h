#ifndef VULKAN_RENDER_BACKEND_H
#define VULKAN_RENDER_BACKEND_H

#include <cstdint>
#include <vector>

#include "vulkan/vulkan.hpp"

#include "RenderBackend.h"
#include "RenderTextureAllocator.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/RecyclablePool.h"

struct VulkanBufferVertexPositionInfo
{
	int vertexCount;
	int componentsPerVertex;
	size_t sizeOfComponent;
};

struct VulkanBufferVertexAttributeInfo
{
	int vertexCount;
	int componentsPerVertex;
	size_t sizeOfComponent;
};

struct VulkanBufferIndexInfo
{
	int indexCount;
	size_t sizeOfIndex;
};

struct VulkanBufferUniformInfo
{
	int elementCount;
	size_t sizeOfElement;
	size_t alignmentOfElement;
};

enum class VulkanBufferType
{
	VertexPosition,
	VertexAttribute,
	Index,
	Uniform
};

struct VulkanBuffer
{
	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;

	VulkanBufferType type;

	union
	{
		VulkanBufferVertexPositionInfo vertexPosition;
		VulkanBufferVertexAttributeInfo vertexAttribute;
		VulkanBufferIndexInfo index;
		VulkanBufferUniformInfo uniform;
	};

	VulkanBuffer();

	void initVertexPosition(int vertexCount, int componentsPerVertex, size_t sizeOfComponent, vk::Buffer buffer, vk::DeviceMemory deviceMemory);
	void initVertexAttribute(int vertexCount, int componentsPerVertex, size_t sizeOfComponent, vk::Buffer buffer, vk::DeviceMemory deviceMemory);
	void initIndex(int indexCount, size_t sizeOfIndex, vk::Buffer buffer, vk::DeviceMemory deviceMemory);
	void initUniform(int elementCount, size_t sizeOfElement, size_t alignmentOfElement, vk::Buffer buffer, vk::DeviceMemory deviceMemory);

	void setLocked(vk::Buffer stagingBuffer, vk::DeviceMemory stagingDeviceMemory);
	void setUnlocked();
};

struct VulkanLightInfo
{
	float pointX, pointY, pointZ;
	float startRadius, endRadius;
	float startRadiusSqr, endRadiusSqr;

	VulkanLightInfo();

	void init(float pointX, float pointY, float pointZ, float startRadius, float endRadius);
};

struct VulkanLight
{
	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;
	VulkanLightInfo lightInfo;

	void init(float pointX, float pointY, float pointZ, float startRadius, float endRadius, vk::Buffer buffer, vk::DeviceMemory deviceMemory);

	void setLocked(vk::Buffer stagingBuffer, vk::DeviceMemory stagingDeviceMemory);
	void setUnlocked();
};

struct VulkanTexture
{
	int width;
	int height;
	int bytesPerTexel;
	vk::Image image;
	vk::DeviceMemory deviceMemory;
	vk::ImageView imageView;
	vk::Sampler sampler;
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;

	VulkanTexture();

	void init(int width, int height, int bytesPerTexel, vk::Image image, vk::DeviceMemory deviceMemory, vk::ImageView imageView, vk::Sampler sampler);

	void setLocked(vk::Buffer stagingBuffer, vk::DeviceMemory stagingDeviceMemory);
	void setUnlocked();
};

using VulkanVertexPositionBufferPool = RecyclablePool<VertexPositionBufferID, VulkanBuffer>;
using VulkanVertexAttributeBufferPool = RecyclablePool<VertexAttributeBufferID, VulkanBuffer>;
using VulkanIndexBufferPool = RecyclablePool<IndexBufferID, VulkanBuffer>;
using VulkanUniformBufferPool = RecyclablePool<UniformBufferID, VulkanBuffer>;
using VulkanLightPool = RecyclablePool<RenderLightID, VulkanLight>;
using VulkanObjectTexturePool = RecyclablePool<ObjectTextureID, VulkanTexture>;
using VulkanUiTexturePool = RecyclablePool<UiTextureID, VulkanTexture>;

struct VulkanObjectTextureAllocator final : public ObjectTextureAllocator
{
	VulkanObjectTexturePool *pool;
	vk::PhysicalDevice physicalDevice;
	uint32_t queueFamilyIndex;
	vk::Device device;
	vk::Queue queue;
	vk::CommandBuffer commandBuffer;

	VulkanObjectTextureAllocator();

	void init(VulkanObjectTexturePool *pool, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex, vk::Device device, vk::Queue queue, vk::CommandBuffer commandBuffer);

	ObjectTextureID create(int width, int height, int bytesPerTexel) override;
	ObjectTextureID create(const TextureBuilder &textureBuilder) override;

	void free(ObjectTextureID textureID) override;

	LockedTexture lock(ObjectTextureID textureID) override;
	void unlock(ObjectTextureID textureID) override;
};

struct VulkanUiTextureAllocator final : public UiTextureAllocator
{
	VulkanUiTexturePool *pool;
	vk::PhysicalDevice physicalDevice;
	uint32_t queueFamilyIndex;
	vk::Device device;
	vk::Queue queue;
	vk::CommandBuffer commandBuffer;

	VulkanUiTextureAllocator();

	void init(VulkanUiTexturePool *pool, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex, vk::Device device, vk::Queue queue, vk::CommandBuffer commandBuffer);

	UiTextureID create(int width, int height) override;
	UiTextureID create(Span2D<const uint32_t> texels) override;
	UiTextureID create(Span2D<const uint8_t> texels, const Palette &palette) override;
	UiTextureID create(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager) override;

	void free(UiTextureID textureID) override;

	LockedTexture lock(UiTextureID textureID) override;
	void unlock(UiTextureID textureID) override;
};

struct VulkanCamera
{
	static constexpr int BYTE_COUNT = sizeof(Matrix4f) * 3;

	// Updates frequently, host-visible.
	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	Span<std::byte> hostMappedBytes; // Always mapped.

	Matrix4f model, view, projection;
	Span<const std::byte> matrixBytes;

	VulkanCamera();

	void init(vk::Buffer buffer, vk::DeviceMemory deviceMemory, Span<std::byte> hostMappedBytes);
};

class VulkanRenderBackend final : public RenderBackend
{
private:
	vk::Instance instance;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicalDevice;
	uint32_t graphicsQueueFamilyIndex;
	uint32_t presentQueueFamilyIndex;

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

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorPool descriptorPool;
	vk::DescriptorSet descriptorSet;

	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;

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

	VulkanCamera camera;
public:
	bool init(const RenderInitSettings &initSettings) override;
	void shutdown() override;

	void resize(int windowWidth, int windowHeight, int internalWidth, int internalHeight) override;
	void handleRenderTargetsReset(int windowWidth, int windowHeight, int internalWidth, int internalHeight) override;

	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex) override;
	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex) override;
	IndexBufferID createIndexBuffer(int indexCount) override;
	Span<float> lockVertexPositionBuffer(VertexPositionBufferID id);
	void unlockVertexPositionBuffer(VertexPositionBufferID id);
	void populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions) override;
	Span<float> lockVertexAttributeBuffer(VertexAttributeBufferID id);
	void unlockVertexAttributeBuffer(VertexAttributeBufferID id);
	void populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes) override;
	Span<int32_t> lockIndexBuffer(IndexBufferID id);
	void unlockIndexBuffer(IndexBufferID id);
	void populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices) override;
	void freeVertexPositionBuffer(VertexPositionBufferID id) override;
	void freeVertexAttributeBuffer(VertexAttributeBufferID id) override;
	void freeIndexBuffer(IndexBufferID id) override;

	ObjectTextureAllocator *getObjectTextureAllocator() override;
	UiTextureAllocator *getUiTextureAllocator() override;
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;
	std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const override;

	UniformBufferID createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement) override;
	Span<std::byte> lockUniformBuffer(UniformBufferID id);
	void unlockUniformBuffer(UniformBufferID id);
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
