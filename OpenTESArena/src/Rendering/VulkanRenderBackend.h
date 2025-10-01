#ifndef VULKAN_RENDER_BACKEND_H
#define VULKAN_RENDER_BACKEND_H

#ifdef HAVE_VULKAN

#include <cstdint>
#include <functional>
#include <vector>

#define VULKAN_HPP_ASSERT(x) (nullptr)
#include "vulkan/vulkan.hpp"

#include "RenderBackend.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/FlatMap.h"
#include "components/utilities/Heap.h"
#include "components/utilities/KeyValuePool.h"

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
	int bytesPerStride;
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
	vk::Buffer deviceLocalBuffer;
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

	void init(vk::Buffer deviceLocalBuffer, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes);

	void initVertexPosition(int vertexCount, int componentsPerVertex, int bytesPerComponent);
	void initVertexAttribute(int vertexCount, int componentsPerVertex, int bytesPerComponent);
	void initIndex(int indexCount, int bytesPerIndex);
	void initUniform(int elementCount, int bytesPerElement, int bytesPerStride, vk::DescriptorSet descriptorSet);

	void freeAllocations(vk::Device device);
};

struct VulkanTexture
{
	int width;
	int height;
	int bytesPerTexel;
	vk::Image image;
	vk::ImageView imageView;
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;

	VulkanTexture();

	void init(int width, int height, int bytesPerTexel, vk::Image image, vk::ImageView imageView, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes);

	void freeAllocations(vk::Device device);
};

enum class VulkanMaterialPushConstantType
{
	None,
	MeshLightPercent,
	TexCoordAnimPercent
};

struct VulkanMaterial
{
	vk::Pipeline pipeline;
	vk::PipelineLayout pipelineLayout;
	vk::DescriptorSet descriptorSet;
	VulkanMaterialPushConstantType pushConstantTypes[2];

	VulkanMaterial();

	void init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, vk::DescriptorSet descriptorSet);
};

struct VulkanMaterialInstance
{
	float meshLightPercent;
	float texCoordAnimPercent;

	VulkanMaterialInstance();
};

using VulkanVertexPositionBufferPool = KeyValuePool<VertexPositionBufferID, VulkanBuffer>;
using VulkanVertexAttributeBufferPool = KeyValuePool<VertexAttributeBufferID, VulkanBuffer>;
using VulkanIndexBufferPool = KeyValuePool<IndexBufferID, VulkanBuffer>;
using VulkanUniformBufferPool = KeyValuePool<UniformBufferID, VulkanBuffer>;
using VulkanObjectTexturePool = KeyValuePool<ObjectTextureID, VulkanTexture>;
using VulkanUiTexturePool = KeyValuePool<UiTextureID, VulkanTexture>;
using VulkanMaterialPool = KeyValuePool<RenderMaterialID, VulkanMaterial>;
using VulkanMaterialInstancePool = KeyValuePool<RenderMaterialInstanceID, VulkanMaterialInstance>;

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
	FragmentShaderType type;
	vk::ShaderModule module;

	VulkanFragmentShader();
};

using VulkanPipelineKeyCode = uint16_t;

struct VulkanPipelineKey
{
	VertexShaderType vertexShaderType;
	FragmentShaderType fragmentShaderType;
	bool depthRead;
	bool depthWrite;
	bool backFaceCulling;
	bool alphaBlend;

	VulkanPipelineKey();

	constexpr VulkanPipelineKey(VertexShaderType vertexShaderType, FragmentShaderType fragmentShaderType, bool depthRead, bool depthWrite, bool backFaceCulling, bool alphaBlend)
	{
		this->vertexShaderType = vertexShaderType;
		this->fragmentShaderType = fragmentShaderType;
		this->depthRead = depthRead;
		this->depthWrite = depthWrite;
		this->backFaceCulling = backFaceCulling;
		this->alphaBlend = alphaBlend;
	}

	bool operator==(const VulkanPipelineKey &other) const
	{
		return (this->vertexShaderType == other.vertexShaderType) && (this->fragmentShaderType == other.fragmentShaderType) &&
			(this->depthRead == other.depthRead) && (this->depthWrite == other.depthWrite) &&
			(this->backFaceCulling == other.backFaceCulling) && (this->alphaBlend == other.alphaBlend);
	}
};

struct VulkanPipeline
{
	VulkanPipelineKeyCode keyCode;
	vk::Pipeline pipeline;
};

struct VulkanBufferTransferCommand
{
	vk::Buffer srcBuffer;
	vk::Buffer dstBuffer;
	vk::PipelineStageFlags dstStageFlags;
	vk::AccessFlags dstAccessMask;
	int byteOffset;
	int byteCount;

	VulkanBufferTransferCommand();

	void init(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::PipelineStageFlags dstStageFlags, vk::AccessFlags dstAccessMask, int byteOffset, int byteCount);
};

struct VulkanImageTransferCommand
{
	vk::Buffer buffer;
	vk::Image image;
	int width;
	int height;

	VulkanImageTransferCommand();

	void init(vk::Buffer buffer, vk::Image image, int width, int height);
};

