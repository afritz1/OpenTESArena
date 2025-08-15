#include <algorithm>
#include <fstream>
#include <functional>
#include <limits>
#include <string>

#include "SDL_vulkan.h"

#include "RenderBuffer.h"
#include "RenderCamera.h"
#include "RenderCommand.h"
#include "RenderDrawCall.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "RendererUtils.h"
#include "VulkanRenderBackend.h"
#include "Window.h"
#include "../UI/Surface.h"
#include "../World/MeshUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"
#include "components/utilities/File.h"
#include "components/utilities/StringView.h"

namespace
{
	constexpr uint32_t INVALID_UINT32 = std::numeric_limits<uint32_t>::max();
	constexpr uint64_t TIMEOUT_UNLIMITED = std::numeric_limits<uint64_t>::max();

	constexpr int HEAP_MAX_BYTES_VERTEX_BUFFER = 1 << 16; // 64KB
	constexpr int HEAP_MAX_BYTES_INDEX_BUFFER = HEAP_MAX_BYTES_VERTEX_BUFFER;
	constexpr int HEAP_MAX_BYTES_UNIFORM_BUFFER = 1 << 27; // 128MB (need lots for render transforms)

	constexpr vk::BufferUsageFlags VertexBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags VertexBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
	constexpr vk::BufferUsageFlags IndexBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags IndexBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
	constexpr vk::BufferUsageFlags UniformBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags UniformBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;

	struct Vertex
	{
		Float2 position;
		Float3 color;
	};
}

// Vulkan application
namespace
{
	constexpr uint32_t RequiredApiVersion = VK_API_VERSION_1_0;

	std::vector<const char*> GetInstanceValidationLayers()
	{
		vk::ResultValue<std::vector<vk::LayerProperties>> availableValidationLayersResult = vk::enumerateInstanceLayerProperties();
		if (availableValidationLayersResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't enumerate validation layers (%d).", availableValidationLayersResult.result);
			return std::vector<const char*>();
		}

		const std::vector<vk::LayerProperties> availableValidationLayers = std::move(availableValidationLayersResult.value);

		std::vector<const char*> validationLayers;

		bool supportsKhronosValidationLayer = false;
		const char *khronosValidationLayerName = "VK_LAYER_KHRONOS_validation";
		for (const vk::LayerProperties &layerProperties : availableValidationLayers)
		{
			if (StringView::equals(layerProperties.layerName, khronosValidationLayerName))
			{
				supportsKhronosValidationLayer = true;
				validationLayers.emplace_back(khronosValidationLayerName);
				break;
			}
		}

		if (!supportsKhronosValidationLayer)
		{
			DebugLogWarningFormat("%s not supported.", khronosValidationLayerName);
		}

		return validationLayers;
	}

	bool TryCreateVulkanInstance(SDL_Window *window, vk::Instance *outInstance)
	{
		uint32_t instanceExtensionCount;
		if (SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, nullptr) != SDL_TRUE)
		{
			DebugLogError("Couldn't get Vulkan instance extension count. Vulkan is not supported.");
			return false;
		}

		Buffer<const char*> instanceExtensions(instanceExtensionCount);
		if (SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, instanceExtensions.begin()) != SDL_TRUE)
		{
			DebugLogErrorFormat("Couldn't get Vulkan instance extensions (expected %d).", instanceExtensionCount);
			return false;
		}

		bool isMinimumRequiredSurfaceAvailable = false;
		for (const char *instanceExtensionName : instanceExtensions)
		{
			if (StringView::equals(instanceExtensionName, VK_KHR_SURFACE_EXTENSION_NAME))
			{
				isMinimumRequiredSurfaceAvailable = true;
				break;
			}
		}

		if (!isMinimumRequiredSurfaceAvailable)
		{
			DebugLogError("Vulkan is supported but no window surface is available.");
			return false;
		}

		vk::ApplicationInfo appInfo;
		appInfo.pApplicationName = "OpenTESArena";
		appInfo.applicationVersion = 0;
		appInfo.apiVersion = RequiredApiVersion;

		const std::vector<const char*> instanceValidationLayers = GetInstanceValidationLayers();

		vk::InstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceValidationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = instanceValidationLayers.data();
		instanceCreateInfo.enabledExtensionCount = instanceExtensions.getCount();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.begin();

		vk::ResultValue<vk::Instance> instanceCreateResult = vk::createInstance(instanceCreateInfo);
		if (instanceCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::Instance (%d).", instanceCreateResult.result);
			return false;
		}

		*outInstance = std::move(instanceCreateResult.value);
		return true;
	}
}

// Vulkan physical device
namespace
{
	vk::PhysicalDevice GetBestPhysicalDevice(Span<const vk::PhysicalDevice> physicalDevices)
	{
		if (physicalDevices.getCount() == 0)
		{
			DebugLogError("No physical devices to choose from.");
			return nullptr;
		}

		struct PhysicalDeviceEntry
		{
			int index;
			std::string name;
			vk::PhysicalDeviceType type;
			uint32_t vendorID;
			bool isDriverWrapper; // Vulkan-on-D3D12 etc.
			int score;
		};

		auto getDeviceEntryScore = [](const PhysicalDeviceEntry &entry)
		{
			int score = 0;

			switch (entry.type)
			{
			case vk::PhysicalDeviceType::eDiscreteGpu:
				score += 100;
				break;
			case vk::PhysicalDeviceType::eIntegratedGpu:
				score += 20;
				break;
			case vk::PhysicalDeviceType::eVirtualGpu:
				score += 10;
				break;
			case vk::PhysicalDeviceType::eCpu:
				score += 5;
				break;
			case vk::PhysicalDeviceType::eOther:
				score += 0;
				break;
			}

			constexpr uint32_t recognizedHardwareVendorIDs[] =
			{
				0x1002, // AMD
				0x10DE, // Nvidia
				0x8086, // Intel
				0x106B, // Apple M-series
				0x14E4 // Raspberry Pi
			};

			bool isRecognizedHardwareVendor = false;
			for (const uint32_t recognizedVendorID : recognizedHardwareVendorIDs)
			{
				if (entry.vendorID == recognizedVendorID)
				{
					isRecognizedHardwareVendor = true;
					break;
				}
			}

			if (isRecognizedHardwareVendor)
			{
				score *= 2;
			}

			if (entry.isDriverWrapper)
			{
				score /= 2;
			}

			return score;
		};

		std::vector<PhysicalDeviceEntry> entries;

		for (int i = 0; i < physicalDevices.getCount(); i++)
		{
			const vk::PhysicalDevice &physicalDevice = physicalDevices[i];
			const vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();
			const std::string deviceName = physicalDeviceProperties.deviceName;

			PhysicalDeviceEntry entry;
			entry.index = i;
			entry.name = deviceName;
			entry.type = physicalDeviceProperties.deviceType;
			entry.vendorID = physicalDeviceProperties.vendorID;

			bool isDriverWrapper = false;
			if ((deviceName.find("Microsoft") != std::string::npos) ||
				(deviceName.find("Direct3D") != std::string::npos) ||
				(deviceName.find("Basic Render Driver") != std::string::npos))
			{
				isDriverWrapper = true;
			}

			entry.isDriverWrapper = isDriverWrapper;
			entry.score = getDeviceEntryScore(entry);
			entries.emplace_back(std::move(entry));
		}

		constexpr const char *deviceTypeNames[] =
		{
			"Other", "Integrated GPU", "Discrete GPU", "Virtual GPU", "CPU"
		};

		DebugLog("Physical devices:");
		for (const PhysicalDeviceEntry &entry : entries)
		{
			const int deviceTypeIndex = static_cast<int>(entry.type);
			DebugAssertIndex(deviceTypeNames, deviceTypeIndex);
			const char *deviceTypeName = deviceTypeNames[deviceTypeIndex];
			DebugLogFormat("- %s | %s | Vendor: 0x%X", entry.name.c_str(), deviceTypeName, entry.vendorID);
		}

		std::sort(entries.begin(), entries.end(),
			[](const PhysicalDeviceEntry &a, const PhysicalDeviceEntry &b)
		{
			return a.score > b.score;
		});

		const PhysicalDeviceEntry &selectedEntry = entries[0];
		const vk::PhysicalDevice selectedPhysicalDevice = physicalDevices[selectedEntry.index];
		if (!selectedPhysicalDevice)
		{
			DebugLogError("No valid physical device available.");
			return nullptr;
		}

		DebugLogFormat("Selected: %s", selectedEntry.name.c_str());
		return selectedPhysicalDevice;
	}

	bool TryGetQueueFamilyIndices(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, uint32_t *outGraphicsQueueFamilyIndex, uint32_t *outPresentQueueFamilyIndex)
	{
		*outGraphicsQueueFamilyIndex = INVALID_UINT32;
		*outPresentQueueFamilyIndex = INVALID_UINT32;

		const std::vector<vk::QueueFamilyProperties> queueFamilyPropertiesList = physicalDevice.getQueueFamilyProperties();

		uint32_t graphicsQueueFamilyIndex = INVALID_UINT32;
		for (uint32_t i = 0; i < queueFamilyPropertiesList.size(); i++)
		{
			const vk::QueueFamilyProperties &queueFamilyProperties = queueFamilyPropertiesList[i];
			if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				graphicsQueueFamilyIndex = i;
				break;
			}
		}

		if (graphicsQueueFamilyIndex == INVALID_UINT32)
		{
			DebugLogErrorFormat("No graphics queue family index found.");
			return false;
		}

		uint32_t presentQueueFamilyIndex = INVALID_UINT32;
		for (uint32_t i = 0; i < queueFamilyPropertiesList.size(); i++)
		{
			vk::ResultValue<uint32_t> surfaceSupportResult = physicalDevice.getSurfaceSupportKHR(i, surface);
			if (surfaceSupportResult.result != vk::Result::eSuccess)
			{
				DebugLogErrorFormat("Couldn't query physical device getSurfaceSupportKHR() index %d (%d).", i, surfaceSupportResult.result);
				continue;
			}

			const VkBool32 isPresentSupported = surfaceSupportResult.value;
			if (isPresentSupported)
			{
				presentQueueFamilyIndex = i;
				if (i == graphicsQueueFamilyIndex)
				{
					// Queue family index is valid for graphics and presenting.
					break;
				}
			}
		}

		if (presentQueueFamilyIndex == INVALID_UINT32)
		{
			DebugLogErrorFormat("Couldn't find present queue family index.");
			return false;
		}

		*outGraphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
		*outPresentQueueFamilyIndex = presentQueueFamilyIndex;
		return true;
	}

	// Finds a memory type that satisfies device local, host visible (mappable to CPU), etc..
	uint32_t FindPhysicalDeviceMemoryTypeIndex(vk::PhysicalDevice physicalDevice, const vk::MemoryRequirements &memoryRequirements, vk::MemoryPropertyFlags flags)
	{
		const vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			const bool isRequiredMemoryType = (memoryRequirements.memoryTypeBits & (1 << i)) != 0;
			if (!isRequiredMemoryType)
			{
				continue;
			}

			const vk::MemoryType physicalDeviceMemoryType = physicalDeviceMemoryProperties.memoryTypes[i];
			const bool hasRequiredMemoryPropertyFlags = (physicalDeviceMemoryType.propertyFlags & flags) == flags;
			if (!hasRequiredMemoryPropertyFlags)
			{
				continue;
			}

			return i;
		}

		return INVALID_UINT32;
	}
}

// Vulkan buffers
namespace
{
	bool TryAllocateMemory(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, bool isHostVisible, vk::PhysicalDevice physicalDevice, vk::DeviceMemory *outDeviceMemory)
	{
		// Create dummy buffer for memory requirements.
		vk::BufferCreateInfo dummyBufferCreateInfo;
		dummyBufferCreateInfo.size = byteCount;
		dummyBufferCreateInfo.usage = usageFlags;
		dummyBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		dummyBufferCreateInfo.queueFamilyIndexCount = 0;
		dummyBufferCreateInfo.pQueueFamilyIndices = nullptr;

		vk::ResultValue<vk::Buffer> dummyBufferCreateResult = device.createBuffer(dummyBufferCreateInfo);
		if (dummyBufferCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create dummy vk::Buffer with %d bytes requirement (%d).", byteCount, dummyBufferCreateResult.result);
			return false;
		}

		vk::Buffer dummyBuffer = std::move(dummyBufferCreateResult.value);
		const vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(dummyBuffer);
		device.destroyBuffer(dummyBuffer);
		dummyBuffer = vk::Buffer(nullptr);

		const vk::MemoryPropertyFlags memoryPropertyFlags = isHostVisible ? (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) : vk::MemoryPropertyFlagBits::eDeviceLocal;

		vk::MemoryAllocateInfo memoryAllocateInfo;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = FindPhysicalDeviceMemoryTypeIndex(physicalDevice, memoryRequirements, memoryPropertyFlags);
		if (memoryAllocateInfo.memoryTypeIndex == INVALID_UINT32)
		{
			DebugLogErrorFormat("Couldn't find suitable memory type.");
			return false;
		}

		vk::ResultValue<vk::DeviceMemory> deviceMemoryCreateResult = device.allocateMemory(memoryAllocateInfo);
		if (deviceMemoryCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't allocate device memory for %d bytes (%d).", byteCount, deviceMemoryCreateResult.result);
			return false;
		}

		*outDeviceMemory = std::move(deviceMemoryCreateResult.value);
		return true;
	}

