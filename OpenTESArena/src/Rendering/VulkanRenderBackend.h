#ifndef VULKAN_RENDER_BACKEND_H
#define VULKAN_RENDER_BACKEND_H

#include <cstdint>
#include <functional>
#include <vector>

#include "vulkan/vulkan.hpp"

#include "RenderBackend.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Heap.h"
#include "components/utilities/RecyclablePool.h"

struct VulkanBufferVertexPositionInfo
{
	int vertexCount;
	int componentsPerVertex;
	int bytesPerComponent;
};

struct VulkanBufferVertexAttributeInfo
{
	int vertexCount;
	int componentsPerVertex;
	int bytesPerComponent;
};

struct VulkanBufferIndexInfo
{
	int indexCount;
	int bytesPerIndex;
};

struct VulkanBufferUniformInfo
{
	int elementCount;
	int bytesPerElement;
	int alignmentOfElement;
	vk::DescriptorSet descriptorSet;
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
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;

	VulkanBufferType type;

	union
	{
		VulkanBufferVertexPositionInfo vertexPosition;
		VulkanBufferVertexAttributeInfo vertexAttribute;
		VulkanBufferIndexInfo index;
		VulkanBufferUniformInfo uniform;
	};

	VulkanBuffer();

	void init(vk::Buffer buffer, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes);

	void initVertexPosition(int vertexCount, int componentsPerVertex, int bytesPerComponent);
	void initVertexAttribute(int vertexCount, int componentsPerVertex, int bytesPerComponent);
	void initIndex(int indexCount, int bytesPerIndex);
	void initUniform(int elementCount, int bytesPerElement, int alignmentOfElement, vk::DescriptorSet descriptorSet);
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
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	VulkanLightInfo lightInfo;

	void init(float pointX, float pointY, float pointZ, float startRadius, float endRadius, vk::Buffer buffer, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes);
};

struct VulkanTexture
{
	int width;
	int height;
	int bytesPerTexel;
	vk::Image image;
	vk::ImageView imageView;
	vk::Sampler sampler;
	vk::DescriptorSet descriptorSet;
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;

	VulkanTexture();

	void init(int width, int height, int bytesPerTexel, vk::Image image, vk::ImageView imageView, vk::Sampler sampler, vk::DescriptorSet descriptorSet,
		vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes);
};

using VulkanVertexPositionBufferPool = RecyclablePool<VertexPositionBufferID, VulkanBuffer>;
using VulkanVertexAttributeBufferPool = RecyclablePool<VertexAttributeBufferID, VulkanBuffer>;
using VulkanIndexBufferPool = RecyclablePool<IndexBufferID, VulkanBuffer>;
using VulkanUniformBufferPool = RecyclablePool<UniformBufferID, VulkanBuffer>;
using VulkanLightPool = RecyclablePool<RenderLightID, VulkanLight>;
using VulkanObjectTexturePool = RecyclablePool<ObjectTextureID, VulkanTexture>;
using VulkanUiTexturePool = RecyclablePool<UiTextureID, VulkanTexture>;

struct VulkanPendingCommands
{
	std::vector<std::function<void()>> copyCommands;
};

struct VulkanCamera
{
	static constexpr int BYTE_COUNT = sizeof(Matrix4f);

	// Updates frequently, host-visible.
	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	Span<std::byte> hostMappedBytes; // Always mapped.

	Matrix4f viewProjection;
	Span<const std::byte> matrixBytes;

	VulkanCamera();

	void init(vk::Buffer buffer, vk::DeviceMemory deviceMemory, Span<std::byte> hostMappedBytes);
};

enum class VulkanHeapType
{
	Buffer,
	Image
};

struct VulkanHeapBufferMapping
{
	vk::Buffer buffer;
	HeapBlock block;
};

struct VulkanHeapImageMapping
{
	vk::Image image;
	HeapBlock block;
};

// A single memory allocation sliced by several smaller buffers/images.
struct VulkanHeap
{
	VulkanHeapType type;
	vk::DeviceMemory deviceMemory;
	Span<std::byte> hostMappedBytes;
	HeapAllocator allocator;
	std::vector<VulkanHeapBufferMapping> bufferMappings;
	std::vector<VulkanHeapImageMapping> imageMappings;

	VulkanHeap();

	bool initBufferHeap(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, bool isHostVisible, vk::PhysicalDevice physicalDevice);
	bool initImageHeap(vk::Device device, int byteCount, vk::ImageUsageFlags usageFlags, vk::PhysicalDevice physicalDevice);