using VulkanBufferTransferCommands = std::vector<VulkanBufferTransferCommand>;
using VulkanImageTransferCommands = std::vector<VulkanImageTransferCommand>;
using VulkanCommands = std::vector<std::function<void()>>;

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
	vk::Extent2D sceneViewExtent;
	vk::Extent2D internalExtent;
	vk::SwapchainKHR swapchain;
	std::vector<vk::Image> swapchainImages;
	Buffer<vk::ImageView> swapchainImageViews;

	static constexpr int MAX_SCENE_FRAMEBUFFERS = 2; // For ping-pong support.
	vk::DeviceMemory colorDeviceMemories[MAX_SCENE_FRAMEBUFFERS];
	vk::Image colorImages[MAX_SCENE_FRAMEBUFFERS];
	vk::ImageView colorImageViews[MAX_SCENE_FRAMEBUFFERS];
	vk::Sampler colorSampler;
	vk::DeviceMemory depthDeviceMemory;
	vk::Image depthImage;
	vk::ImageView depthImageView;
	vk::RenderPass sceneRenderPass;
	vk::RenderPass uiRenderPass;
	vk::Framebuffer sceneFramebuffers[MAX_SCENE_FRAMEBUFFERS];
	Buffer<vk::Framebuffer> uiFramebuffers;
	uint32_t prevAcquiredSwapchainImageIndex;

	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;
	VulkanBufferTransferCommands bufferTransferCommands;
	VulkanImageTransferCommands imageTransferCommands;
	VulkanCommands freeCommands;

	Buffer<VulkanVertexShader> vertexShaders;
	Buffer<VulkanFragmentShader> fragmentShaders;
	vk::ShaderModule lightBinningComputeShader;
	vk::ShaderModule conversionShader;

	vk::DescriptorPool globalDescriptorPool;
	vk::DescriptorPool transformDescriptorPool;
	vk::DescriptorPool materialDescriptorPool;
	vk::DescriptorSetLayout globalDescriptorSetLayout;
	vk::DescriptorSetLayout lightDescriptorSetLayout;
	vk::DescriptorSetLayout transformDescriptorSetLayout;
	vk::DescriptorSetLayout materialDescriptorSetLayout;
	vk::DescriptorSetLayout lightBinningDescriptorSetLayout;
	vk::DescriptorSetLayout conversionDescriptorSetLayout;
	vk::DescriptorSetLayout uiMaterialDescriptorSetLayout;
	vk::DescriptorSet globalDescriptorSets[MAX_SCENE_FRAMEBUFFERS]; // Two for ping-ponging sampled framebuffer.
	vk::DescriptorSet lightDescriptorSet;
	vk::DescriptorSet lightBinningDescriptorSet;
	vk::DescriptorSet conversionDescriptorSet;
	FlatMap<UiTextureID, vk::DescriptorSet> uiTextureDescriptorSets; // Avoids UI material support since UI is simplistic.

	Buffer<vk::PipelineLayout> pipelineLayouts;
	vk::PipelineLayout lightBinningPipelineLayout;
	Buffer<VulkanPipeline> graphicsPipelines;
	vk::Pipeline lightBinningPipeline;
	vk::Pipeline conversionPipeline;

	vk::Sampler textureSampler;

	vk::Semaphore imageIsAvailableSemaphore;
	vk::Semaphore renderIsFinishedSemaphore;

	VulkanVertexPositionBufferPool vertexPositionBufferPool;
	VulkanVertexAttributeBufferPool vertexAttributeBufferPool;
	VulkanIndexBufferPool indexBufferPool;
	VulkanUniformBufferPool uniformBufferPool;
	VulkanObjectTexturePool objectTexturePool;
	VulkanUiTexturePool uiTexturePool;
	VulkanMaterialPool materialPool;
	VulkanMaterialInstancePool materialInstPool;

	VulkanHeapManager vertexBufferHeapManagerDeviceLocal;
	VulkanHeapManager vertexBufferHeapManagerStaging;
	VulkanHeapManager indexBufferHeapManagerDeviceLocal;
	VulkanHeapManager indexBufferHeapManagerStaging;
	VulkanHeapManager uniformBufferHeapManagerDeviceLocal;
	VulkanHeapManager uniformBufferHeapManagerStaging;
	VulkanHeapManager storageBufferHeapManagerDeviceLocal;
	VulkanHeapManager storageBufferHeapManagerStaging;
	VulkanHeapManager objectTextureHeapManagerDeviceLocal;
	VulkanHeapManager objectTextureHeapManagerStaging;
	VulkanHeapManager uiTextureHeapManagerDeviceLocal;
	VulkanHeapManager uiTextureHeapManagerStaging;

	VulkanBuffer camera;
	VulkanBuffer framebufferDims;
	VulkanBuffer ambientLight;
	VulkanBuffer screenSpaceAnim;
	VulkanBuffer horizonMirror;
	VulkanBuffer optimizedVisibleLights;
	VulkanBuffer lightBins;
	VulkanBuffer lightBinLightCounts;
	VulkanBuffer lightBinDims;
	VulkanBuffer perPixelLightMode;
	VulkanBuffer perMeshLightMode;
	VertexPositionBufferID uiVertexPositionBufferID;
	VertexAttributeBufferID uiVertexAttributeBufferID;
	vk::Image dummyImage;
	vk::ImageView dummyImageView;

	RendererProfilerData2D profilerData2D;
	RendererProfilerData3D profilerData3D;
public:
	bool initContext(const RenderContextSettings &contextSettings) override;
	bool initRendering(const RenderInitSettings &initSettings) override;
	void shutdown() override;

	void resize(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight) override;
	void handleRenderTargetsReset(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight) override;

	RendererProfilerData2D getProfilerData2D() const override;
	RendererProfilerData3D getProfilerData3D() const override;

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

	RenderMaterialID createMaterial(RenderMaterialKey key) override;
	void freeMaterial(RenderMaterialID id) override;

	RenderMaterialInstanceID createMaterialInstance() override;
	void freeMaterialInstance(RenderMaterialInstanceID id) override;
	void setMaterialInstanceMeshLightPercent(RenderMaterialInstanceID id, double value) override;
	void setMaterialInstanceTexCoordAnimPercent(RenderMaterialInstanceID id, double value) override;

	void submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
		const RenderCamera &camera, const RenderFrameSettings &frameSettings) override;
};

#endif

#endif