	bool TryCreateBuffer(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex, vk::Buffer *outBuffer)
	{
		vk::BufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.size = byteCount;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		bufferCreateInfo.queueFamilyIndexCount = 1;
		bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;

		vk::ResultValue<vk::Buffer> bufferCreateResult = device.createBuffer(bufferCreateInfo);
		if (bufferCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::Buffer with %d bytes requirement (%d).", byteCount, bufferCreateResult.result);
			return false;
		}

		*outBuffer = std::move(bufferCreateResult.value);
		return true;
	}

	template<typename T>
	bool TryCreateBuffer(vk::Device device, int elementCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex, vk::Buffer *outBuffer)
	{
		const int byteCount = elementCount * sizeof(T);
		return TryCreateBuffer(device, byteCount, usageFlags, queueFamilyIndex, outBuffer);
	}

	bool TryBindBufferToMemory(vk::Device device, vk::Buffer buffer, vk::DeviceMemory deviceMemory, int byteOffset)
	{
		const vk::Result bufferBindMemoryResult = device.bindBufferMemory(buffer, deviceMemory, byteOffset);
		if (bufferBindMemoryResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't bind buffer to device memory at byte offset %d (%d).", byteOffset, bufferBindMemoryResult);
			return false;
		}

		return true;
	}

	bool TryCreateBufferAndBindWithHeap(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex, VulkanHeap &heap,
		vk::Buffer *outBuffer, HeapBlock *outBlock, Span<std::byte> *outHostMappedBytes)
	{
		vk::Buffer buffer;
		if (!TryCreateBuffer(device, byteCount, usageFlags, queueFamilyIndex, &buffer))
		{
			DebugLogError("Couldn't create buffer with heap.");
			return false;
		}

		const vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);
		const HeapBlock block = heap.addMapping(buffer, memoryRequirements.size, memoryRequirements.alignment);
		if (!block.isValid())
		{
			DebugLogError("Couldn't add heap block mapping.");
			return false;
		}

		if (!TryBindBufferToMemory(device, buffer, heap.deviceMemory, block.offset))
		{
			DebugLogError("Couldn't bind buffer to heap memory.");
			return false;
		}

		*outBuffer = buffer;
		*outBlock = block;

		if (outHostMappedBytes)
		{
			*outHostMappedBytes = Span<std::byte>(heap.hostMappedBytes.begin() + block.offset, block.byteCount);
		}
		
