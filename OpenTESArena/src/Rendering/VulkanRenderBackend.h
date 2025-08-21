#ifndef VULKAN_RENDER_BACKEND_H
#define VULKAN_RENDER_BACKEND_H

#include <cstdint>
#include <functional>
#include <vector>

#define VULKAN_HPP_ASSERT(x) (nullptr)
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
	int alignedBytesPerElement;
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
	void initUniform(int elementCount, int bytesPerElement, int alignedBytesPerElement, int alignmentOfElement, vk::DescriptorSet descriptorSet);
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

struct VulkanHeap
{
	vk::DeviceMemory deviceMemory;
	Span<std::byte> hostMappedBytes;
	HeapAllocator allocator;
};

struct VulkanHeapMapping
{
	int heapIndex;
	HeapBlock block;

	VulkanHeapMapping();

	bool isValid() const;
};

struct VulkanHeapBufferMapping
{
	VulkanHeapMapping mapping;
	vk::Buffer buffer;
};

struct VulkanHeapImageMapping
{
	VulkanHeapMapping mapping;
	vk::Image image;
};

// Manages several independent heaps which are sliced by several smaller buffers/images.
struct VulkanHeapManager
{
	std::vector<VulkanHeap> heaps;
	vk::Device device;
	vk::MemoryAllocateInfo memoryAllocateInfo; // For creating new heaps.
	bool isHostVisible;

	VulkanHeapType type;
	std::vector<VulkanHeapBufferMapping> bufferMappings;
	std::vector<VulkanHeapImageMapping> imageMappings;

	VulkanHeapManager();

	bool initBufferManager(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, bool isHostVisible, vk::PhysicalDevice physicalDevice);
	bool initImageManager(vk::Device device, int byteCount, vk::ImageUsageFlags usageFlags, vk::PhysicalDevice physicalDevice);

	VulkanHeap &getHeap(int heapIndex);
	int findAvailableHeapIndex(int byteCount, int alignment) const;

	int addHeap();

	VulkanHeapMapping addBufferMapping(vk::Buffer buffer, int byteCount, int alignment);
	VulkanHeapMapping addImageMapping(vk::Image image, int byteCount, int alignment);
	void freeBufferMapping(vk::Buffer buffer);
	void freeImageMapping(vk::Image image);
	void freeAllocations();
	void clear();
};

struct VulkanVertexShader
{
	VertexShaderType type;
	vk::ShaderModule module;

	VulkanVertexShader();
};

struct VulkanFragmentShader
{
	PixelShaderType type;
	vk::ShaderModule module;
	
	VulkanFragmentShader();
};

using VulkanPipelineKeyCode = uint8_t;

struct VulkanPipelineKey
{
	VertexShaderType vertexShaderType;
	PixelShaderType fragmentShaderType;
	bool depthTest;
	bool backFaceCulling;

	VulkanPipelineKey();

	constexpr VulkanPipelineKey(VertexShaderType vertexShaderType, PixelShaderType fragmentShaderType, bool depthTest, bool backFaceCulling)
	{
		this->vertexShaderType = vertexShaderType;
		this->fragmentShaderType = fragmentShaderType;
		this->depthTest = depthTest;
		this->backFaceCulling = backFaceCulling;
	}
};

struct VulkanPipeline
{
	VulkanPipelineKeyCode keyCode;
	vk::Pipeline pipeline;
};

using VulkanPendingCommands = std::vector<std::function<void()>>;

class VulkanRenderBackend final : public RenderBackend
{
private:
	vk::Instance instance;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties physicalDeviceProperties;
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
	VulkanPendingCommands copyCommands;
	VulkanPendingCommands freeCommands;

	Buffer<VulkanVertexShader> vertexShaders;
	Buffer<VulkanFragmentShader> fragmentShaders;

	vk::DescriptorPool descriptorPool;
	vk::DescriptorSetLayout globalDescriptorSetLayout;
	vk::DescriptorSetLayout transformDescriptorSetLayout;
	vk::DescriptorSetLayout materialDescriptorSetLayout;
	vk::DescriptorSet globalDescriptorSet;

	vk::PipelineLayout scenePipelineLayout;
	vk::PipelineLayout uiPipelineLayout;
	Buffer<VulkanPipeline> graphicsPipelines;

	vk::Semaphore imageIsAvailableSemaphore;
	vk::Semaphore renderIsFinishedSemaphore;

	VulkanVertexPositionBufferPool vertexPositionBufferPool;
	VulkanVertexAttributeBufferPool vertexAttributeBufferPool;
	VulkanIndexBufferPool indexBufferPool;
	VulkanUniformBufferPool uniformBufferPool;
	VulkanLightPool lightPool;
	VulkanObjectTexturePool objectTexturePool;
	VulkanUiTexturePool uiTexturePool;

	VulkanHeapManager vertexBufferHeapManagerDeviceLocal;
	VulkanHeapManager vertexBufferHeapManagerStaging;
	VulkanHeapManager indexBufferHeapManagerDeviceLocal;
	VulkanHeapManager indexBufferHeapManagerStaging;
	VulkanHeapManager uniformBufferHeapManagerDeviceLocal;
	VulkanHeapManager uniformBufferHeapManagerStaging;
	VulkanHeapManager objectTextureHeapManagerDeviceLocal;
	VulkanHeapManager objectTextureHeapManagerStaging;
	VulkanHeapManager uiTextureHeapManagerDeviceLocal;
	VulkanHeapManager uiTextureHeapManagerStaging;

	VulkanCamera camera;
	VertexPositionBufferID uiVertexPositionBufferID;
	VertexAttributeBufferID uiVertexAttributeBufferID;
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