	HeapBlock addBufferMapping(vk::Buffer buffer, int byteCount, int alignment);
	HeapBlock addImageMapping(vk::Image image, int byteCount, int alignment);
	void freeBufferMapping(vk::Buffer buffer);
	void freeImageMapping(vk::Image image);
	void clear();
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
	vk::DeviceMemory depthDeviceMemory;
	vk::Image depthImage;
	vk::ImageView depthImageView;
	vk::RenderPass renderPass;
	Buffer<vk::Framebuffer> swapchainFramebuffers;

	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;
	VulkanPendingCommands pendingCommands;

	vk::ShaderModule vertexShaderModule;
	vk::ShaderModule fragmentShaderModule;

	vk::DescriptorPool descriptorPool;
	vk::DescriptorSetLayout globalDescriptorSetLayout;
	vk::DescriptorSetLayout transformDescriptorSetLayout;
	vk::DescriptorSetLayout materialDescriptorSetLayout;
	vk::DescriptorSet globalDescriptorSet;

	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	vk::Pipeline noDepthGraphicsPipeline;

	vk::Semaphore imageIsAvailableSemaphore;
	vk::Semaphore renderIsFinishedSemaphore;

	VulkanVertexPositionBufferPool vertexPositionBufferPool;
	VulkanVertexAttributeBufferPool vertexAttributeBufferPool;
	VulkanIndexBufferPool indexBufferPool;
	VulkanUniformBufferPool uniformBufferPool;
	VulkanLightPool lightPool;
	VulkanObjectTexturePool objectTexturePool;
	VulkanUiTexturePool uiTexturePool;

	VulkanHeap vertexBufferDeviceLocalHeap;
	VulkanHeap vertexBufferStagingHeap;
	VulkanHeap indexBufferDeviceLocalHeap;
	VulkanHeap indexBufferStagingHeap;
	VulkanHeap uniformBufferDeviceLocalHeap;
	VulkanHeap uniformBufferStagingHeap;
	VulkanHeap objectTextureDeviceLocalHeap;
	VulkanHeap objectTextureStagingHeap;
	VulkanHeap uiTextureDeviceLocalHeap;
	VulkanHeap uiTextureStagingHeap;

	VulkanCamera camera;
public:
	bool init(const RenderInitSettings &initSettings) override;
	void shutdown() override;

	void resize(int windowWidth, int windowHeight, int internalWidth, int internalHeight) override;
	void handleRenderTargetsReset(int windowWidth, int windowHeight, int internalWidth, int internalHeight) override;

	Renderer3DProfilerData getProfilerData() const override;

	Surface getScreenshot() const override;

	int getBytesPerFloat() const override;

	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent) override;
	void freeVertexPositionBuffer(VertexPositionBufferID id) override;
	LockedBuffer lockVertexPositionBuffer(VertexPositionBufferID id) override;
	void unlockVertexPositionBuffer(VertexPositionBufferID id) override;

	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent) override;
	void freeVertexAttributeBuffer(VertexAttributeBufferID id) override;
	LockedBuffer lockVertexAttributeBuffer(VertexAttributeBufferID id) override;
	void unlockVertexAttributeBuffer(VertexAttributeBufferID id) override;

	IndexBufferID createIndexBuffer(int indexCount, int bytesPerIndex) override;
	void freeIndexBuffer(IndexBufferID id) override;
	LockedBuffer lockIndexBuffer(IndexBufferID id) override;
	void unlockIndexBuffer(IndexBufferID id) override;

	UniformBufferID createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement) override;
	void freeUniformBuffer(UniformBufferID id) override;
	LockedBuffer lockUniformBuffer(UniformBufferID id) override;
	LockedBuffer lockUniformBufferIndex(UniformBufferID id, int index) override;
	void unlockUniformBuffer(UniformBufferID id) override;
	void unlockUniformBufferIndex(UniformBufferID id, int index) override;

	RenderLightID createLight() override;
	void freeLight(RenderLightID id) override;
	bool populateLight(RenderLightID id, const Double3 &point, double startRadius, double endRadius) override;

	ObjectTextureID createObjectTexture(int width, int height, int bytesPerTexel) override;
	void freeObjectTexture(ObjectTextureID id) override;
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;
	LockedTexture lockObjectTexture(ObjectTextureID id) override;
	void unlockObjectTexture(ObjectTextureID id) override;

	UiTextureID createUiTexture(int width, int height) override;
	void freeUiTexture(UiTextureID id) override;
	std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const override;
	LockedTexture lockUiTexture(UiTextureID id) override;
	void unlockUiTexture(UiTextureID id) override;

	void submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
		const RenderCamera &camera, const RenderFrameSettings &frameSettings) override;
};

#endif