		return true;
	}

	template<typename T>
	bool TryCreateBufferAndBindWithHeap(vk::Device device, int elementCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex, VulkanHeap &heap,
		vk::Buffer *outBuffer, HeapBlock *outBlock, Span<std::byte> *outHostMappedBytes)
	{
		const int byteCount = elementCount * sizeof(T);
		return TryCreateBufferAndBindWithHeap(device, byteCount, usageFlags, queueFamilyIndex, heap, outBuffer, outBlock, outHostMappedBytes);
	}

	bool TryMapMemory(vk::Device device, vk::DeviceMemory deviceMemory, int byteOffset, int byteCount, Span<std::byte> *outHostMappedBytes)
	{
		vk::ResultValue<void*> deviceMemoryMapResult = device.mapMemory(deviceMemory, byteOffset, byteCount);
		if (deviceMemoryMapResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't map device memory at byte offset %d with %d bytes.", byteOffset, byteCount);
			return false;
		}

		*outHostMappedBytes = Span<std::byte>(reinterpret_cast<std::byte*>(deviceMemoryMapResult.value), byteCount);
		return true;
	}

	// Assumes device memory has already been allocated but not mapped.
	bool TryCreateBufferAndMapMemory(vk::Device device, int byteOffset, int byteCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex,
		vk::DeviceMemory deviceMemory, vk::Buffer *outBuffer, Span<std::byte> *outHostMappedBytes)
	{
		vk::Buffer buffer;
		if (!TryCreateBuffer(device, byteCount, usageFlags, queueFamilyIndex, &buffer))
		{
			DebugLogErrorFormat("Couldn't create buffer with %d bytes.", byteCount);
			return false;
		}

		if (!TryBindBufferToMemory(device, buffer, deviceMemory, byteOffset))
		{
			DebugLogErrorFormat("Couldn't bind buffer to memory with %d bytes.", byteCount);
			return false;
		}

		Span<std::byte> hostMappedBytes;
		if (!TryMapMemory(device, deviceMemory, byteOffset, byteCount, &hostMappedBytes))
		{
			DebugLogErrorFormat("Couldn't map device memory for %d bytes.", byteCount);
			return false;
		}

		*outBuffer = buffer;
		*outHostMappedBytes = hostMappedBytes;
		return true;
	}

	void CopyToBufferDeviceLocal(vk::Buffer sourceBuffer, int sourceByteCount, vk::Buffer destinationBuffer, vk::CommandBuffer commandBuffer)
	{
		vk::BufferCopy bufferCopy;
		bufferCopy.srcOffset = 0;
		bufferCopy.dstOffset = 0;
		bufferCopy.size = sourceByteCount;

		commandBuffer.copyBuffer(sourceBuffer, destinationBuffer, bufferCopy);
	}

	bool TryCreateImage(vk::Device device, int width, int height, vk::Format format, vk::ImageUsageFlags usageFlags, uint32_t queueFamilyIndex,
		vk::PhysicalDevice physicalDevice, vk::Image *outImage, vk::DeviceMemory *outDeviceMemory)
	{
		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.imageType = vk::ImageType::e2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = vk::Extent3D(width, height, 1);
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
		imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
		imageCreateInfo.usage = usageFlags;
		imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		imageCreateInfo.queueFamilyIndexCount = 1;
		imageCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
		imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

		vk::ResultValue<vk::Image> createImageResult = device.createImage(imageCreateInfo);
		if (createImageResult.result != vk::Result::eSuccess)
		{
			DebugLogError("Couldn't create vk::Image.");
			return false;
		}

		const vk::Image image = std::move(createImageResult.value);
		const vk::MemoryRequirements imageMemoryRequirements = device.getImageMemoryRequirements(image);
		const vk::MemoryPropertyFlags imageMemoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

		vk::MemoryAllocateInfo imageMemoryAllocateInfo;
		imageMemoryAllocateInfo.allocationSize = imageMemoryRequirements.size;
		imageMemoryAllocateInfo.memoryTypeIndex = FindPhysicalDeviceMemoryTypeIndex(physicalDevice, imageMemoryRequirements, imageMemoryPropertyFlags);

		vk::ResultValue<vk::DeviceMemory> createImageMemoryResult = device.allocateMemory(imageMemoryAllocateInfo);
		if (createImageMemoryResult.result != vk::Result::eSuccess)
		{
			DebugLogError("Couldn't allocate vk::Image memory.");
			device.destroyImage(image);
			return false;
		}

		vk::DeviceMemory deviceMemory = std::move(createImageMemoryResult.value);

		const vk::Result imageBindMemoryResult = device.bindImageMemory(image, deviceMemory, 0);
		if (imageBindMemoryResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't bind device image memory (%d).", imageBindMemoryResult);
			device.freeMemory(deviceMemory);
			device.destroyImage(image);
			return false;
		}

		*outImage = image;
		*outDeviceMemory = deviceMemory;
		return true;
	}

	bool TryCreateImageView(vk::Device device, vk::Format format, vk::Image image, vk::ImageView *outImageView)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.components = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity };
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.image = image;

		vk::ResultValue<vk::ImageView> imageViewCreateResult = device.createImageView(imageViewCreateInfo);
		if (imageViewCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create image view (%d).", imageViewCreateResult.result);
			return false;
		}

		*outImageView = std::move(imageViewCreateResult.value);
		return true;
	}

	bool TryCreateSampler(vk::Device device, vk::Sampler *outSampler)
	{
		vk::SamplerCreateInfo samplerCreateInfo;
		samplerCreateInfo.magFilter = vk::Filter::eNearest;
		samplerCreateInfo.minFilter = vk::Filter::eNearest;
		samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
		samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.anisotropyEnable = false;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = vk::CompareOp::eAlways;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		vk::ResultValue<vk::Sampler> samplerCreateResult = device.createSampler(samplerCreateInfo);
		if (samplerCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::Sampler (%d).", samplerCreateResult.result);
			return false;
		}

		*outSampler = std::move(samplerCreateResult.value);
		return true;
	}

	void TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer commandBuffer)
	{
		const bool isTransitionToInitialPopulate = (oldLayout == vk::ImageLayout::eUndefined) && (newLayout == vk::ImageLayout::eTransferDstOptimal);
		const bool isTransitionToShaderReadOnly = (oldLayout == vk::ImageLayout::eTransferDstOptimal) && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::ImageMemoryBarrier imageMemoryBarrier;
		imageMemoryBarrier.oldLayout = oldLayout;
		imageMemoryBarrier.newLayout = newLayout;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		vk::PipelineStageFlags srcPipelineStageFlags;
		vk::PipelineStageFlags dstPipelineStageFlags;
		if (isTransitionToInitialPopulate)
		{
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eNone;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
			srcPipelineStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
			dstPipelineStageFlags = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (isTransitionToShaderReadOnly)
		{
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			srcPipelineStageFlags = vk::PipelineStageFlagBits::eTransfer;
			dstPipelineStageFlags = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else
		{
			DebugLogErrorFormat("Unsupported image layout transition %d -> %d.", oldLayout, newLayout);
			return;
		}

		vk::DependencyFlags dependencyFlags;
		vk::ArrayProxy<vk::MemoryBarrier> memoryBarrierArrayProxy; // Unused
		vk::ArrayProxy<vk::BufferMemoryBarrier> bufferMemoryBarrierArrayProxy; // Unused
		commandBuffer.pipelineBarrier(srcPipelineStageFlags, dstPipelineStageFlags, dependencyFlags, memoryBarrierArrayProxy, bufferMemoryBarrierArrayProxy, imageMemoryBarrier);
	}

	void CopyBufferToImage(vk::Buffer sourceBuffer, vk::Image destinationImage, int imageWidth, int imageHeight, vk::CommandBuffer commandBuffer)
	{
		const vk::ImageLayout imageLayout = vk::ImageLayout::eTransferDstOptimal;

		vk::BufferImageCopy bufferImageCopy;
		bufferImageCopy.bufferOffset = 0;
		bufferImageCopy.bufferRowLength = 0;
		bufferImageCopy.bufferImageHeight = 0;
		bufferImageCopy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		bufferImageCopy.imageSubresource.mipLevel = 0;
		bufferImageCopy.imageSubresource.baseArrayLayer = 0;
		bufferImageCopy.imageSubresource.layerCount = 1;
		bufferImageCopy.imageOffset = vk::Offset3D();
		bufferImageCopy.imageExtent = vk::Extent3D(imageWidth, imageHeight, 1);

		commandBuffer.copyBufferToImage(sourceBuffer, destinationImage, imageLayout, bufferImageCopy);
	}
}

// Vulkan device
namespace
{
	bool TryCreateDevice(vk::PhysicalDevice physicalDevice, uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex, vk::Device *outDevice)
	{
		Buffer<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;

		constexpr float deviceQueuePriority = 1.0f;
		if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
		{
			deviceQueueCreateInfos.init(2);

			vk::DeviceQueueCreateInfo &graphicsDeviceQueueCreateInfo = deviceQueueCreateInfos[0];
			graphicsDeviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
			graphicsDeviceQueueCreateInfo.queueCount = 1;
			graphicsDeviceQueueCreateInfo.pQueuePriorities = &deviceQueuePriority;

			vk::DeviceQueueCreateInfo &presentDeviceQueueCreateInfo = deviceQueueCreateInfos[1];
			presentDeviceQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex;
			presentDeviceQueueCreateInfo.queueCount = 1;
			presentDeviceQueueCreateInfo.pQueuePriorities = &deviceQueuePriority;
		}
		else
		{
			deviceQueueCreateInfos.init(1);

			vk::DeviceQueueCreateInfo &deviceQueueCreateInfo = deviceQueueCreateInfos[0];
			deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
			deviceQueueCreateInfo.queueCount = 1;
			deviceQueueCreateInfo.pQueuePriorities = &deviceQueuePriority;
		}

		Buffer<const char*> deviceExtensions(1);
		deviceExtensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.getCount();
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.begin();
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.getCount();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.begin();

		vk::ResultValue<vk::Device> deviceCreateResult = physicalDevice.createDevice(deviceCreateInfo);
		if (deviceCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::Device (%d).", deviceCreateResult.result);
			return false;
		}

		vk::Device device = std::move(deviceCreateResult.value);

		*outDevice = device;
		return true;
	}
}

// Vulkan swapchain
namespace
{
	bool TryGetSurfaceFormat(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, vk::Format format, vk::ColorSpaceKHR colorSpace, vk::SurfaceFormatKHR *outSurfaceFormat)
	{
		vk::ResultValue<std::vector<vk::SurfaceFormatKHR>> surfaceFormatsResult = physicalDevice.getSurfaceFormatsKHR(surface);
		if (surfaceFormatsResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't query physical device getSurfaceFormatsKHR() (%d).", surfaceFormatsResult.result);
			return false;
		}

		const std::vector<vk::SurfaceFormatKHR> surfaceFormats = std::move(surfaceFormatsResult.value);
		if (surfaceFormats.empty())
		{
			DebugLogErrorFormat("No surface formats available.");
			return false;
		}

		vk::SurfaceFormatKHR surfaceFormat = surfaceFormats[0];
		for (const vk::SurfaceFormatKHR currentSurfaceFormat : surfaceFormats)
		{
			if ((currentSurfaceFormat.format == format) && (currentSurfaceFormat.colorSpace == colorSpace))
			{
				surfaceFormat = currentSurfaceFormat;
				break;
			}
		}

		*outSurfaceFormat = surfaceFormat;
		return true;
	}

	bool TryGetSurfaceCapabilities(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, vk::SurfaceCapabilitiesKHR *outSurfaceCapabilities)
	{
		vk::ResultValue<vk::SurfaceCapabilitiesKHR> surfaceCapabilitiesResult = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		if (surfaceCapabilitiesResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't query physical device getSurfaceCapabilitiesKHR() (%d).", surfaceCapabilitiesResult.result);
			return false;
		}

		*outSurfaceCapabilities = std::move(surfaceCapabilitiesResult.value);
		return true;
	}

	bool TryGetSurfaceExtentForSwapchain(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities, SDL_Window *window, vk::Extent2D *outExtent)
	{
		vk::Extent2D extent = surfaceCapabilities.currentExtent;
		if (extent.width == INVALID_UINT32)
		{
			int windowWidth, windowHeight;
			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			extent.width = windowWidth;
			extent.height = windowHeight;
		}

		*outExtent = extent;
		return true;
	}

	uint32_t GetSurfaceImageCountForSwapchain(const vk::SurfaceCapabilitiesKHR &surfaceCapabilities)
	{
		uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
		if (surfaceCapabilities.maxImageCount > 0)
		{
			imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount);
		}

		return imageCount;
	}

	bool TryGetPresentModeOrDefault(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, vk::PresentModeKHR desiredPresentMode, vk::PresentModeKHR *outPresentMode)
	{
		vk::ResultValue<std::vector<vk::PresentModeKHR>> presentModesResult = physicalDevice.getSurfacePresentModesKHR(surface);
		if (presentModesResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't query physical device getSurfacePresentModesKHR() (%d).", presentModesResult.result);
			return false;
		}

		const std::vector<vk::PresentModeKHR> presentModes = std::move(presentModesResult.value);
		if (presentModes.empty())
		{
			DebugLogErrorFormat("No present modes available.");
			return false;
		}

		vk::PresentModeKHR presentMode = presentModes[0];
		for (const vk::PresentModeKHR currentPresentMode : presentModes)
		{
			if (currentPresentMode == desiredPresentMode)
			{
				presentMode = currentPresentMode;
				break;
			}
		}

		*outPresentMode = presentMode;
		return true;
	}

	bool TryCreateSwapchain(vk::Device device, vk::SurfaceKHR surface, vk::SurfaceFormatKHR surfaceFormat, vk::PresentModeKHR presentMode,
		const vk::SurfaceCapabilitiesKHR &surfaceCapabilities, vk::Extent2D surfaceExtent, uint32_t graphicsQueueFamilyIndex,
		uint32_t presentQueueFamilyIndex, vk::SwapchainKHR *outSwapchain)
	{
		const uint32_t imageCount = GetSurfaceImageCountForSwapchain(surfaceCapabilities);

		vk::SwapchainCreateInfoKHR swapchainCreateInfo;
		swapchainCreateInfo.surface = surface;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = surfaceExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

		if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
		{
			const uint32_t queueFamilyIndices[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		}

		swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = nullptr;

		vk::ResultValue<vk::SwapchainKHR> swapchainCreateResult = device.createSwapchainKHR(swapchainCreateInfo);
		if (swapchainCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create device swapchain (%d).", swapchainCreateResult.result);
			return false;
		}

		*outSwapchain = std::move(swapchainCreateResult.value);
		return true;
	}

	bool TryCreateSwapchainImageViews(vk::Device device, vk::SwapchainKHR swapchain, vk::SurfaceFormatKHR surfaceFormat, Buffer<vk::ImageView> *outImageViews)
	{
		vk::ResultValue<std::vector<vk::Image>> swapchainImagesResult = device.getSwapchainImagesKHR(swapchain);
		if (swapchainImagesResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't query device getSwapchainImagesKHR() (%d).", swapchainImagesResult.result);
			return false;
		}

		const std::vector<vk::Image> swapchainImages = std::move(swapchainImagesResult.value);
		if (swapchainImages.empty())
		{
			DebugLogErrorFormat("No swapchain images available.");
			return false;
		}

		outImageViews->init(swapchainImages.size());

		for (int i = 0; i < static_cast<int>(swapchainImages.size()); i++)
		{
			if (!TryCreateImageView(device, surfaceFormat.format, swapchainImages[i], &(*outImageViews)[i]))
			{
				DebugLogErrorFormat("Couldn't create swapchain image view index %d.", i);
				return false;
			}
		}

		return true;
	}

	bool TryCreateSwapchainRenderPass(vk::Device device, vk::SurfaceFormatKHR surfaceFormat, vk::RenderPass *outRenderPass)
	{
		vk::AttachmentDescription colorAttachmentDescription;
		colorAttachmentDescription.format = surfaceFormat.format;
		colorAttachmentDescription.samples = vk::SampleCountFlagBits::e1;
		colorAttachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachmentDescription.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference colorAttachmentReference;
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription colorAttachmentSubpassDescription;
		colorAttachmentSubpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		colorAttachmentSubpassDescription.colorAttachmentCount = 1;
		colorAttachmentSubpassDescription.pColorAttachments = &colorAttachmentReference;

		vk::SubpassDependency colorAttachmentSubpassDependency;
		colorAttachmentSubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		colorAttachmentSubpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		colorAttachmentSubpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		colorAttachmentSubpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

		vk::RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &colorAttachmentSubpassDescription;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &colorAttachmentSubpassDependency;

		vk::ResultValue<vk::RenderPass> renderPassCreateResult = device.createRenderPass(renderPassCreateInfo);
		if (renderPassCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create device render pass (%d).", renderPassCreateResult.result);
			return false;
		}

		*outRenderPass = std::move(renderPassCreateResult.value);
		return true;
	}

	bool TryCreateSwapchainFramebuffers(vk::Device device, Span<const vk::ImageView> swapchainImageViews, vk::Extent2D swapchainExtent,
		vk::RenderPass renderPass, Buffer<vk::Framebuffer> *outFramebuffers)
	{
		outFramebuffers->init(swapchainImageViews.getCount());

		for (int i = 0; i < swapchainImageViews.getCount(); i++)
		{
			vk::FramebufferCreateInfo framebufferCreateInfo;
			framebufferCreateInfo.renderPass = renderPass;
			framebufferCreateInfo.attachmentCount = 1;
			framebufferCreateInfo.pAttachments = &swapchainImageViews[i];
			framebufferCreateInfo.width = swapchainExtent.width;
			framebufferCreateInfo.height = swapchainExtent.height;
			framebufferCreateInfo.layers = 1;

			vk::ResultValue<vk::Framebuffer> framebufferCreateResult = device.createFramebuffer(framebufferCreateInfo);
			if (framebufferCreateResult.result != vk::Result::eSuccess)
			{
				DebugLogErrorFormat("Couldn't create device framebuffer index %d (%d).", i, framebufferCreateResult.result);
				return false;
			}

			(*outFramebuffers)[i] = std::move(framebufferCreateResult.value);
		}

		return true;
	}
}

// Vulkan command buffers
namespace
{
	bool TryCreateCommandPool(vk::Device device, uint32_t queueFamilyIndex, vk::CommandPool *outCommandPool)
	{
		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
		commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

		vk::ResultValue<vk::CommandPool> commandPoolCreateResult = device.createCommandPool(commandPoolCreateInfo);
		if (commandPoolCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::CommandPool (%d).", commandPoolCreateResult.result);
			return false;
		}

		*outCommandPool = std::move(commandPoolCreateResult.value);
		return true;
	}

	bool TryCreateCommandBuffer(vk::Device device, vk::CommandPool commandPool, vk::CommandBuffer *outCommandBuffer)
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.commandPool = commandPool;
		commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
		commandBufferAllocateInfo.commandBufferCount = 1;

		vk::ResultValue<std::vector<vk::CommandBuffer>> commandBufferAllocateResult = device.allocateCommandBuffers(commandBufferAllocateInfo);
		if (commandBufferAllocateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::CommandBuffer list (%d).", commandBufferAllocateResult.result);
			return false;
		}

		const std::vector<vk::CommandBuffer> commandBuffers = std::move(commandBufferAllocateResult.value);
		if (commandBuffers.empty())
		{
			DebugLogError("No command buffers allocated.");
			return false;
		}

		*outCommandBuffer = commandBuffers[0];
		return true;
	}

	bool TrySubmitCommandBufferOnce(const std::function<void()> &func, vk::CommandBuffer commandBuffer, vk::Queue queue)
	{
		const vk::Result commandBufferResetResult = commandBuffer.reset();
		if (commandBufferResetResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't reset command buffer one-time command (%d).", commandBufferResetResult);
			return false;
		}

		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		const vk::Result commandBufferBeginResult = commandBuffer.begin(commandBufferBeginInfo);
		if (commandBufferBeginResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't begin command buffer for one-time command (%d).", commandBufferBeginResult);
			return false;
		}

		func();

		const vk::Result commandBufferEndResult = commandBuffer.end();
		if (commandBufferEndResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't end command buffer for one-time command (%d).", commandBufferEndResult);
			return false;
		}

		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		const vk::Result queueSubmitResult = queue.submit(submitInfo);
		if (queueSubmitResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't submit queue for one-time command (%d).", queueSubmitResult);
			return false;
		}

		const vk::Result waitForCopyCompletionResult = queue.waitIdle();
		if (waitForCopyCompletionResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't wait idle for one-time command (%d).", waitForCopyCompletionResult);
			return false;
		}

		return true;
	}
}

// Vulkan synchronization
namespace
{
	bool TryCreateSemaphore(vk::Device device, vk::Semaphore *outSemaphore)
	{
		vk::SemaphoreCreateInfo semaphoreCreateInfo;
		vk::ResultValue<vk::Semaphore> createSemaphoreResult = device.createSemaphore(semaphoreCreateInfo);
		if (createSemaphoreResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create semaphore (%d).", createSemaphoreResult.result);
			return false;
		}

		*outSemaphore = std::move(createSemaphoreResult.value);
		return true;
	}
}

// Vulkan shaders
namespace
{
	bool TryCreateShaderModule(vk::Device device, const char *filename, vk::ShaderModule *outShaderModule)
	{
		const Buffer<std::byte> shaderBytes = File::readAllBytes(filename);

		vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
		shaderModuleCreateInfo.codeSize = shaderBytes.getCount();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBytes.begin());

		vk::ResultValue<vk::ShaderModule> shaderModuleResult = device.createShaderModule(shaderModuleCreateInfo);
		if (shaderModuleResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create shader module from \"%s\" (%d).", filename, shaderModuleResult.result);
			return false;
		}

		*outShaderModule = std::move(shaderModuleResult.value);
		return true;
	}
}

// Vulkan descriptor sets
namespace
{
	bool TryCreateDescriptorSetLayout(vk::Device device, vk::DescriptorSetLayout *outDescriptorSetLayout)
	{
		vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding[3];

		vk::DescriptorSetLayoutBinding &cameraDescriptorSetLayoutBinding = descriptorSetLayoutBinding[0];
		cameraDescriptorSetLayoutBinding.binding = 0;
		cameraDescriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
		cameraDescriptorSetLayoutBinding.descriptorCount = 1;
		cameraDescriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
		cameraDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		vk::DescriptorSetLayoutBinding &textureDescriptorSetLayoutBinding = descriptorSetLayoutBinding[1];
		textureDescriptorSetLayoutBinding.binding = 1;
		textureDescriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		textureDescriptorSetLayoutBinding.descriptorCount = 1;
		textureDescriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		textureDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		vk::DescriptorSetLayoutBinding &paletteDescriptorSetLayoutBinding = descriptorSetLayoutBinding[2];
		paletteDescriptorSetLayoutBinding.binding = 2;
		paletteDescriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		paletteDescriptorSetLayoutBinding.descriptorCount = 1;
		paletteDescriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		paletteDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(std::size(descriptorSetLayoutBinding));
		descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBinding;

		vk::ResultValue<vk::DescriptorSetLayout> descriptorSetLayoutCreateResult = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
		if (descriptorSetLayoutCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::DescriptorSetLayout (%d).", descriptorSetLayoutCreateResult.result);
			return false;
		}

		*outDescriptorSetLayout = std::move(descriptorSetLayoutCreateResult.value);
		return true;
	}

	bool TryCreateDescriptorPool(vk::Device device, vk::DescriptorPool *outDescriptorPool)
	{
		vk::DescriptorPoolSize descriptorPoolSizes[2];

		vk::DescriptorPoolSize &cameraDescriptorPoolSize = descriptorPoolSizes[0];
		cameraDescriptorPoolSize.type = vk::DescriptorType::eUniformBuffer;
		cameraDescriptorPoolSize.descriptorCount = 1;

		vk::DescriptorPoolSize &samplerDescriptorPoolSize = descriptorPoolSizes[1];
		samplerDescriptorPoolSize.type = vk::DescriptorType::eCombinedImageSampler;
		samplerDescriptorPoolSize.descriptorCount = 2;

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.poolSizeCount = static_cast<int>(std::size(descriptorPoolSizes));
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

		vk::ResultValue<vk::DescriptorPool> descriptorPoolCreateResult = device.createDescriptorPool(descriptorPoolCreateInfo);
		if (descriptorPoolCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::DescriptorPool (%d).", descriptorPoolCreateResult.result);
			return false;
		}

		*outDescriptorPool = std::move(descriptorPoolCreateResult.value);
		return true;
	}

	bool TryCreateDescriptorSet(vk::Device device, vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorPool descriptorPool, vk::DescriptorSet *outDescriptorSet)
	{
		vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
		descriptorSetAllocateInfo.descriptorPool = descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

		vk::ResultValue<std::vector<vk::DescriptorSet>> descriptorSetCreateResult = device.allocateDescriptorSets(descriptorSetAllocateInfo);
		if (descriptorSetCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't allocate descriptor set (%d).", descriptorSetCreateResult.result);
			return false;
		}

		std::vector<vk::DescriptorSet> descriptorSets = std::move(descriptorSetCreateResult.value);
		if (descriptorSets.empty())
		{
			DebugLogError("Couldn't allocate any desecriptor sets.");
			return false;
		}

		*outDescriptorSet = descriptorSets[0];
		return true;
	}

	void UpdateDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::Buffer cameraBuffer,
		vk::ImageView textureImageView, vk::Sampler textureSampler, vk::ImageView paletteImageView, vk::Sampler paletteSampler)
	{
		vk::DescriptorBufferInfo cameraDescriptorBufferInfo;
		cameraDescriptorBufferInfo.buffer = cameraBuffer;
		cameraDescriptorBufferInfo.offset = 0;
		cameraDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorImageInfo textureDescriptorImageInfo;
		textureDescriptorImageInfo.sampler = textureSampler;
		textureDescriptorImageInfo.imageView = textureImageView;
		textureDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::DescriptorImageInfo paletteDescriptorImageInfo;
		paletteDescriptorImageInfo.sampler = paletteSampler;
		paletteDescriptorImageInfo.imageView = paletteImageView;
		paletteDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::WriteDescriptorSet writeDescriptorSets[3];

		vk::WriteDescriptorSet &cameraWriteDescriptorSet = writeDescriptorSets[0];
		cameraWriteDescriptorSet.dstSet = descriptorSet;
		cameraWriteDescriptorSet.dstBinding = 0;
		cameraWriteDescriptorSet.dstArrayElement = 0;
		cameraWriteDescriptorSet.descriptorCount = 1;
		cameraWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		cameraWriteDescriptorSet.pBufferInfo = &cameraDescriptorBufferInfo;

		vk::WriteDescriptorSet &textureWriteDescriptorSet = writeDescriptorSets[1];
		textureWriteDescriptorSet.dstSet = descriptorSet;
		textureWriteDescriptorSet.dstBinding = 1;
		textureWriteDescriptorSet.dstArrayElement = 0;
		textureWriteDescriptorSet.descriptorCount = 1;
		textureWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		textureWriteDescriptorSet.pImageInfo = &textureDescriptorImageInfo;

		vk::WriteDescriptorSet &paletteWriteDescriptorSet = writeDescriptorSets[2];
		paletteWriteDescriptorSet.dstSet = descriptorSet;
		paletteWriteDescriptorSet.dstBinding = 2;
		paletteWriteDescriptorSet.dstArrayElement = 0;
		paletteWriteDescriptorSet.descriptorCount = 1;
		paletteWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		paletteWriteDescriptorSet.pImageInfo = &paletteDescriptorImageInfo;

		vk::ArrayProxy<vk::CopyDescriptorSet> copyDescriptorSets;
		device.updateDescriptorSets(writeDescriptorSets, copyDescriptorSets);
	}
}

// Vulkan pipelines
namespace
{
	bool TryCreatePipelineLayout(vk::Device device, vk::DescriptorSetLayout descriptorSetLayout, vk::PipelineLayout *outPipelineLayout)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

		vk::ResultValue<vk::PipelineLayout> pipelineLayoutResult = device.createPipelineLayout(pipelineLayoutCreateInfo);
		if (pipelineLayoutResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create device vk::PipelineLayout (%d).", pipelineLayoutResult.result);
			return false;
		}

		*outPipelineLayout = std::move(pipelineLayoutResult.value);
		return true;
	}

	bool TryCreateGraphicsPipeline(vk::Device device, vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule, vk::Extent2D swapchainExtent,
		vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, vk::Pipeline *outPipeline)
	{
		vk::PipelineShaderStageCreateInfo vertexPipelineShaderStageCreateInfo;
		vertexPipelineShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertexPipelineShaderStageCreateInfo.module = vertexShaderModule;
		vertexPipelineShaderStageCreateInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo fragmentPipelineShaderStageCreateInfo;
		fragmentPipelineShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragmentPipelineShaderStageCreateInfo.module = fragmentShaderModule;
		fragmentPipelineShaderStageCreateInfo.pName = "main";

		const vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] =
		{
			vertexPipelineShaderStageCreateInfo,
			fragmentPipelineShaderStageCreateInfo
		};

		vk::VertexInputBindingDescription positionVertexInputBindingDescription;
		positionVertexInputBindingDescription.binding = 0;
		positionVertexInputBindingDescription.stride = static_cast<uint32_t>(MeshUtils::POSITION_COMPONENTS_PER_VERTEX * MeshUtils::POSITION_COMPONENT_SIZE_FLOAT);
		positionVertexInputBindingDescription.inputRate = vk::VertexInputRate::eVertex;

		vk::VertexInputAttributeDescription positionVertexInputAttributeDescription;
		positionVertexInputAttributeDescription.location = 0;
		positionVertexInputAttributeDescription.binding = 0;
		positionVertexInputAttributeDescription.format = vk::Format::eR32G32B32Sfloat;
		positionVertexInputAttributeDescription.offset = 0;

		vk::VertexInputBindingDescription texCoordVertexInputBindingDescription;
		texCoordVertexInputBindingDescription.binding = 1;
		texCoordVertexInputBindingDescription.stride = static_cast<uint32_t>(MeshUtils::TEX_COORD_COMPONENTS_PER_VERTEX * MeshUtils::TEX_COORD_COMPONENT_SIZE_FLOAT);
		texCoordVertexInputBindingDescription.inputRate = vk::VertexInputRate::eVertex;

		vk::VertexInputAttributeDescription texCoordVertexInputAttributeDescription;
		texCoordVertexInputAttributeDescription.location = 1;
		texCoordVertexInputAttributeDescription.binding = 1;
		texCoordVertexInputAttributeDescription.format = vk::Format::eR32G32Sfloat;
		texCoordVertexInputAttributeDescription.offset = 0;

		const vk::VertexInputBindingDescription vertexInputBindingDescriptions[] =
		{
			positionVertexInputBindingDescription,
			texCoordVertexInputBindingDescription
		};

		const vk::VertexInputAttributeDescription vertexInputAttributeDescriptions[] =
		{
			positionVertexInputAttributeDescription,
			texCoordVertexInputAttributeDescription
		};

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
		pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(std::size(vertexInputBindingDescriptions));
		pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions;
		pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(std::size(vertexInputAttributeDescriptions));
		pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions;

		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
		pipelineInputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
		pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		vk::Viewport viewport;
		viewport.width = static_cast<float>(swapchainExtent.width);
		viewport.height = static_cast<float>(swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::Rect2D viewportScissor;
		viewportScissor.extent = swapchainExtent;

		vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
		pipelineViewportStateCreateInfo.viewportCount = 1;
		pipelineViewportStateCreateInfo.pViewports = &viewport;
		pipelineViewportStateCreateInfo.scissorCount = 1;
		pipelineViewportStateCreateInfo.pScissors = &viewportScissor;

		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
		pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
		pipelineRasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
		pipelineRasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;
		pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;

		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
		pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
		pipelineColorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
		pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		pipelineColorBlendStateCreateInfo.attachmentCount = 1;
		pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
		graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(std::size(pipelineShaderStageCreateInfos));
		graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos;
		graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
		graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.layout = pipelineLayout;
		graphicsPipelineCreateInfo.renderPass = renderPass;
		graphicsPipelineCreateInfo.subpass = 0;
		graphicsPipelineCreateInfo.basePipelineHandle = nullptr;

		vk::PipelineCache pipelineCache;
		vk::ResultValue<vk::Pipeline> graphicsPipelineResult = device.createGraphicsPipeline(pipelineCache, graphicsPipelineCreateInfo);
		if (graphicsPipelineResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create device graphics pipeline (%d).", graphicsPipelineResult.result);
			return false;
		}

		*outPipeline = std::move(graphicsPipelineResult.value);
		return true;
	}
}

VulkanBuffer::VulkanBuffer()
{
	this->type = static_cast<VulkanBufferType>(-1);
}

void VulkanBuffer::init(vk::Buffer buffer, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes)
{
	this->buffer = buffer;
	this->stagingBuffer = stagingBuffer;
	this->stagingHostMappedBytes = stagingHostMappedBytes;
}

void VulkanBuffer::initVertexPosition(int vertexCount, int componentsPerVertex, int bytesPerComponent)
{
	this->type = VulkanBufferType::VertexPosition;
	this->vertexPosition.vertexCount = vertexCount;
	this->vertexPosition.componentsPerVertex = componentsPerVertex;
	this->vertexPosition.bytesPerComponent = bytesPerComponent;
}

void VulkanBuffer::initVertexAttribute(int vertexCount, int componentsPerVertex, int bytesPerComponent)
{
	this->type = VulkanBufferType::VertexAttribute;
	this->vertexAttribute.vertexCount = vertexCount;
	this->vertexAttribute.componentsPerVertex = componentsPerVertex;
	this->vertexAttribute.bytesPerComponent = bytesPerComponent;
}

void VulkanBuffer::initIndex(int indexCount, int bytesPerIndex)
{
	this->type = VulkanBufferType::Index;
	this->index.indexCount = indexCount;
	this->index.bytesPerIndex = bytesPerIndex;
}

void VulkanBuffer::initUniform(int elementCount, int bytesPerElement, int alignmentOfElement)
{
	this->type = VulkanBufferType::Uniform;
	this->uniform.elementCount = elementCount;
	this->uniform.bytesPerElement = bytesPerElement;
	this->uniform.alignmentOfElement = alignmentOfElement;
}

VulkanLightInfo::VulkanLightInfo()
{
	this->pointX = 0.0f;
	this->pointY = 0.0f;
	this->pointZ = 0.0f;
	this->startRadius = 0.0f;
	this->endRadius = 0.0f;
	this->startRadiusSqr = 0.0f;
	this->endRadiusSqr = 0.0f;
}

void VulkanLightInfo::init(float pointX, float pointY, float pointZ, float startRadius, float endRadius)
{
	this->pointX = pointX;
	this->pointY = pointY;
	this->pointZ = pointZ;
	this->startRadius = startRadius;
	this->endRadius = endRadius;
	this->startRadiusSqr = startRadius * startRadius;
	this->endRadiusSqr = endRadius * endRadius;
}

void VulkanLight::init(float pointX, float pointY, float pointZ, float startRadius, float endRadius, vk::Buffer buffer, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes)
{
	this->buffer = buffer;
	this->stagingBuffer = stagingBuffer;
	this->stagingHostMappedBytes = stagingHostMappedBytes;

	DebugAssert(endRadius >= startRadius);
	this->lightInfo.init(pointX, pointY, pointZ, startRadius, endRadius);
}

VulkanTexture::VulkanTexture()
{
	this->width = 0;
	this->height = 0;
	this->bytesPerTexel = 0;
}

void VulkanTexture::init(int width, int height, int bytesPerTexel, vk::Image image, vk::DeviceMemory deviceMemory, vk::ImageView imageView, vk::Sampler sampler,
	vk::DeviceMemory stagingDeviceMemory, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(bytesPerTexel > 0);
	this->width = width;
	this->height = height;
	this->bytesPerTexel = bytesPerTexel;
	this->image = image;
	this->deviceMemory = deviceMemory; // @todo device memory is more important than the buffer, it should come first
	this->imageView = imageView;
	this->sampler = sampler;
	this->stagingDeviceMemory = stagingDeviceMemory;
	this->stagingBuffer = stagingBuffer;
	this->stagingHostMappedBytes = stagingHostMappedBytes;
}

VulkanObjectTextureAllocator::VulkanObjectTextureAllocator()
{
	this->pool = nullptr;
	this->queueFamilyIndex = INVALID_UINT32;
}

void VulkanObjectTextureAllocator::init(VulkanObjectTexturePool *pool, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
	vk::Device device, vk::Queue queue, vk::CommandBuffer commandBuffer)
{
	this->pool = pool;
	this->physicalDevice = physicalDevice;
	this->queueFamilyIndex = queueFamilyIndex;
	this->device = device;
	this->queue = queue;
	this->commandBuffer = commandBuffer;
}

ObjectTextureID VulkanObjectTextureAllocator::create(int width, int height, int bytesPerTexel)
{
	const ObjectTextureID textureID = this->pool->alloc();
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate object texture with dims %dx%d and %d bytes per texel.", width, height, bytesPerTexel);
		return -1;
	}

	const vk::Format format = (bytesPerTexel == 1) ? vk::Format::eR8Uint : vk::Format::eR8G8B8A8Uint;
	constexpr vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

	vk::Image image;
	vk::DeviceMemory deviceMemory;
	if (!TryCreateImage(this->device, width, height, format, usageFlags, this->queueFamilyIndex, this->physicalDevice, &image, &deviceMemory))
	{
		DebugLogErrorFormat("Couldn't create image with dims %dx%d.", width, height);
		this->pool->free(textureID);
		return -1;
	}

	vk::ImageView imageView;
	if (!TryCreateImageView(this->device, format, image, &imageView))
	{
		DebugLogErrorFormat("Couldn't create image view with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	vk::Sampler sampler;
	if (!TryCreateSampler(this->device, &sampler))
	{
		DebugLogErrorFormat("Couldn't create sampler for image with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	constexpr vk::BufferUsageFlags stagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	const int byteCount = width * height * bytesPerTexel;
	vk::DeviceMemory stagingDeviceMemory;
	if (!TryAllocateMemory(this->device, byteCount, stagingUsageFlags, true, this->physicalDevice, &stagingDeviceMemory))
	{
		DebugLogErrorFormat("Couldn't allocate memory for object texture with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	constexpr int stagingByteOffset = 0;
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndMapMemory(this->device, stagingByteOffset, byteCount, stagingUsageFlags, this->queueFamilyIndex, stagingDeviceMemory, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create buffer and map memory for object texture with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	VulkanTexture &texture = this->pool->get(textureID);
	texture.init(width, height, bytesPerTexel, image, deviceMemory, imageView, sampler, stagingDeviceMemory, stagingBuffer, stagingHostMappedBytes);

	return textureID;
}

void VulkanObjectTextureAllocator::free(ObjectTextureID textureID)
{
	VulkanTexture *texture = this->pool->tryGet(textureID);
	if (texture != nullptr)
	{
		if (texture->stagingDeviceMemory)
		{
			this->device.freeMemory(texture->stagingDeviceMemory);
		}

		if (texture->stagingBuffer)
		{
			this->device.destroyBuffer(texture->stagingBuffer);
		}

		if (texture->sampler)
		{
			this->device.destroySampler(texture->sampler);
		}

		if (texture->imageView)
		{
			this->device.destroyImageView(texture->imageView);
		}

		if (texture->deviceMemory)
		{
			this->device.freeMemory(texture->deviceMemory);
		}

		if (texture->image)
		{
			this->device.destroyImage(texture->image);
		}
	}

	this->pool->free(textureID);
}

std::optional<Int2> VulkanObjectTextureAllocator::tryGetDimensions(ObjectTextureID id) const
{
	const VulkanTexture *texture = this->pool->tryGet(id);
	if (texture == nullptr)
	{
		return std::nullopt;
	}

	return Int2(texture->width, texture->height);
}

LockedTexture VulkanObjectTextureAllocator::lock(ObjectTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);
	return LockedTexture(texture.stagingHostMappedBytes, texture.width, texture.height, texture.bytesPerTexel);
}

void VulkanObjectTextureAllocator::unlock(ObjectTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);
	const int width = texture.width;
	const int height = texture.height;
	const int bytesPerTexel = texture.bytesPerTexel;

	auto commandBufferFunc = [this, &texture, width, height]()
	{
		vk::Image image = texture.image;
		TransitionImageLayout(image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, this->commandBuffer);
		CopyBufferToImage(texture.stagingBuffer, image, width, height, this->commandBuffer);
		TransitionImageLayout(image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->queue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for object texture unlock with dims %dx%d and %d bytes per texel (%d).", width, height, bytesPerTexel);
		return;
	}
}

VulkanUiTextureAllocator::VulkanUiTextureAllocator()
{
	this->pool = nullptr;
	this->queueFamilyIndex = INVALID_UINT32;
}

void VulkanUiTextureAllocator::init(VulkanUiTexturePool *pool, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex, vk::Device device, vk::Queue queue, vk::CommandBuffer commandBuffer)
{
	this->pool = pool;
	this->physicalDevice = physicalDevice;
	this->queueFamilyIndex = queueFamilyIndex;
	this->device = device;
	this->queue = queue;
	this->commandBuffer = commandBuffer;
}

UiTextureID VulkanUiTextureAllocator::create(int width, int height)
{
	const UiTextureID textureID = this->pool->alloc();
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate UI texture with dims %dx%d.", width, height);
		return -1;
	}

	constexpr int bytesPerTexel = 4;
	constexpr vk::Format format = vk::Format::eR8G8B8A8Unorm;
	constexpr vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

	vk::Image image;
	vk::DeviceMemory deviceMemory;
	if (!TryCreateImage(this->device, width, height, format, usageFlags, this->queueFamilyIndex, this->physicalDevice, &image, &deviceMemory))
	{
		DebugLogErrorFormat("Couldn't create image with dims %dx%d.", width, height);
		this->pool->free(textureID);
		return -1;
	}

	vk::ImageView imageView;
	if (!TryCreateImageView(this->device, format, image, &imageView))
	{
		DebugLogErrorFormat("Couldn't create image view with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	vk::Sampler sampler;
	if (!TryCreateSampler(this->device, &sampler))
	{
		DebugLogErrorFormat("Couldn't create sampler for image with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	constexpr vk::BufferUsageFlags stagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	const int byteCount = width * height * bytesPerTexel;
	vk::DeviceMemory stagingDeviceMemory;
	if (!TryAllocateMemory(this->device, byteCount, stagingUsageFlags, true, this->physicalDevice, &stagingDeviceMemory))
	{
		DebugLogErrorFormat("Couldn't allocate memory for UI texture with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	constexpr int stagingByteOffset = 0;
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndMapMemory(this->device, stagingByteOffset, byteCount, stagingUsageFlags, this->queueFamilyIndex, stagingDeviceMemory, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create buffer and map memory for UI texture with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	VulkanTexture &texture = this->pool->get(textureID);
	texture.init(width, height, bytesPerTexel, image, deviceMemory, imageView, sampler, stagingDeviceMemory, stagingBuffer, stagingHostMappedBytes);

	return textureID;
}

void VulkanUiTextureAllocator::free(UiTextureID textureID)
{
	VulkanTexture *texture = this->pool->tryGet(textureID);
	if (texture != nullptr)
	{
		if (texture->stagingDeviceMemory)
		{
			this->device.freeMemory(texture->stagingDeviceMemory);
		}

		if (texture->stagingBuffer)
		{
			this->device.destroyBuffer(texture->stagingBuffer);
		}

		if (texture->sampler)
		{
			this->device.destroySampler(texture->sampler);
		}

		if (texture->imageView)
		{
			this->device.destroyImageView(texture->imageView);
		}

		if (texture->deviceMemory)
		{
			this->device.freeMemory(texture->deviceMemory);
		}

		if (texture->image)
		{
			this->device.destroyImage(texture->image);
		}
	}

	this->pool->free(textureID);
}

std::optional<Int2> VulkanUiTextureAllocator::tryGetDimensions(UiTextureID id) const
{
	const VulkanTexture *texture = this->pool->tryGet(id);
	if (texture == nullptr)
	{
		return std::nullopt;
	}

	return Int2(texture->width, texture->height);
}

LockedTexture VulkanUiTextureAllocator::lock(UiTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);
	return LockedTexture(texture.stagingHostMappedBytes, texture.width, texture.height, texture.bytesPerTexel);
}

void VulkanUiTextureAllocator::unlock(UiTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);
	const int width = texture.width;
	const int height = texture.height;
	DebugAssert(texture.bytesPerTexel == 4);

	auto commandBufferFunc = [this, &texture, width, height]()
	{
		vk::Image image = texture.image;
		TransitionImageLayout(image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, this->commandBuffer);
		CopyBufferToImage(texture.stagingBuffer, image, width, height, this->commandBuffer);
		TransitionImageLayout(image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->queue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for UI texture unlock with dims %dx%d (%d).", width, height);
		return;
	}
}

VulkanCamera::VulkanCamera()
{
	this->matrixBytes = Span<const std::byte>(reinterpret_cast<const std::byte*>(&this->model), VulkanCamera::BYTE_COUNT);
}

void VulkanCamera::init(vk::Buffer buffer, vk::DeviceMemory deviceMemory, Span<std::byte> hostMappedBytes)
{
	this->buffer = buffer;
	this->deviceMemory = deviceMemory;
	this->hostMappedBytes = hostMappedBytes;
}

bool VulkanHeap::init(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, bool isHostVisible, vk::PhysicalDevice physicalDevice)
{
	if (!TryAllocateMemory(device, byteCount, usageFlags, isHostVisible, physicalDevice, &this->deviceMemory))
	{
		DebugLogErrorFormat("Couldn't allocate memory for heap with %d bytes.", byteCount);
		return false;
	}

	if (isHostVisible)
	{
		if (!TryMapMemory(device, this->deviceMemory, 0, byteCount, &this->hostMappedBytes))
		{
			DebugLogErrorFormat("Couldn't map memory for heap with %d bytes.", byteCount);
			device.freeMemory(this->deviceMemory);
			this->deviceMemory = vk::DeviceMemory(nullptr);
			return false;
		}
	}

	this->allocator.init(0, byteCount);
	return true;
}

HeapBlock VulkanHeap::addMapping(vk::Buffer buffer, int byteCount, int alignment)
{
	for (const VulkanHeapMapping &mapping : this->bufferMappings)
	{
		if (mapping.buffer == buffer)
		{
			DebugLogError("Heap buffer mapping already exists.");
			return HeapBlock();
		}
	}

	const HeapBlock block = this->allocator.alloc(byteCount, alignment);
	if (!block.isValid())
	{
		DebugLogWarningFormat("Couldn't allocate block for buffer mapping with %d bytes and alignment %d.", byteCount, alignment);
		return block;
	}

	VulkanHeapMapping mapping;
	mapping.buffer = buffer;
	mapping.block = block;
	this->bufferMappings.emplace_back(std::move(mapping));

	return block;
}

void VulkanHeap::freeMapping(vk::Buffer buffer)
{
	int index = -1;
	for (int i = 0; i < static_cast<int>(this->bufferMappings.size()); i++)
	{
		const VulkanHeapMapping &mapping = this->bufferMappings[i];
		if (mapping.buffer == buffer)
		{
			index = i;
			break;
		}
	}

	if (index < 0)
	{
		DebugLogWarning("No heap buffer to free.");
		return;
	}

	const VulkanHeapMapping &mapping = this->bufferMappings[index];
	this->allocator.free(mapping.block);
	this->bufferMappings.erase(this->bufferMappings.begin() + index);
}

void VulkanHeap::clear()
{
	this->deviceMemory = nullptr;
	this->allocator.clear();
	this->bufferMappings.clear();
}

bool VulkanRenderBackend::init(const RenderInitSettings &initSettings)
{
	const Window *window = initSettings.window;
	const std::string &dataFolderPath = initSettings.dataFolderPath;

	if (!TryCreateVulkanInstance(window->window, &this->instance))
	{
		DebugLogError("Couldn't create Vulkan instance.");
		return false;
	}

	VkSurfaceKHR vulkanSurface;
	if (SDL_Vulkan_CreateSurface(window->window, this->instance, &vulkanSurface) != SDL_TRUE)
	{
		DebugLogErrorFormat("Couldn't create VkSurfaceKHR.");
		return false;
	}

	this->surface = vk::SurfaceKHR(vulkanSurface);

	vk::ResultValue<std::vector<vk::PhysicalDevice>> physicalDevicesResult = this->instance.enumeratePhysicalDevices();
	if (physicalDevicesResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't get vk::PhysicalDevice list (%d).", physicalDevicesResult.result);
		return false;
	}

	std::vector<vk::PhysicalDevice> physicalDevices = std::move(physicalDevicesResult.value);
	if (physicalDevices.empty())
	{
		DebugLogErrorFormat("No physical devices in vk::PhysicalDevice list.");
		return false;
	}

	this->physicalDevice = GetBestPhysicalDevice(physicalDevices);

	if (!TryGetQueueFamilyIndices(this->physicalDevice, this->surface, &this->graphicsQueueFamilyIndex, &this->presentQueueFamilyIndex))
	{
		DebugLogError("Couldn't get queue family indices from physical device.");
		return false;
	}

	if (!TryCreateDevice(this->physicalDevice, this->graphicsQueueFamilyIndex, this->presentQueueFamilyIndex, &this->device))
	{
		DebugLogError("Couldn't create device.");
		return false;
	}

	this->graphicsQueue = this->device.getQueue(this->graphicsQueueFamilyIndex, 0);
	this->presentQueue = this->device.getQueue(this->presentQueueFamilyIndex, 0);

	vk::SurfaceFormatKHR surfaceFormat;
	if (!TryGetSurfaceFormat(this->physicalDevice, this->surface, vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear, &surfaceFormat))
	{
		DebugLogError("Couldn't get surface format for swapchain.");
		return false;
	}

	vk::PresentModeKHR presentMode;
	if (!TryGetPresentModeOrDefault(this->physicalDevice, this->surface, vk::PresentModeKHR::eImmediate, &presentMode))
	{
		DebugLogError("Couldn't get present mode for swapchain.");
		return false;
	}

	vk::SurfaceCapabilitiesKHR surfaceCapabilities;
	if (!TryGetSurfaceCapabilities(this->physicalDevice, this->surface, &surfaceCapabilities))
	{
		DebugLogError("Couldn't get surface capabilities for swapchain.");
		return false;
	}

	if (!TryGetSurfaceExtentForSwapchain(surfaceCapabilities, window->window, &this->swapchainExtent))
	{
		DebugLogError("Couldn't get surface extent for swapchain.");
		return false;
	}

	if (!TryCreateSwapchain(this->device, this->surface, surfaceFormat, presentMode, surfaceCapabilities, this->swapchainExtent,
		this->graphicsQueueFamilyIndex, this->presentQueueFamilyIndex, &this->swapchain))
	{
		DebugLogError("Couldn't create swapchain.");
		return false;
	}

	if (!TryCreateSwapchainImageViews(this->device, this->swapchain, surfaceFormat, &this->swapchainImageViews))
	{
		DebugLogError("Couldn't create swapchain image views.");
		return false;
	}

	if (!TryCreateSwapchainRenderPass(this->device, surfaceFormat, &this->renderPass))
	{
		DebugLogError("Couldn't create swapchain render pass.");
		return false;
	}

	if (!TryCreateSwapchainFramebuffers(this->device, this->swapchainImageViews, this->swapchainExtent, this->renderPass, &this->swapchainFramebuffers))
	{
		DebugLogError("Couldn't create swapchain framebuffers.");
		return false;
	}

	if (!TryCreateCommandPool(this->device, this->graphicsQueueFamilyIndex, &this->commandPool))
	{
		DebugLogError("Couldn't create command pool.");
		return false;
	}

	if (!TryCreateCommandBuffer(this->device, this->commandPool, &this->commandBuffer))
	{
		DebugLogError("Couldn't create command buffer.");
		return false;
	}

	this->objectTextureAllocator.init(&this->objectTexturePool, this->physicalDevice, this->graphicsQueueFamilyIndex, this->device, this->graphicsQueue, this->commandBuffer);
	this->uiTextureAllocator.init(&this->uiTexturePool, this->physicalDevice, this->graphicsQueueFamilyIndex, this->device, this->graphicsQueue, this->commandBuffer);

	const std::string shadersFolderPath = dataFolderPath + "shaders/";
	const std::string vertexShaderBytesFilename = shadersFolderPath + "testVertex.spv";
	if (!TryCreateShaderModule(this->device, vertexShaderBytesFilename.c_str(), &this->vertexShaderModule))
	{
		DebugLogError("Couldn't create vertex shader module.");
		return false;
	}

	const std::string fragmentShaderBytesFilename = shadersFolderPath + "testFragment.spv";
	if (!TryCreateShaderModule(this->device, fragmentShaderBytesFilename.c_str(), &this->fragmentShaderModule))
	{
		DebugLogError("Couldn't create fragment shader module.");
		return false;
	}

	if (!TryCreateDescriptorSetLayout(this->device, &this->descriptorSetLayout))
	{
		DebugLogError("Couldn't create descriptor set layout.");
		return false;
	}

	if (!TryCreateDescriptorPool(this->device, &this->descriptorPool))
	{
		DebugLogError("Couldn't create descriptor pool.");
		return false;
	}

	if (!TryCreateDescriptorSet(this->device, this->descriptorSetLayout, this->descriptorPool, &this->descriptorSet))
	{
		DebugLogError("Couldn't create camera descriptor set.");
		return false;
	}

	if (!TryCreatePipelineLayout(this->device, this->descriptorSetLayout, &this->pipelineLayout))
	{
		DebugLogError("Couldn't create pipeline layout.");
		return false;
	}

	if (!TryCreateGraphicsPipeline(this->device, this->vertexShaderModule, this->fragmentShaderModule, this->swapchainExtent, this->pipelineLayout,
		this->renderPass, &this->graphicsPipeline))
	{
		DebugLogError("Couldn't create graphics pipeline.");
		return false;
	}

	if (!TryCreateSemaphore(this->device, &this->imageIsAvailableSemaphore))
	{
		DebugLogError("Couldn't create image-is-available semaphore.");
		return false;
	}

	if (!TryCreateSemaphore(this->device, &this->renderIsFinishedSemaphore))
	{
		DebugLogError("Couldn't create render-is-finished semaphore.");
		return false;
	}

	if (!this->vertexBufferDeviceLocalHeap.init(this->device, HEAP_MAX_BYTES_VERTEX_BUFFER, VertexBufferDeviceLocalUsageFlags, false, this->physicalDevice))
	{
		DebugLogError("Couldn't create vertex buffer device-local heap.");
		return false;
	}

	if (!this->vertexBufferStagingHeap.init(this->device, HEAP_MAX_BYTES_VERTEX_BUFFER, VertexBufferStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create vertex buffer staging heap.");
		return false;
	}

	if (!this->indexBufferDeviceLocalHeap.init(this->device, HEAP_MAX_BYTES_INDEX_BUFFER, IndexBufferDeviceLocalUsageFlags, false, this->physicalDevice))
	{
		DebugLogError("Couldn't create index buffer device-local heap.");
		return false;
	}

	if (!this->indexBufferStagingHeap.init(this->device, HEAP_MAX_BYTES_INDEX_BUFFER, IndexBufferStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create index buffer staging heap.");
		return false;
	}

	if (!this->uniformBufferDeviceLocalHeap.init(this->device, HEAP_MAX_BYTES_UNIFORM_BUFFER, UniformBufferDeviceLocalUsageFlags, false, this->physicalDevice))
	{
		DebugLogError("Couldn't create uniform buffer device-local heap.");
		return false;
	}

	if (!this->uniformBufferStagingHeap.init(this->device, HEAP_MAX_BYTES_UNIFORM_BUFFER, UniformBufferStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create uniform buffer staging heap.");
		return false;
	}

	constexpr vk::BufferUsageFlags cameraUsageFlags = vk::BufferUsageFlagBits::eUniformBuffer;
	constexpr int cameraByteCount = VulkanCamera::BYTE_COUNT;
	vk::DeviceMemory cameraDeviceMemory;
	if (!TryAllocateMemory(this->device, cameraByteCount, cameraUsageFlags, true, this->physicalDevice, &cameraDeviceMemory))
	{
		DebugLogError("Couldn't allocate memory for camera uniform buffer.");
		return false;
	}

	vk::Buffer cameraBuffer;
	Span<std::byte> cameraHostMappedBytes;
	if (!TryCreateBufferAndMapMemory(this->device, 0, cameraByteCount, cameraUsageFlags, this->graphicsQueueFamilyIndex, cameraDeviceMemory, &cameraBuffer, &cameraHostMappedBytes))
	{
		DebugLogError("Couldn't create buffer and map memory for camera uniform buffer.");
		return false;
	}

	this->camera.init(cameraBuffer, cameraDeviceMemory, cameraHostMappedBytes);

	return true;
}

void VulkanRenderBackend::shutdown()
{
	if (this->device)
	{
		if (this->camera.buffer)
		{
			this->device.destroyBuffer(this->camera.buffer);
			this->camera.buffer = nullptr;
		}

		if (this->camera.deviceMemory)
		{
			this->device.freeMemory(this->camera.deviceMemory);
			this->camera.deviceMemory = nullptr;
		}

		this->camera.hostMappedBytes = Span<std::byte>();

		if (this->uniformBufferStagingHeap.deviceMemory)
		{
			this->device.freeMemory(this->uniformBufferStagingHeap.deviceMemory);
			this->uniformBufferStagingHeap.deviceMemory = nullptr;
		}

		this->uniformBufferStagingHeap.clear();

		if (this->uniformBufferDeviceLocalHeap.deviceMemory)
		{
			this->device.freeMemory(this->uniformBufferDeviceLocalHeap.deviceMemory);
			this->uniformBufferDeviceLocalHeap.deviceMemory = nullptr;
		}

		this->uniformBufferDeviceLocalHeap.clear();

		if (this->indexBufferStagingHeap.deviceMemory)
		{
			this->device.freeMemory(this->indexBufferStagingHeap.deviceMemory);
			this->indexBufferStagingHeap.deviceMemory = nullptr;
		}

		this->indexBufferStagingHeap.clear();

		if (this->indexBufferDeviceLocalHeap.deviceMemory)
		{
			this->device.freeMemory(this->indexBufferDeviceLocalHeap.deviceMemory);
			this->indexBufferDeviceLocalHeap.deviceMemory = nullptr;
		}

		this->indexBufferDeviceLocalHeap.clear();

		if (this->vertexBufferStagingHeap.deviceMemory)
		{
			this->device.freeMemory(this->vertexBufferStagingHeap.deviceMemory);
			this->vertexBufferStagingHeap.deviceMemory = nullptr;
		}

		this->vertexBufferStagingHeap.clear();

		if (this->vertexBufferDeviceLocalHeap.deviceMemory)
		{
			this->device.freeMemory(this->vertexBufferDeviceLocalHeap.deviceMemory);
			this->vertexBufferDeviceLocalHeap.deviceMemory = nullptr;
		}

		this->vertexBufferDeviceLocalHeap.clear();

		for (VulkanTexture &texture : this->uiTexturePool.values)
		{
			if (texture.stagingDeviceMemory)
			{
				this->device.freeMemory(texture.stagingDeviceMemory);
			}

			if (texture.stagingBuffer)
			{
				this->device.destroyBuffer(texture.stagingBuffer);
			}

			if (texture.sampler)
			{
				this->device.destroySampler(texture.sampler);
			}

			if (texture.imageView)
			{
				this->device.destroyImageView(texture.imageView);
			}

			if (texture.deviceMemory)
			{
				this->device.freeMemory(texture.deviceMemory);
			}

			if (texture.image)
			{
				this->device.destroyImage(texture.image);
			}
		}

		this->uiTexturePool.clear();

		for (VulkanTexture &texture : this->objectTexturePool.values)
		{
			if (texture.stagingDeviceMemory)
			{
				this->device.freeMemory(texture.stagingDeviceMemory);
			}

			if (texture.stagingBuffer)
			{
				this->device.destroyBuffer(texture.stagingBuffer);
			}

			if (texture.sampler)
			{
				this->device.destroySampler(texture.sampler);
			}

			if (texture.imageView)
			{
				this->device.destroyImageView(texture.imageView);
			}

			if (texture.deviceMemory)
			{
				this->device.freeMemory(texture.deviceMemory);
			}

			if (texture.image)
			{
				this->device.destroyImage(texture.image);
			}
		}

		this->objectTexturePool.clear();

		for (VulkanLight &light : this->lightPool.values)
		{
			if (light.stagingBuffer)
			{
				this->device.destroyBuffer(light.stagingBuffer);
			}

			if (light.buffer)
			{
				this->device.destroyBuffer(light.buffer);
			}
		}

		this->lightPool.clear();

		for (VulkanBuffer &buffer : this->uniformBufferPool.values)
		{
			if (buffer.stagingBuffer)
			{
				this->device.destroyBuffer(buffer.stagingBuffer);
			}

			if (buffer.buffer)
			{
				this->device.destroyBuffer(buffer.buffer);
			}
		}

		this->uniformBufferPool.clear();

		for (VulkanBuffer &buffer : this->indexBufferPool.values)
		{
			if (buffer.stagingBuffer)
			{
				this->device.destroyBuffer(buffer.stagingBuffer);
			}

			if (buffer.buffer)
			{
				this->device.destroyBuffer(buffer.buffer);
			}
		}

		this->indexBufferPool.clear();

		for (VulkanBuffer &buffer : this->vertexAttributeBufferPool.values)
		{
			if (buffer.stagingBuffer)
			{
				this->device.destroyBuffer(buffer.stagingBuffer);
			}

			if (buffer.buffer)
			{
				this->device.destroyBuffer(buffer.buffer);
			}
		}

		this->vertexAttributeBufferPool.clear();

		for (VulkanBuffer &buffer : this->vertexPositionBufferPool.values)
		{
			if (buffer.stagingBuffer)
			{
				this->device.destroyBuffer(buffer.stagingBuffer);
			}

			if (buffer.buffer)
			{
				this->device.destroyBuffer(buffer.buffer);
			}
		}

		this->vertexPositionBufferPool.clear();

		if (this->renderIsFinishedSemaphore)
		{
			this->device.destroySemaphore(this->renderIsFinishedSemaphore);
			this->renderIsFinishedSemaphore = nullptr;
		}

		if (this->imageIsAvailableSemaphore)
		{
			this->device.destroySemaphore(this->imageIsAvailableSemaphore);
			this->imageIsAvailableSemaphore = nullptr;
		}

		if (this->descriptorPool)
		{
			this->descriptorSet = nullptr;

			this->device.destroyDescriptorPool(this->descriptorPool);
			this->descriptorPool = nullptr;
		}

		if (this->descriptorSetLayout)
		{
			this->device.destroyDescriptorSetLayout(this->descriptorSetLayout);
			this->descriptorSetLayout = nullptr;
		}

		if (this->graphicsPipeline)
		{
			this->device.destroyPipeline(this->graphicsPipeline);
			this->graphicsPipeline = nullptr;
		}

		if (this->pipelineLayout)
		{
			this->device.destroyPipelineLayout(this->pipelineLayout);
			this->pipelineLayout = nullptr;
		}

		if (this->fragmentShaderModule)
		{
			this->device.destroyShaderModule(this->fragmentShaderModule);
			this->fragmentShaderModule = nullptr;
		}

		if (this->vertexShaderModule)
		{
			this->device.destroyShaderModule(this->vertexShaderModule);
			this->vertexShaderModule = nullptr;
		}

		if (this->commandBuffer)
		{
			this->device.freeCommandBuffers(this->commandPool, this->commandBuffer);
			this->commandBuffer = nullptr;
		}

		if (this->commandPool)
		{
			this->device.destroyCommandPool(this->commandPool);
			this->commandPool = nullptr;
		}

		for (vk::Framebuffer framebuffer : this->swapchainFramebuffers)
		{
			if (framebuffer)
			{
				this->device.destroyFramebuffer(framebuffer);
			}
		}

		this->swapchainFramebuffers.clear();

		if (this->renderPass)
		{
			this->device.destroyRenderPass(this->renderPass);
			this->renderPass = nullptr;
		}

		for (vk::ImageView imageView : this->swapchainImageViews)
		{
			if (imageView)
			{
				this->device.destroyImageView(imageView);
			}
		}

		this->swapchainImageViews.clear();

		if (this->swapchain)
		{
			this->device.destroySwapchainKHR(this->swapchain);
			this->swapchain = nullptr;
		}

		this->swapchainExtent = vk::Extent2D();

		this->presentQueue = nullptr;
		this->graphicsQueue = nullptr;

		this->device.destroy();
		this->device = nullptr;
	}

	if (this->instance)
	{
		this->graphicsQueueFamilyIndex = INVALID_UINT32;
		this->presentQueueFamilyIndex = INVALID_UINT32;

		if (this->physicalDevice)
		{
			this->physicalDevice = nullptr;
		}

		if (this->surface)
		{
			this->instance.destroySurfaceKHR(this->surface);
			this->surface = nullptr;
		}

		this->instance.destroy();
		this->instance = nullptr;
	}
}

void VulkanRenderBackend::resize(int windowWidth, int windowHeight, int internalWidth, int internalHeight)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::handleRenderTargetsReset(int windowWidth, int windowHeight, int internalWidth, int internalHeight)
{
	DebugNotImplemented();
}

Renderer3DProfilerData VulkanRenderBackend::getProfilerData() const
{
	DebugLogWarning("Not implemented: VulkanRenderBackend::getProfilerData");
	return Renderer3DProfilerData(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

Surface VulkanRenderBackend::getScreenshot() const
{
	DebugLogWarning("Not implemented: VulkanRenderBackend::getScreenshot");
	return Surface();
}

int VulkanRenderBackend::getBytesPerFloat() const
{
	return sizeof(float);
}

VertexPositionBufferID VulkanRenderBackend::createVertexPositionBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent)
{
	DebugAssert(vertexCount > 0);
	DebugAssert(componentsPerVertex >= 2);
	DebugAssert(bytesPerComponent == sizeof(float));

	const VertexPositionBufferID id = this->vertexPositionBufferPool.alloc();
	if (id < 0)
	{
		DebugLogErrorFormat("Couldn't allocate ID for vertex position buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		return -1;
	}

	const int elementCount = vertexCount * componentsPerVertex;
	vk::Buffer buffer;
	HeapBlock block;
	if (!TryCreateBufferAndBindWithHeap<float>(this->device, elementCount, VertexBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->vertexBufferDeviceLocalHeap, &buffer, &block, nullptr))
	{
		DebugLogErrorFormat("Couldn't create vertex position buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexPositionBufferPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	HeapBlock stagingBlock;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap<float>(this->device, elementCount, VertexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->vertexBufferStagingHeap, &stagingBuffer, &stagingBlock, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create vertex position staging buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexBufferDeviceLocalHeap.freeMapping(buffer);
		this->device.destroyBuffer(buffer);
		this->vertexPositionBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);
	vertexPositionBuffer.init(buffer, stagingBuffer, stagingHostMappedBytes);
	vertexPositionBuffer.initVertexPosition(vertexCount, componentsPerVertex, bytesPerComponent);

	return id;
}

void VulkanRenderBackend::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	VulkanBuffer *vertexPositionBuffer = this->vertexPositionBufferPool.tryGet(id);
	if (vertexPositionBuffer != nullptr)
	{
		if (vertexPositionBuffer->stagingBuffer)
		{
			this->vertexBufferStagingHeap.freeMapping(vertexPositionBuffer->stagingBuffer);
			this->device.destroyBuffer(vertexPositionBuffer->stagingBuffer);
		}

		if (vertexPositionBuffer->buffer)
		{
			this->vertexBufferDeviceLocalHeap.freeMapping(vertexPositionBuffer->buffer);
			this->device.destroyBuffer(vertexPositionBuffer->buffer);
		}
	}

	this->vertexPositionBufferPool.free(id);
}

LockedBuffer VulkanRenderBackend::lockVertexPositionBuffer(VertexPositionBufferID id)
{
	VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);
	const VulkanBufferVertexPositionInfo &vertexPositionInfo = vertexPositionBuffer.vertexPosition;
	return LockedBuffer(vertexPositionBuffer.stagingHostMappedBytes, vertexPositionInfo.bytesPerComponent);
}

void VulkanRenderBackend::unlockVertexPositionBuffer(VertexPositionBufferID id)
{
	const VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);

	auto commandBufferFunc = [this, &vertexPositionBuffer]()
	{
		const int byteCount = vertexPositionBuffer.stagingHostMappedBytes.getCount();
		CopyToBufferDeviceLocal(vertexPositionBuffer.stagingBuffer, byteCount, vertexPositionBuffer.buffer, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->graphicsQueue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for unlocking vertex position buffer %d.", id);
		return;
	}
}

VertexAttributeBufferID VulkanRenderBackend::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent)
{
	DebugAssert(vertexCount > 0);
	DebugAssert(componentsPerVertex >= 2);
	DebugAssert(bytesPerComponent == sizeof(float));

	const VertexAttributeBufferID id = this->vertexAttributeBufferPool.alloc();
	if (id < 0)
	{
		DebugLogErrorFormat("Couldn't allocate ID for vertex attribute buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		return -1;
	}

	const int elementCount = vertexCount * componentsPerVertex;
	vk::Buffer buffer;
	HeapBlock block;
	if (!TryCreateBufferAndBindWithHeap<float>(this->device, elementCount, VertexBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->vertexBufferDeviceLocalHeap, &buffer, &block, nullptr))
	{
		DebugLogErrorFormat("Couldn't create vertex attribute buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexAttributeBufferPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	HeapBlock stagingBlock;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap<float>(this->device, elementCount, VertexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->vertexBufferStagingHeap, &stagingBuffer, &stagingBlock, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create vertex attribute staging buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexBufferDeviceLocalHeap.freeMapping(buffer);
		this->device.destroyBuffer(buffer);
		this->vertexAttributeBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);
	vertexAttributeBuffer.init(buffer, stagingBuffer, stagingHostMappedBytes);
	vertexAttributeBuffer.initVertexAttribute(vertexCount, componentsPerVertex, bytesPerComponent);

	return id;
}

void VulkanRenderBackend::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	VulkanBuffer *vertexAttributeBuffer = this->vertexAttributeBufferPool.tryGet(id);
	if (vertexAttributeBuffer != nullptr)
	{
		if (vertexAttributeBuffer->stagingBuffer)
		{
			this->vertexBufferStagingHeap.freeMapping(vertexAttributeBuffer->stagingBuffer);
			this->device.destroyBuffer(vertexAttributeBuffer->stagingBuffer);
		}

		if (vertexAttributeBuffer->buffer)
		{
			this->vertexBufferDeviceLocalHeap.freeMapping(vertexAttributeBuffer->buffer);
			this->device.destroyBuffer(vertexAttributeBuffer->buffer);
		}
	}

	this->vertexAttributeBufferPool.free(id);
}

LockedBuffer VulkanRenderBackend::lockVertexAttributeBuffer(VertexAttributeBufferID id)
{
	VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);
	const VulkanBufferVertexAttributeInfo &vertexAttributeInfo = vertexAttributeBuffer.vertexAttribute;
	return LockedBuffer(vertexAttributeBuffer.stagingHostMappedBytes, vertexAttributeInfo.bytesPerComponent);
}

void VulkanRenderBackend::unlockVertexAttributeBuffer(VertexAttributeBufferID id)
{
	const VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);

	auto commandBufferFunc = [this, &vertexAttributeBuffer]()
	{
		const int byteCount = vertexAttributeBuffer.stagingHostMappedBytes.getCount();
		CopyToBufferDeviceLocal(vertexAttributeBuffer.stagingBuffer, byteCount, vertexAttributeBuffer.buffer, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->graphicsQueue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for unlocking vertex attribute buffer %d.", id);
		return;
	}
}

IndexBufferID VulkanRenderBackend::createIndexBuffer(int indexCount, int bytesPerIndex)
{
	DebugAssert(indexCount > 0);
	DebugAssert((indexCount % 3) == 0);
	DebugAssert(bytesPerIndex == sizeof(int32_t));

	const IndexBufferID id = this->indexBufferPool.alloc();
	if (id < 0)
	{
		DebugLogErrorFormat("Couldn't allocate ID for index buffer (indices: %d).", indexCount);
		return -1;
	}

	vk::Buffer buffer;
	HeapBlock block;
	if (!TryCreateBufferAndBindWithHeap<int32_t>(this->device, indexCount, IndexBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->indexBufferDeviceLocalHeap, &buffer, &block, nullptr))
	{
		DebugLogErrorFormat("Couldn't create index buffer (indices: %d).", indexCount);
		this->indexBufferPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	HeapBlock stagingBlock;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap<int32_t>(this->device, indexCount, IndexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->indexBufferStagingHeap, &stagingBuffer, &stagingBlock, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create index staging buffer (indices: %d).", indexCount);
		this->indexBufferDeviceLocalHeap.freeMapping(buffer);
		this->device.destroyBuffer(buffer);
		this->indexBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);
	indexBuffer.init(buffer, stagingBuffer, stagingHostMappedBytes);
	indexBuffer.initIndex(indexCount, bytesPerIndex);

	return id;
}

void VulkanRenderBackend::freeIndexBuffer(IndexBufferID id)
{
	VulkanBuffer *indexBuffer = this->indexBufferPool.tryGet(id);
	if (indexBuffer != nullptr)
	{
		if (indexBuffer->stagingBuffer)
		{
			this->indexBufferStagingHeap.freeMapping(indexBuffer->stagingBuffer);
			this->device.destroyBuffer(indexBuffer->stagingBuffer);
		}

		if (indexBuffer->buffer)
		{
			this->indexBufferDeviceLocalHeap.freeMapping(indexBuffer->buffer);
			this->device.destroyBuffer(indexBuffer->buffer);
		}
	}

	this->indexBufferPool.free(id);
}

LockedBuffer VulkanRenderBackend::lockIndexBuffer(IndexBufferID id)
{
	VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);
	const VulkanBufferIndexInfo &indexInfo = indexBuffer.index;
	return LockedBuffer(indexBuffer.stagingHostMappedBytes, indexInfo.bytesPerIndex);
}

void VulkanRenderBackend::unlockIndexBuffer(IndexBufferID id)
{
	const VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);

	auto commandBufferFunc = [this, &indexBuffer]()
	{
		const int byteCount = indexBuffer.stagingHostMappedBytes.getCount();
		CopyToBufferDeviceLocal(indexBuffer.stagingBuffer, byteCount, indexBuffer.buffer, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->graphicsQueue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for unlocking index buffer %d.", id);
		return;
	}
}

ObjectTextureAllocator *VulkanRenderBackend::getObjectTextureAllocator()
{
	return &this->objectTextureAllocator;
}

UiTextureAllocator *VulkanRenderBackend::getUiTextureAllocator()
{
	return &this->uiTextureAllocator;
}

UniformBufferID VulkanRenderBackend::createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement)
{
	DebugAssert(elementCount > 0);
	DebugAssert(bytesPerElement > 0);
	DebugAssert(alignmentOfElement > 0);

	const UniformBufferID id = this->uniformBufferPool.alloc();
	if (id < 0)
	{
		DebugLogErrorFormat("Couldn't allocate ID for uniform buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, bytesPerElement, alignmentOfElement);
		return -1;
	}

	const int byteCount = elementCount * bytesPerElement;
	vk::Buffer buffer;
	HeapBlock block;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, UniformBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->uniformBufferDeviceLocalHeap, &buffer, &block, nullptr))
	{
		DebugLogErrorFormat("Couldn't create uniform buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, bytesPerElement, alignmentOfElement);
		this->uniformBufferPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	HeapBlock stagingBlock;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, UniformBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->uniformBufferStagingHeap, &stagingBuffer, &stagingBlock, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create uniform staging buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, bytesPerElement, alignmentOfElement);
		this->uniformBufferDeviceLocalHeap.freeMapping(buffer);
		this->device.destroyBuffer(buffer);
		this->uniformBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	uniformBuffer.init(buffer, stagingBuffer, stagingHostMappedBytes);
	uniformBuffer.initUniform(elementCount, bytesPerElement, alignmentOfElement);

	return id;
}

void VulkanRenderBackend::freeUniformBuffer(UniformBufferID id)
{
	VulkanBuffer *uniformBuffer = this->uniformBufferPool.tryGet(id);
	if (uniformBuffer != nullptr)
	{
		if (uniformBuffer->stagingBuffer)
		{
			this->uniformBufferStagingHeap.freeMapping(uniformBuffer->stagingBuffer);
			this->device.destroyBuffer(uniformBuffer->stagingBuffer);
		}

		if (uniformBuffer->buffer)
		{
			this->uniformBufferDeviceLocalHeap.freeMapping(uniformBuffer->buffer);
			this->device.destroyBuffer(uniformBuffer->buffer);
		}
	}

	this->uniformBufferPool.free(id);
}

LockedBuffer VulkanRenderBackend::lockUniformBuffer(UniformBufferID id)
{
	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
	return LockedBuffer(uniformBuffer.stagingHostMappedBytes, uniformInfo.bytesPerElement);
}

LockedBuffer VulkanRenderBackend::lockUniformBufferIndex(UniformBufferID id, int index)
{
	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
	const int bytesPerElement = uniformInfo.bytesPerElement;
	Span<std::byte> stagingHostMappedBytesSlice(uniformBuffer.stagingHostMappedBytes.begin() + (index * bytesPerElement), bytesPerElement);
	return LockedBuffer(stagingHostMappedBytesSlice, bytesPerElement);
}

void VulkanRenderBackend::unlockUniformBuffer(UniformBufferID id)
{
	const VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);

	auto commandBufferFunc = [this, &uniformBuffer]()
	{
		const int byteCount = uniformBuffer.stagingHostMappedBytes.getCount();
		CopyToBufferDeviceLocal(uniformBuffer.stagingBuffer, byteCount, uniformBuffer.buffer, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->graphicsQueue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for unlocking uniform buffer %d.", id);
		return;
	}
}

void VulkanRenderBackend::unlockUniformBufferIndex(UniformBufferID id, int index)
{
	const VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	// @todo need CopyToBufferDeviceLocal() to take an offset into the device local buffer
}

RenderLightID VulkanRenderBackend::createLight()
{
	const RenderLightID id = this->lightPool.alloc();
	if (id < 0)
	{
		DebugLogError("Couldn't allocate render light ID.");
		return -1;
	}

	const int byteCount = sizeof(VulkanLightInfo);
	vk::Buffer buffer;
	HeapBlock block;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, UniformBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->uniformBufferDeviceLocalHeap, &buffer, &block, nullptr))
	{
		DebugLogErrorFormat("Couldn't create buffer for light.");
		this->lightPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	HeapBlock stagingBlock;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, UniformBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->uniformBufferStagingHeap, &stagingBuffer, &stagingBlock, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create staging buffer for light.");
		this->uniformBufferDeviceLocalHeap.freeMapping(buffer);
		this->device.destroyBuffer(buffer);
		this->lightPool.free(id);
		return -1;
	}

	VulkanLight &light = this->lightPool.get(id);
	light.init(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, buffer, stagingBuffer, stagingHostMappedBytes);

	return id;
}

void VulkanRenderBackend::freeLight(RenderLightID id)
{
	VulkanLight *light = this->lightPool.tryGet(id);
	if (light != nullptr)
	{
		if (light->stagingBuffer)
		{
			this->uniformBufferStagingHeap.freeMapping(light->stagingBuffer);
			this->device.destroyBuffer(light->stagingBuffer);
		}

		if (light->buffer)
		{
			this->uniformBufferDeviceLocalHeap.freeMapping(light->buffer);
			this->device.destroyBuffer(light->buffer);
		}
	}

	this->lightPool.free(id);
}

bool VulkanRenderBackend::populateLight(RenderLightID id, const Double3 &point, double startRadius, double endRadius)
{
	// @todo
	//DebugLogWarning("VulkanRenderBackend::populateLight() not implemented");
	return true;
}

void VulkanRenderBackend::submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
	const RenderCamera &camera, const RenderFrameSettings &frameSettings)
{
	constexpr uint64_t acquireTimeout = TIMEOUT_UNLIMITED;
	vk::ResultValue<uint32_t> acquiredSwapchainImageIndexResult = this->device.acquireNextImageKHR(this->swapchain, acquireTimeout, this->imageIsAvailableSemaphore);
	if (acquiredSwapchainImageIndexResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't acquire next swapchain image (%d).", acquiredSwapchainImageIndexResult.result);
		return;
	}

	const uint32_t acquiredSwapchainImageIndex = std::move(acquiredSwapchainImageIndexResult.value);
	const vk::Framebuffer acquiredSwapchainFramebuffer = this->swapchainFramebuffers[acquiredSwapchainImageIndex];

	DebugAssert(this->camera.matrixBytes.getCount() == this->camera.hostMappedBytes.getCount());
	this->camera.model = Matrix4f::identity();
	this->camera.view = RendererUtils::matrix4DoubleToFloat(camera.viewMatrix);
	this->camera.projection = RendererUtils::matrix4DoubleToFloat(camera.projectionMatrix);
	this->camera.projection.y.y = -this->camera.projection.y.y; // Flip Y so world is not upside down.
	std::copy(this->camera.matrixBytes.begin(), this->camera.matrixBytes.end(), this->camera.hostMappedBytes.begin());

	const bool isSceneValid = frameSettings.paletteTextureID >= 0;
	if (isSceneValid)
	{
		// @todo light table + light level calculation
		const VulkanTexture &paletteTexture = this->objectTexturePool.get(frameSettings.paletteTextureID);

		// @todo I think we have to have one descriptor set per texture? so that vkCmdBindDescriptorSets() can pick the texture for the draw call
		// - also i think i have to do texture atlases to reduce the # of descriptor sets so it's not in the hundreds
		const VulkanTexture &texture = this->objectTexturePool.get(250);
		UpdateDescriptorSet(this->device, this->descriptorSet, this->camera.buffer, texture.imageView, texture.sampler, paletteTexture.imageView, paletteTexture.sampler);
	}

	this->commandBuffer.reset();

	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	const vk::Result commandBufferBeginResult = this->commandBuffer.begin(commandBufferBeginInfo);
	if (commandBufferBeginResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't begin command buffer (%d).", commandBufferBeginResult);
		return;
	}

	vk::ClearValue clearColor;
	clearColor.color = vk::ClearColorValue(0.05f, 0.05f, 0.05f, 1.0f);

	vk::RenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.renderPass = this->renderPass;
	renderPassBeginInfo.framebuffer = acquiredSwapchainFramebuffer;
	renderPassBeginInfo.renderArea.extent = this->swapchainExtent;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	this->commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
	this->commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, this->graphicsPipeline);

	vk::ArrayProxy<const uint32_t> dynamicOffsets;
	this->commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, this->pipelineLayout, 0, this->descriptorSet, dynamicOffsets);

	for (int i = 0; i < renderCommandList.entryCount; i++)
	{
		for (const RenderDrawCall &drawCall : renderCommandList.entries[i])
		{
			if (drawCall.pixelShaderType != PixelShaderType::Opaque) // @todo
			{
				continue;
			}

			const VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(drawCall.positionBufferID);
			const VulkanBufferVertexPositionInfo &vertexPositionInfo = vertexPositionBuffer.vertexPosition;

			const VulkanBuffer &vertexTexCoordsBuffer = this->vertexAttributeBufferPool.get(drawCall.texCoordBufferID);
			const VulkanBufferVertexAttributeInfo &vertexTexCoordsInfo = vertexTexCoordsBuffer.vertexAttribute;

			const VulkanBuffer &transformBuffer = this->uniformBufferPool.get(drawCall.transformBufferID); // @todo uniform at index
			const VulkanBufferUniformInfo &transformBufferInfo = transformBuffer.uniform;

			const vk::DeviceSize bufferOffset = 0;
			this->commandBuffer.bindVertexBuffers(0, vertexPositionBuffer.buffer, bufferOffset);
			this->commandBuffer.bindVertexBuffers(1, vertexTexCoordsBuffer.buffer, bufferOffset);

			const VulkanBuffer &indexBuffer = this->indexBufferPool.get(drawCall.indexBufferID);
			const VulkanBufferIndexInfo &indexInfo = indexBuffer.index;
			this->commandBuffer.bindIndexBuffer(indexBuffer.buffer, bufferOffset, vk::IndexType::eUint32);

			constexpr uint32_t meshInstanceCount = 1;
			this->commandBuffer.drawIndexed(indexInfo.indexCount, meshInstanceCount, 0, 0, 0);
		}
	}

	this->commandBuffer.endRenderPass();
	const vk::Result commandBufferEndResult = this->commandBuffer.end();
	if (commandBufferEndResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't end command buffer (%d).", commandBufferEndResult);
		return;
	}

	const vk::PipelineStageFlags waitPipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submitInfo;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &this->imageIsAvailableSemaphore;
	submitInfo.pWaitDstStageMask = &waitPipelineStageFlags;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &this->commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &this->renderIsFinishedSemaphore;

	const vk::Result graphicsQueueSubmitResult = this->graphicsQueue.submit(submitInfo);
	if (graphicsQueueSubmitResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't submit graphics queue (%d).", graphicsQueueSubmitResult);
		return;
	}

	vk::PresentInfoKHR presentInfo;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &this->renderIsFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &this->swapchain;
	presentInfo.pImageIndices = &acquiredSwapchainImageIndex;

	const vk::Result presentQueuePresentResult = this->presentQueue.presentKHR(presentInfo);
	if (presentQueuePresentResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't execute present queue (%d).", presentQueuePresentResult);
		return;
	}

	const vk::Result waitForFrameCompletionResult = this->device.waitIdle();
	if (waitForFrameCompletionResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't wait idle for frame completion (%d).", waitForFrameCompletionResult);
		return;
	}
}
