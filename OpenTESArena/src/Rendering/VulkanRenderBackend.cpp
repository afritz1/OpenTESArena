#include <algorithm>
#include <fstream>
#include <functional>
#include <limits>
#include <string>

#include "SDL_vulkan.h"

#include "RenderCamera.h"
#include "RenderCommand.h"
#include "RenderDrawCall.h"
#include "RenderInitSettings.h"
#include "VulkanRenderBackend.h"
#include "Window.h"
#include "../Assets/TextureBuilder.h"
#include "../Assets/TextureManager.h"
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
	bool TryCreateBuffer(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, bool isHostVisible, uint32_t queueFamilyIndex,
		vk::PhysicalDevice physicalDevice, vk::Buffer *outBuffer, vk::DeviceMemory *outDeviceMemory)
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
			DebugLogErrorFormat("Couldn't create device buffer (%d).", bufferCreateResult.result);
			return false;
		}

		const vk::Buffer buffer = std::move(bufferCreateResult.value);

		const vk::MemoryRequirements bufferMemoryRequirements = device.getBufferMemoryRequirements(buffer);
		const vk::MemoryPropertyFlags bufferMemoryPropertyFlags = isHostVisible ? (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) : vk::MemoryPropertyFlagBits::eDeviceLocal;

		vk::MemoryAllocateInfo bufferMemoryAllocateInfo;
		bufferMemoryAllocateInfo.allocationSize = bufferMemoryRequirements.size;
		bufferMemoryAllocateInfo.memoryTypeIndex = FindPhysicalDeviceMemoryTypeIndex(physicalDevice, bufferMemoryRequirements, bufferMemoryPropertyFlags);
		if (bufferMemoryAllocateInfo.memoryTypeIndex == INVALID_UINT32)
		{
			DebugLogErrorFormat("Couldn't find suitable buffer memory type.");
			device.destroyBuffer(buffer);
			return false;
		}

		vk::ResultValue<vk::DeviceMemory> bufferDeviceMemoryResult = device.allocateMemory(bufferMemoryAllocateInfo);
		if (bufferDeviceMemoryResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't allocate device buffer memory (%d).", bufferDeviceMemoryResult.result);
			device.destroyBuffer(buffer);
			return false;
		}

		const vk::DeviceMemory deviceMemory = std::move(bufferDeviceMemoryResult.value);

		const vk::Result bufferBindMemoryResult = device.bindBufferMemory(buffer, deviceMemory, 0);
		if (bufferBindMemoryResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't bind device buffer memory (%d).", bufferBindMemoryResult);
			device.freeMemory(deviceMemory);
			device.destroyBuffer(buffer);
			return false;
		}

		*outBuffer = buffer;
		*outDeviceMemory = deviceMemory;
		return true;
	}

	template<typename T>
	bool TryCreateBuffer(vk::Device device, int elementCount, vk::BufferUsageFlags usageFlags, bool isHostVisible, uint32_t queueFamilyIndex,
		vk::PhysicalDevice physicalDevice, vk::Buffer *outBuffer, vk::DeviceMemory *outDeviceMemory)
	{
		const int byteCount = elementCount * sizeof(T);
		return TryCreateBuffer(device, byteCount, usageFlags, isHostVisible, queueFamilyIndex, physicalDevice, outBuffer, outDeviceMemory);
	}

	bool TryCreateAndMapStagingBuffer(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex, vk::PhysicalDevice physicalDevice,
		vk::Buffer *outBuffer, vk::DeviceMemory *outDeviceMemory, Span<std::byte> *outHostMappedBytes)
	{
		constexpr bool isHostVisible = true;

		vk::Buffer buffer;
		vk::DeviceMemory deviceMemory;
		if (!TryCreateBuffer(device, byteCount, usageFlags, isHostVisible, queueFamilyIndex, physicalDevice, &buffer, &deviceMemory))
		{
			DebugLogErrorFormat("Couldn't create buffer for staging buffer with %d bytes.", byteCount);
			return false;
		}

		vk::ResultValue<void*> deviceMemoryMapResult = device.mapMemory(deviceMemory, 0, byteCount);
		if (deviceMemoryMapResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't map device memory for staging buffer with %d bytes.", byteCount);
			device.freeMemory(deviceMemory);
			device.destroyBuffer(buffer);
			return false;
		}

		*outBuffer = buffer;
		*outDeviceMemory = deviceMemory;
		*outHostMappedBytes = Span<std::byte>(reinterpret_cast<std::byte*>(deviceMemoryMapResult.value), byteCount);
		return true;
	}

	template<typename T>
	bool TryCreateAndMapStagingBuffer(vk::Device device, int elementCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex,
		vk::PhysicalDevice physicalDevice, vk::Buffer *outBuffer, vk::DeviceMemory *outDeviceMemory, Span<T> *outHostMappedElements)
	{
		const int byteCount = elementCount * sizeof(T);
		Span<std::byte> hostMappedBytes;
		const bool success = TryCreateAndMapStagingBuffer(device, byteCount, usageFlags, queueFamilyIndex, physicalDevice, outBuffer, outDeviceMemory, &hostMappedBytes);
		*outHostMappedElements = Span<T>(reinterpret_cast<T*>(hostMappedBytes.begin()), elementCount);
		return success;
	}

	bool TryCopyToBufferHostVisible(vk::Device device, Span<const std::byte> sourceBytes, vk::DeviceMemory destinationDeviceMemory)
	{
		const vk::DeviceSize sourceByteCount = sourceBytes.getCount();
		vk::ResultValue<void*> destinationMapMemoryResult = device.mapMemory(destinationDeviceMemory, 0, sourceByteCount);
		if (destinationMapMemoryResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't map device buffer memory with %d bytes (%d).", sourceByteCount, destinationMapMemoryResult.result);
			return false;
		}

		void *hostMappedMemory = std::move(destinationMapMemoryResult.value);
		std::copy(sourceBytes.begin(), sourceBytes.end(), reinterpret_cast<std::byte*>(hostMappedMemory));
		device.unmapMemory(destinationDeviceMemory);
		return true;
	}

	template<typename T>
	bool TryCopyToBufferHostVisible(vk::Device device, Span<const T> values, vk::DeviceMemory destinationDeviceMemory)
	{
		const size_t valuesByteCount = values.getCount() * sizeof(T);
		Span<const std::byte> valuesAsBytes(reinterpret_cast<const std::byte*>(values.begin()), valuesByteCount);
		return TryCopyToBufferHostVisible(device, valuesAsBytes, destinationDeviceMemory);
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
		samplerCreateInfo.compareEnable = vk::False;
		samplerCreateInfo.compareOp = vk::CompareOp::eAlways;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		samplerCreateInfo.unnormalizedCoordinates = vk::False;

		vk::ResultValue<vk::Sampler> samplerCreateResult = device.createSampler(samplerCreateInfo);
		if (samplerCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogError("Couldn't create vk::Sampler (%d).", samplerCreateResult.result);
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
		vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding;
		descriptorSetLayoutBinding.binding = 0;
		descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptorSetLayoutBinding.descriptorCount = 1;
		descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.bindingCount = 1;
		descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

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
		vk::DescriptorPoolSize descriptorPoolSize;
		descriptorPoolSize.type = vk::DescriptorType::eUniformBuffer;
		descriptorPoolSize.descriptorCount = 1;

		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

		vk::ResultValue<vk::DescriptorPool> descriptorPoolCreateResult = device.createDescriptorPool(descriptorPoolCreateInfo);
		if (descriptorPoolCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::DescriptorPool (%d).", descriptorPoolCreateResult.result);
			return false;
		}

		*outDescriptorPool = std::move(descriptorPoolCreateResult.value);
		return true;
	}

	bool TryCreateDescriptorSet(vk::Device device, vk::DescriptorSetLayout descriptorSetLayout, vk::DescriptorPool descriptorPool, vk::Buffer buffer, vk::DescriptorSet *outDescriptorSet)
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

		const vk::DescriptorSet descriptorSet = descriptorSets[0];

		vk::DescriptorBufferInfo descriptorBufferInfo;
		descriptorBufferInfo.buffer = buffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet writeDescriptorSet;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

		vk::ArrayProxy<vk::CopyDescriptorSet> copyDescriptorSetArrayProxy;
		device.updateDescriptorSets(writeDescriptorSet, copyDescriptorSetArrayProxy);

		*outDescriptorSet = descriptorSet;
		return true;
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

		vk::VertexInputBindingDescription vertexInputBindingDescription;
		vertexInputBindingDescription.binding = 0;
		vertexInputBindingDescription.stride = static_cast<uint32_t>(MeshUtils::POSITION_COMPONENTS_PER_VERTEX * MeshUtils::POSITION_COMPONENT_SIZE_FLOAT);
		vertexInputBindingDescription.inputRate = vk::VertexInputRate::eVertex;

		vk::VertexInputAttributeDescription vertexInputAttributeDescription;
		vertexInputAttributeDescription.location = 0;
		vertexInputAttributeDescription.binding = 0;
		vertexInputAttributeDescription.format = vk::Format::eR32G32B32Sfloat;
		vertexInputAttributeDescription.offset = 0;

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
		pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
		pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
		pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = &vertexInputAttributeDescription;

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
		pipelineRasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eNone;
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

void VulkanBuffer::initVertexPosition(int vertexCount, int componentsPerVertex, size_t sizeOfComponent, vk::Buffer buffer, vk::DeviceMemory deviceMemory)
{
	DebugAssert(!this->stagingBuffer);
	DebugAssert(!this->stagingDeviceMemory);
	this->buffer = buffer;
	this->deviceMemory = deviceMemory;

	this->type = VulkanBufferType::VertexPosition;
	this->vertexPosition.vertexCount = vertexCount;
	this->vertexPosition.componentsPerVertex = componentsPerVertex;
	this->vertexPosition.sizeOfComponent = sizeOfComponent;
}

void VulkanBuffer::initVertexAttribute(int vertexCount, int componentsPerVertex, size_t sizeOfComponent, vk::Buffer buffer, vk::DeviceMemory deviceMemory)
{
	DebugAssert(!this->stagingBuffer);
	DebugAssert(!this->stagingDeviceMemory);
	this->buffer = buffer;
	this->deviceMemory = deviceMemory;

	this->type = VulkanBufferType::VertexAttribute;
	this->vertexAttribute.vertexCount = vertexCount;
	this->vertexAttribute.componentsPerVertex = componentsPerVertex;
	this->vertexAttribute.sizeOfComponent = sizeOfComponent;
}

void VulkanBuffer::initIndex(int indexCount, size_t sizeOfIndex, vk::Buffer buffer, vk::DeviceMemory deviceMemory)
{
	DebugAssert(!this->stagingBuffer);
	DebugAssert(!this->stagingDeviceMemory);
	this->buffer = buffer;
	this->deviceMemory = deviceMemory;

	this->type = VulkanBufferType::Index;
	this->index.indexCount = indexCount;
	this->index.sizeOfIndex = sizeOfIndex;
}

void VulkanBuffer::initUniform(int elementCount, size_t sizeOfElement, size_t alignmentOfElement, vk::Buffer buffer, vk::DeviceMemory deviceMemory)
{
	DebugAssert(!this->stagingBuffer);
	DebugAssert(!this->stagingDeviceMemory);
	this->buffer = buffer;
	this->deviceMemory = deviceMemory;

	this->type = VulkanBufferType::Uniform;
	this->uniform.elementCount = elementCount;
	this->uniform.sizeOfElement = sizeOfElement;
	this->uniform.alignmentOfElement = alignmentOfElement;
}

void VulkanBuffer::setLocked(vk::Buffer stagingBuffer, vk::DeviceMemory stagingDeviceMemory)
{
	DebugAssert(!this->stagingBuffer);
	DebugAssert(!this->stagingDeviceMemory);
	DebugAssert(stagingBuffer);
	DebugAssert(stagingDeviceMemory);
	this->stagingBuffer = stagingBuffer;
	this->stagingDeviceMemory = stagingDeviceMemory;
}

void VulkanBuffer::setUnlocked()
{
	DebugAssert(this->stagingBuffer);
	DebugAssert(this->stagingDeviceMemory);
	this->stagingBuffer = vk::Buffer(nullptr);
	this->stagingDeviceMemory = vk::DeviceMemory(nullptr);
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

void VulkanLight::init(float pointX, float pointY, float pointZ, float startRadius, float endRadius, vk::Buffer buffer, vk::DeviceMemory deviceMemory)
{
	DebugAssert(!this->stagingBuffer);
	DebugAssert(!this->stagingDeviceMemory);
	this->buffer = buffer;
	this->deviceMemory = deviceMemory;

	DebugAssert(endRadius >= startRadius);
	this->lightInfo.init(pointX, pointY, pointZ, startRadius, endRadius);
}

void VulkanLight::setLocked(vk::Buffer stagingBuffer, vk::DeviceMemory stagingDeviceMemory)
{
	DebugAssert(!this->stagingBuffer);
	DebugAssert(!this->stagingDeviceMemory);
	DebugAssert(stagingBuffer);
	DebugAssert(stagingDeviceMemory);
	this->stagingBuffer = stagingBuffer;
	this->stagingDeviceMemory = stagingDeviceMemory;
}

void VulkanLight::setUnlocked()
{
	DebugAssert(this->stagingBuffer);
	DebugAssert(this->stagingDeviceMemory);
	this->stagingBuffer = vk::Buffer(nullptr);
	this->stagingDeviceMemory = vk::DeviceMemory(nullptr);
}

VulkanTexture::VulkanTexture()
{
	this->width = 0;
	this->height = 0;
	this->bytesPerTexel = 0;
}

void VulkanTexture::init(int width, int height, int bytesPerTexel, vk::Image image, vk::DeviceMemory deviceMemory, vk::ImageView imageView, vk::Sampler sampler)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(bytesPerTexel > 0);
	this->width = width;
	this->height = height;
	this->bytesPerTexel = bytesPerTexel;
	this->image = image;
	this->deviceMemory = deviceMemory;
	this->imageView = imageView;
	this->sampler = sampler;
}

void VulkanTexture::setLocked(vk::Buffer stagingBuffer, vk::DeviceMemory stagingDeviceMemory)
{
	DebugAssert(!this->stagingBuffer);
	DebugAssert(!this->stagingDeviceMemory);
	DebugAssert(stagingBuffer);
	DebugAssert(stagingDeviceMemory);
	this->stagingBuffer = stagingBuffer;
	this->stagingDeviceMemory = stagingDeviceMemory;
}

void VulkanTexture::setUnlocked()
{
	DebugAssert(this->stagingBuffer);
	DebugAssert(this->stagingDeviceMemory);
	this->stagingBuffer = vk::Buffer(nullptr);
	this->stagingDeviceMemory = vk::DeviceMemory(nullptr);
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

	const vk::Format format = (bytesPerTexel == 1) ? vk::Format::eR8Uint : vk::Format::eR8G8B8A8Unorm;
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

	VulkanTexture &texture = this->pool->get(textureID);
	texture.init(width, height, bytesPerTexel, image, deviceMemory, imageView, sampler);

	return textureID;
}

ObjectTextureID VulkanObjectTextureAllocator::create(const TextureBuilder &textureBuilder)
{
	const int width = textureBuilder.width;
	const int height = textureBuilder.height;
	const int bytesPerTexel = textureBuilder.bytesPerTexel;

	const ObjectTextureID textureID = this->create(width, height, bytesPerTexel);
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate object texture for texture builder with dims %dx%d and %d bytes per texel.", width, height, bytesPerTexel);
		return -1;
	}

	LockedTexture lockedTexture = this->lock(textureID);
	if (!lockedTexture.isValid())
	{
		DebugLogErrorFormat("Couldn't lock object texture for texture builder with dims %dx%d and %d bytes per texel.", width, height, bytesPerTexel);
		this->free(textureID);
		return -1;
	}

	std::copy(textureBuilder.texels.begin(), textureBuilder.texels.end(), lockedTexture.texels.begin());
	this->unlock(textureID);

	return textureID;
}

void VulkanObjectTextureAllocator::free(ObjectTextureID textureID)
{
	VulkanTexture *texture = this->pool->tryGet(textureID);
	if (texture != nullptr)
	{
		if (texture->stagingDeviceMemory)
		{
			DebugLogWarningFormat("Freeing object texture %d that was still locked.", textureID);
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

LockedTexture VulkanObjectTextureAllocator::lock(ObjectTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);
	const int width = texture.width;
	const int height = texture.height;
	const int bytesPerTexel = texture.bytesPerTexel;
	const int texelCount = width * height;
	const int byteCount = texelCount * bytesPerTexel;

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateAndMapStagingBuffer(this->device, byteCount, vk::BufferUsageFlagBits::eTransferSrc, this->queueFamilyIndex, this->physicalDevice, &stagingBuffer, &stagingDeviceMemory, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create and map staging buffer for object texture lock with dims %dx%d and %d bytes per texel.", width, height, bytesPerTexel);
		return LockedTexture();
	}

	texture.setLocked(stagingBuffer, stagingDeviceMemory);
	return LockedTexture(Span2D<std::byte>(stagingHostMappedBytes.begin(), texture.width, texture.height), texture.bytesPerTexel);
}

void VulkanObjectTextureAllocator::unlock(ObjectTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);
	const int width = texture.width;
	const int height = texture.height;
	const int bytesPerTexel = texture.bytesPerTexel;

	this->device.unmapMemory(texture.stagingDeviceMemory);

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
		this->device.freeMemory(texture.stagingDeviceMemory);
		this->device.destroyBuffer(texture.stagingBuffer);
		texture.setUnlocked();
		return;
	}

	this->device.freeMemory(texture.stagingDeviceMemory);
	this->device.destroyBuffer(texture.stagingBuffer);
	texture.setUnlocked();
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

	VulkanTexture &texture = this->pool->get(textureID);
	texture.init(width, height, bytesPerTexel, image, deviceMemory, imageView, sampler);

	return textureID;
}

UiTextureID VulkanUiTextureAllocator::create(Span2D<const uint32_t> texels)
{
	const int width = texels.getWidth();
	const int height = texels.getHeight();
	const UiTextureID textureID = this->create(width, height);
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate UI texture with texels and dims %dx%d.", width, height);
		return -1;
	}

	LockedTexture lockedTexture = this->lock(textureID);
	if (!lockedTexture.isValid())
	{
		DebugLogErrorFormat("Couldn't lock UI texture for creation with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	std::copy(texels.begin(), texels.end(), lockedTexture.getTexels32().begin());
	this->unlock(textureID);

	return textureID;
}

UiTextureID VulkanUiTextureAllocator::create(Span2D<const uint8_t> texels, const Palette &palette)
{
	const int width = texels.getWidth();
	const int height = texels.getHeight();
	const UiTextureID textureID = this->create(width, height);
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate UI texture with texels and dims %dx%d.", width, height);
		return -1;
	}

	LockedTexture lockedTexture = this->lock(textureID);
	if (!lockedTexture.isValid())
	{
		DebugLogErrorFormat("Couldn't lock UI texture for creation with dims %dx%d.", width, height);
		this->free(textureID);
		return -1;
	}

	std::transform(texels.begin(), texels.end(), lockedTexture.getTexels32().begin(),
		[&palette](uint8_t texel)
	{
		return palette[texel].toARGB();
	});

	this->unlock(textureID);

	return textureID;
}

UiTextureID VulkanUiTextureAllocator::create(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager)
{
	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);

	UiTextureID textureID = -1;
	if (textureBuilder.bytesPerTexel == 1)
	{
		Span2D<const uint8_t> texels = textureBuilder.getTexels8();
		const Palette &palette = textureManager.getPaletteHandle(paletteID);
		textureID = this->create(texels, palette);
	}
	else if (textureBuilder.bytesPerTexel == 4)
	{
		Span2D<const uint32_t> texels = textureBuilder.getTexels32();
		textureID = this->create(texels);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(textureBuilder.bytesPerTexel));
	}

	return textureID;
}

void VulkanUiTextureAllocator::free(UiTextureID textureID)
{
	VulkanTexture *texture = this->pool->tryGet(textureID);
	if (texture != nullptr)
	{
		if (texture->stagingDeviceMemory)
		{
			DebugLogWarningFormat("Freeing UI texture %d that was still locked.", textureID);
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

LockedTexture VulkanUiTextureAllocator::lock(UiTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);
	const int width = texture.width;
	const int height = texture.height;
	const int bytesPerTexel = texture.bytesPerTexel;
	const int texelCount = width * height;
	DebugAssert(bytesPerTexel == 4);

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;
	Span<uint32_t> stagingHostMappedElements;
	if (!TryCreateAndMapStagingBuffer<uint32_t>(this->device, texelCount, vk::BufferUsageFlagBits::eTransferSrc, this->queueFamilyIndex, this->physicalDevice, &stagingBuffer, &stagingDeviceMemory, &stagingHostMappedElements))
	{
		DebugLogErrorFormat("Couldn't create and map staging buffer for UI texture lock with dims %dx%d.", width, height);
		return LockedTexture();
	}

	texture.setLocked(stagingBuffer, stagingDeviceMemory);
	return LockedTexture(Span2D<std::byte>(reinterpret_cast<std::byte*>(stagingHostMappedElements.begin()), texture.width, texture.height), texture.bytesPerTexel);
}

void VulkanUiTextureAllocator::unlock(UiTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);
	const int width = texture.width;
	const int height = texture.height;
	DebugAssert(texture.bytesPerTexel == 4);

	this->device.unmapMemory(texture.stagingDeviceMemory);

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
		this->device.freeMemory(texture.stagingDeviceMemory);
		this->device.destroyBuffer(texture.stagingBuffer);
		texture.setUnlocked();
		return;
	}

	this->device.freeMemory(texture.stagingDeviceMemory);
	this->device.destroyBuffer(texture.stagingBuffer);
	texture.setUnlocked();
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
	if (!TryGetSurfaceFormat(this->physicalDevice, this->surface, vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear, &surfaceFormat))
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

	vk::Buffer cameraBuffer;
	vk::DeviceMemory cameraDeviceMemory;
	Span<std::byte> cameraHostMappedBytes;
	constexpr int cameraByteCount = VulkanCamera::BYTE_COUNT;
	if (!TryCreateAndMapStagingBuffer(this->device, cameraByteCount, vk::BufferUsageFlagBits::eUniformBuffer, this->graphicsQueueFamilyIndex, this->physicalDevice, &cameraBuffer, &cameraDeviceMemory, &cameraHostMappedBytes))
	{
		DebugLogError("Couldn't create camera uniform buffer.");
		return false;
	}

	this->camera.init(cameraBuffer, cameraDeviceMemory, cameraHostMappedBytes);

	if (!TryCreateDescriptorSet(this->device, this->descriptorSetLayout, this->descriptorPool, cameraBuffer, &this->descriptorSet))
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
			if (light.stagingDeviceMemory)
			{
				this->device.freeMemory(light.stagingDeviceMemory);
			}

			if (light.stagingBuffer)
			{
				this->device.destroyBuffer(light.stagingBuffer);
			}

			if (light.deviceMemory)
			{
				this->device.freeMemory(light.deviceMemory);
			}

			if (light.buffer)
			{
				this->device.destroyBuffer(light.buffer);
			}
		}

		this->lightPool.clear();

		for (VulkanBuffer &buffer : this->uniformBufferPool.values)
		{
			if (buffer.stagingDeviceMemory)
			{
				this->device.freeMemory(buffer.stagingDeviceMemory);
			}

			if (buffer.stagingBuffer)
			{
				this->device.destroyBuffer(buffer.stagingBuffer);
			}

			if (buffer.deviceMemory)
			{
				this->device.freeMemory(buffer.deviceMemory);
			}

			if (buffer.buffer)
			{
				this->device.destroyBuffer(buffer.buffer);
			}
		}

		this->uniformBufferPool.clear();

		for (VulkanBuffer &buffer : this->indexBufferPool.values)
		{
			if (buffer.stagingDeviceMemory)
			{
				this->device.freeMemory(buffer.stagingDeviceMemory);
			}

			if (buffer.stagingBuffer)
			{
				this->device.destroyBuffer(buffer.stagingBuffer);
			}

			if (buffer.deviceMemory)
			{
				this->device.freeMemory(buffer.deviceMemory);
			}

			if (buffer.buffer)
			{
				this->device.destroyBuffer(buffer.buffer);
			}
		}

		this->indexBufferPool.clear();

		for (VulkanBuffer &buffer : this->vertexAttributeBufferPool.values)
		{
			if (buffer.stagingDeviceMemory)
			{
				this->device.freeMemory(buffer.stagingDeviceMemory);
			}

			if (buffer.stagingBuffer)
			{
				this->device.destroyBuffer(buffer.stagingBuffer);
			}

			if (buffer.deviceMemory)
			{
				this->device.freeMemory(buffer.deviceMemory);
			}

			if (buffer.buffer)
			{
				this->device.destroyBuffer(buffer.buffer);
			}
		}

		this->vertexAttributeBufferPool.clear();

		for (VulkanBuffer &buffer : this->vertexPositionBufferPool.values)
		{
			if (buffer.stagingDeviceMemory)
			{
				this->device.freeMemory(buffer.stagingDeviceMemory);
			}

			if (buffer.stagingBuffer)
			{
				this->device.destroyBuffer(buffer.stagingBuffer);
			}

			if (buffer.deviceMemory)
			{
				this->device.freeMemory(buffer.deviceMemory);
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

VertexPositionBufferID VulkanRenderBackend::createVertexPositionBuffer(int vertexCount, int componentsPerVertex)
{
	DebugAssert(vertexCount > 0);
	DebugAssert(componentsPerVertex >= 2);

	const VertexPositionBufferID id = this->vertexPositionBufferPool.alloc();
	if (id < 0)
	{
		DebugLogErrorFormat("Couldn't allocate vertex position buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		return -1;
	}

	const size_t sizeOfComponent = sizeof(float);
	const int elementCount = vertexCount * componentsPerVertex;
	constexpr vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;

	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	if (!TryCreateBuffer<float>(this->device, elementCount, usageFlags, false, this->graphicsQueueFamilyIndex, this->physicalDevice, &buffer, &deviceMemory))
	{
		DebugLogErrorFormat("Couldn't create vertex position buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexPositionBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);
	vertexPositionBuffer.initVertexPosition(vertexCount, componentsPerVertex, sizeOfComponent, buffer, deviceMemory);

	return id;
}

VertexAttributeBufferID VulkanRenderBackend::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex)
{
	DebugAssert(vertexCount > 0);
	DebugAssert(componentsPerVertex >= 2);

	const VertexAttributeBufferID id = this->vertexAttributeBufferPool.alloc();
	if (id < 0)
	{
		DebugLogErrorFormat("Couldn't allocate vertex attribute buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		return -1;
	}

	const size_t sizeOfComponent = sizeof(float);
	const int elementCount = vertexCount * componentsPerVertex;
	constexpr vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;

	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	if (!TryCreateBuffer<float>(this->device, elementCount, usageFlags, false, this->graphicsQueueFamilyIndex, this->physicalDevice, &buffer, &deviceMemory))
	{
		DebugLogErrorFormat("Couldn't create vertex attribute buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexAttributeBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);
	vertexAttributeBuffer.initVertexAttribute(vertexCount, componentsPerVertex, sizeOfComponent, buffer, deviceMemory);

	return id;
}

IndexBufferID VulkanRenderBackend::createIndexBuffer(int indexCount)
{
	DebugAssert(indexCount > 0);
	DebugAssert((indexCount % 3) == 0);

	const IndexBufferID id = this->indexBufferPool.alloc();
	if (id < 0)
	{
		DebugLogErrorFormat("Couldn't allocate index buffer (indices: %d).", indexCount);
		return -1;
	}

	constexpr int sizeOfIndex = sizeof(int32_t);
	constexpr vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;

	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	if (!TryCreateBuffer<int32_t>(this->device, indexCount, usageFlags, false, this->graphicsQueueFamilyIndex, this->physicalDevice, &buffer, &deviceMemory))
	{
		DebugLogErrorFormat("Couldn't create index buffer (indices: %d).", indexCount);
		this->indexBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);
	indexBuffer.initIndex(indexCount, sizeOfIndex, buffer, deviceMemory);

	return id;
}

Span<float> VulkanRenderBackend::lockVertexPositionBuffer(VertexPositionBufferID id)
{
	VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);
	const VulkanBufferVertexPositionInfo &vertexPositionInfo = vertexPositionBuffer.vertexPosition;
	const int elementCount = vertexPositionInfo.vertexCount * vertexPositionInfo.componentsPerVertex;
	const int byteCount = elementCount * vertexPositionInfo.sizeOfComponent;
	DebugAssert(vertexPositionInfo.sizeOfComponent == sizeof(float));

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateAndMapStagingBuffer(this->device, byteCount, vk::BufferUsageFlagBits::eTransferSrc, this->graphicsQueueFamilyIndex, this->physicalDevice, &stagingBuffer, &stagingDeviceMemory, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create and map staging buffer for vertex position buffer (vertices: %d, components: %d).", vertexPositionInfo.vertexCount, vertexPositionInfo.componentsPerVertex);
		return Span<float>();
	}

	vertexPositionBuffer.setLocked(stagingBuffer, stagingDeviceMemory);
	return Span<float>(reinterpret_cast<float*>(stagingHostMappedBytes.begin()), elementCount);
}

void VulkanRenderBackend::unlockVertexPositionBuffer(VertexPositionBufferID id)
{
	VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);
	this->device.unmapMemory(vertexPositionBuffer.stagingDeviceMemory);

	auto commandBufferFunc = [this, &vertexPositionBuffer]()
	{
		const VulkanBufferVertexPositionInfo &vertexPositionInfo = vertexPositionBuffer.vertexPosition;
		const int sourceElementCount = vertexPositionInfo.vertexCount * vertexPositionInfo.componentsPerVertex;
		const int sourceByteCount = sourceElementCount * vertexPositionInfo.sizeOfComponent;
		CopyToBufferDeviceLocal(vertexPositionBuffer.stagingBuffer, sourceByteCount, vertexPositionBuffer.buffer, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->graphicsQueue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for unlocking vertex position buffer %d.", id);
		this->device.freeMemory(vertexPositionBuffer.stagingDeviceMemory);
		this->device.destroyBuffer(vertexPositionBuffer.stagingBuffer);
		vertexPositionBuffer.setUnlocked();
		return;
	}

	this->device.freeMemory(vertexPositionBuffer.stagingDeviceMemory);
	this->device.destroyBuffer(vertexPositionBuffer.stagingBuffer);
	vertexPositionBuffer.setUnlocked();
}

void VulkanRenderBackend::populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions)
{
	Span<float> hostMappedElements = this->lockVertexPositionBuffer(id);
	if (!hostMappedElements.isValid())
	{
		DebugLogErrorFormat("Couldn't populate vertex position buffer %d.", id);
		return;
	}

	std::transform(positions.begin(), positions.end(), hostMappedElements.begin(),
		[](double component)
	{
		return static_cast<float>(component);
	});

	this->unlockVertexPositionBuffer(id);
}

Span<float> VulkanRenderBackend::lockVertexAttributeBuffer(VertexAttributeBufferID id)
{
	VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);
	const VulkanBufferVertexAttributeInfo &vertexAttributeInfo = vertexAttributeBuffer.vertexAttribute;
	const int elementCount = vertexAttributeInfo.vertexCount * vertexAttributeInfo.componentsPerVertex;
	const int byteCount = elementCount * vertexAttributeInfo.sizeOfComponent;
	DebugAssert(vertexAttributeInfo.sizeOfComponent == sizeof(float));

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateAndMapStagingBuffer(this->device, byteCount, vk::BufferUsageFlagBits::eTransferSrc, this->graphicsQueueFamilyIndex, this->physicalDevice, &stagingBuffer, &stagingDeviceMemory, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create and map staging buffer for vertex attribute buffer (vertices: %d, components: %d).", vertexAttributeInfo.vertexCount, vertexAttributeInfo.componentsPerVertex);
		return Span<float>();
	}

	vertexAttributeBuffer.setLocked(stagingBuffer, stagingDeviceMemory);
	return Span<float>(reinterpret_cast<float*>(stagingHostMappedBytes.begin()), elementCount);
}

void VulkanRenderBackend::unlockVertexAttributeBuffer(VertexAttributeBufferID id)
{
	VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);
	this->device.unmapMemory(vertexAttributeBuffer.stagingDeviceMemory);

	auto commandBufferFunc = [this, &vertexAttributeBuffer]()
	{
		const VulkanBufferVertexAttributeInfo &vertexAttributeInfo = vertexAttributeBuffer.vertexAttribute;
		const int sourceElementCount = vertexAttributeInfo.vertexCount * vertexAttributeInfo.componentsPerVertex;
		const int sourceByteCount = sourceElementCount * vertexAttributeInfo.sizeOfComponent;
		CopyToBufferDeviceLocal(vertexAttributeBuffer.stagingBuffer, sourceByteCount, vertexAttributeBuffer.buffer, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->graphicsQueue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for unlocking vertex attribute buffer %d.", id);
		this->device.freeMemory(vertexAttributeBuffer.stagingDeviceMemory);
		this->device.destroyBuffer(vertexAttributeBuffer.stagingBuffer);
		vertexAttributeBuffer.setUnlocked();
		return;
	}

	this->device.freeMemory(vertexAttributeBuffer.stagingDeviceMemory);
	this->device.destroyBuffer(vertexAttributeBuffer.stagingBuffer);
	vertexAttributeBuffer.setUnlocked();
}

void VulkanRenderBackend::populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes)
{
	Span<float> hostMappedElements = this->lockVertexAttributeBuffer(id);
	if (!hostMappedElements.isValid())
	{
		DebugLogErrorFormat("Couldn't populate vertex attribute buffer %d.", id);
		return;
	}

	std::transform(attributes.begin(), attributes.end(), hostMappedElements.begin(),
		[](double component)
	{
		return static_cast<float>(component);
	});

	this->unlockVertexAttributeBuffer(id);
}

Span<int32_t> VulkanRenderBackend::lockIndexBuffer(IndexBufferID id)
{
	VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);
	const VulkanBufferIndexInfo &indexInfo = indexBuffer.index;
	const int elementCount = indexInfo.indexCount;
	const int byteCount = elementCount * indexInfo.sizeOfIndex;
	DebugAssert(indexInfo.sizeOfIndex == sizeof(int32_t));

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;
	Span<int32_t> stagingHostMappedElements;
	if (!TryCreateAndMapStagingBuffer<int32_t>(this->device, elementCount, vk::BufferUsageFlagBits::eTransferSrc, this->graphicsQueueFamilyIndex, this->physicalDevice, &stagingBuffer, &stagingDeviceMemory, &stagingHostMappedElements))
	{
		DebugLogErrorFormat("Couldn't create and map staging buffer for index buffer (indices: %d).", indexInfo.indexCount);
		return Span<int32_t>();
	}

	indexBuffer.setLocked(stagingBuffer, stagingDeviceMemory);
	return stagingHostMappedElements;
}

void VulkanRenderBackend::unlockIndexBuffer(IndexBufferID id)
{
	VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);
	this->device.unmapMemory(indexBuffer.stagingDeviceMemory);

	auto commandBufferFunc = [this, &indexBuffer]()
	{
		const VulkanBufferIndexInfo &indexInfo = indexBuffer.index;
		const int sourceElementCount = indexInfo.indexCount;
		const int sourceByteCount = sourceElementCount * indexInfo.sizeOfIndex;
		CopyToBufferDeviceLocal(indexBuffer.stagingBuffer, sourceByteCount, indexBuffer.buffer, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->graphicsQueue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for unlocking index buffer %d.", id);
		this->device.freeMemory(indexBuffer.stagingDeviceMemory);
		this->device.destroyBuffer(indexBuffer.stagingBuffer);
		indexBuffer.setUnlocked();
		return;
	}

	this->device.freeMemory(indexBuffer.stagingDeviceMemory);
	this->device.destroyBuffer(indexBuffer.stagingBuffer);
	indexBuffer.setUnlocked();
}

void VulkanRenderBackend::populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices)
{
	Span<int32_t> hostMappedElements = this->lockIndexBuffer(id);
	if (!hostMappedElements.isValid())
	{
		DebugLogErrorFormat("Couldn't populate index buffer %d.", id);
		return;
	}

	std::copy(indices.begin(), indices.end(), hostMappedElements.begin());
	this->unlockIndexBuffer(id);
}

void VulkanRenderBackend::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	VulkanBuffer *vertexPositionBuffer = this->vertexPositionBufferPool.tryGet(id);
	if (vertexPositionBuffer != nullptr)
	{
		if (vertexPositionBuffer->stagingDeviceMemory)
		{
			DebugLogWarningFormat("Freeing vertex position buffer %d that was still locked.", id);
			this->device.freeMemory(vertexPositionBuffer->stagingDeviceMemory);
		}

		if (vertexPositionBuffer->stagingBuffer)
		{
			this->device.destroyBuffer(vertexPositionBuffer->stagingBuffer);
		}

		if (vertexPositionBuffer->deviceMemory)
		{
			this->device.freeMemory(vertexPositionBuffer->deviceMemory);
		}

		if (vertexPositionBuffer->buffer)
		{
			this->device.destroyBuffer(vertexPositionBuffer->buffer);
		}
	}

	this->vertexPositionBufferPool.free(id);
}

void VulkanRenderBackend::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	VulkanBuffer *vertexAttributeBuffer = this->vertexAttributeBufferPool.tryGet(id);
	if (vertexAttributeBuffer != nullptr)
	{
		if (vertexAttributeBuffer->stagingDeviceMemory)
		{
			DebugLogWarningFormat("Freeing vertex attribute buffer %d that was still locked.", id);
			this->device.freeMemory(vertexAttributeBuffer->stagingDeviceMemory);
		}

		if (vertexAttributeBuffer->stagingBuffer)
		{
			this->device.destroyBuffer(vertexAttributeBuffer->stagingBuffer);
		}

		if (vertexAttributeBuffer->deviceMemory)
		{
			this->device.freeMemory(vertexAttributeBuffer->deviceMemory);
		}

		if (vertexAttributeBuffer->buffer)
		{
			this->device.destroyBuffer(vertexAttributeBuffer->buffer);
		}
	}

	this->vertexAttributeBufferPool.free(id);
}

void VulkanRenderBackend::freeIndexBuffer(IndexBufferID id)
{
	VulkanBuffer *indexBuffer = this->indexBufferPool.tryGet(id);
	if (indexBuffer != nullptr)
	{
		if (indexBuffer->stagingDeviceMemory)
		{
			DebugLogWarningFormat("Freeing index buffer %d that was still locked.", id);
			this->device.freeMemory(indexBuffer->stagingDeviceMemory);
		}

		if (indexBuffer->stagingBuffer)
		{
			this->device.destroyBuffer(indexBuffer->stagingBuffer);
		}

		if (indexBuffer->deviceMemory)
		{
			this->device.freeMemory(indexBuffer->deviceMemory);
		}

		if (indexBuffer->buffer)
		{
			this->device.destroyBuffer(indexBuffer->buffer);
		}
	}

	this->indexBufferPool.free(id);
}

ObjectTextureAllocator *VulkanRenderBackend::getObjectTextureAllocator()
{
	return &this->objectTextureAllocator;
}

UiTextureAllocator *VulkanRenderBackend::getUiTextureAllocator()
{
	return &this->uiTextureAllocator;
}

UniformBufferID VulkanRenderBackend::createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement)
{
	DebugAssert(elementCount >= 0);
	DebugAssert(sizeOfElement > 0);
	DebugAssert(alignmentOfElement > 0);

	const UniformBufferID id = this->uniformBufferPool.alloc();
	if (id < 0)
	{
		DebugLogErrorFormat("Couldn't allocate uniform buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, sizeOfElement, alignmentOfElement);
		return -1;
	}

	const int byteCount = elementCount * sizeOfElement;
	constexpr vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;

	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	if (!TryCreateBuffer(this->device, byteCount, usageFlags, false, this->graphicsQueueFamilyIndex, this->physicalDevice, &buffer, &deviceMemory))
	{
		DebugLogErrorFormat("Couldn't create uniform buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, sizeOfElement, alignmentOfElement);
		this->uniformBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	uniformBuffer.initUniform(elementCount, sizeOfElement, alignmentOfElement, buffer, deviceMemory);

	return id;
}

Span<std::byte> VulkanRenderBackend::lockUniformBuffer(UniformBufferID id)
{
	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
	const int elementCount = uniformInfo.elementCount;
	const int byteCount = elementCount * uniformInfo.sizeOfElement;

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateAndMapStagingBuffer(this->device, byteCount, vk::BufferUsageFlagBits::eTransferSrc, this->graphicsQueueFamilyIndex, this->physicalDevice, &stagingBuffer, &stagingDeviceMemory, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create and map staging buffer for uniform buffer (elements: %d, sizeOf: %d).", elementCount, uniformInfo.sizeOfElement);
		return Span<std::byte>();
	}

	uniformBuffer.setLocked(stagingBuffer, stagingDeviceMemory);
	return stagingHostMappedBytes;
}

void VulkanRenderBackend::unlockUniformBuffer(UniformBufferID id)
{
	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	this->device.unmapMemory(uniformBuffer.stagingDeviceMemory);

	auto commandBufferFunc = [this, &uniformBuffer]()
	{
		const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
		const int sourceElementCount = uniformInfo.elementCount;
		const int sourceByteCount = sourceElementCount * uniformInfo.sizeOfElement;
		CopyToBufferDeviceLocal(uniformBuffer.stagingBuffer, sourceByteCount, uniformBuffer.buffer, this->commandBuffer);
	};

	if (!TrySubmitCommandBufferOnce(commandBufferFunc, this->commandBuffer, this->graphicsQueue))
	{
		DebugLogErrorFormat("Couldn't submit command buffer one-time command for unlocking uniform buffer %d.", id);
		this->device.freeMemory(uniformBuffer.stagingDeviceMemory);
		this->device.destroyBuffer(uniformBuffer.stagingBuffer);
		uniformBuffer.setUnlocked();
		return;
	}

	this->device.freeMemory(uniformBuffer.stagingDeviceMemory);
	this->device.destroyBuffer(uniformBuffer.stagingBuffer);
	uniformBuffer.setUnlocked();
}

void VulkanRenderBackend::populateUniformBuffer(UniformBufferID id, Span<const std::byte> data)
{
	// @todo RenderTranform isn't compatible with this bytes design if Vulkan wants 32-bit floats. may want a RenderBackend::sizeOfFloat == 4 or 8 so callsites know to convert RenderTransform to a RenderTransformF

	/*Span<std::byte> hostMappedBytes = this->lockUniformBuffer(id);
	if (!hostMappedBytes.isValid())
	{
		DebugLogErrorFormat("Couldn't populate uniform buffer %d.", id);
		return;
	}

	std::copy(data.begin(), data.end(), hostMappedBytes.begin());
	this->unlockUniformBuffer(id);*/
}

void VulkanRenderBackend::populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData)
{
	// @todo ideally this would probably make a staging buffer only for the required incoming size (sizeOfElement) then commit to that one spot in device local memory

	/*VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
	const int elementCount = 1;
	const int byteCount = elementCount * uniformInfo.sizeOfElement;

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingDeviceMemory;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateAndMapStagingBuffer(this->device, byteCount, this->graphicsQueueFamilyIndex, this->physicalDevice, &stagingBuffer, &stagingDeviceMemory, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create and map staging buffer for uniform buffer write at index %d (elements: %d, sizeOf: %d).", uniformIndex, elementCount, uniformInfo.sizeOfElement);
		return;
	}

	uniformBuffer.setLocked(stagingBuffer, stagingDeviceMemory);
	std::copy(uniformData.begin(), uniformData.end(), stagingHostMappedBytes.begin());

	// @todo this doesn't know the uniformIndex to update, need to do that all here (command buffer submit etc)
	this->unlockUniformBuffer(id);*/
}

void VulkanRenderBackend::freeUniformBuffer(UniformBufferID id)
{
	VulkanBuffer *uniformBuffer = this->uniformBufferPool.tryGet(id);
	if (uniformBuffer != nullptr)
	{
		if (uniformBuffer->stagingDeviceMemory)
		{
			DebugLogWarningFormat("Freeing uniform buffer %d that was still locked.", id);
			this->device.freeMemory(uniformBuffer->stagingDeviceMemory);
		}

		if (uniformBuffer->stagingBuffer)
		{
			this->device.destroyBuffer(uniformBuffer->stagingBuffer);
		}

		if (uniformBuffer->deviceMemory)
		{
			this->device.freeMemory(uniformBuffer->deviceMemory);
		}

		if (uniformBuffer->buffer)
		{
			this->device.destroyBuffer(uniformBuffer->buffer);
		}
	}

	this->uniformBufferPool.free(id);
}

RenderLightID VulkanRenderBackend::createLight()
{
	const RenderLightID id = this->lightPool.alloc();
	if (id < 0)
	{
		DebugLogError("Couldn't allocate render light ID.");
		return -1;
	}

	const int byteCount = 1; // @todo
	constexpr vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;

	vk::Buffer buffer;
	vk::DeviceMemory deviceMemory;
	if (!TryCreateBuffer(this->device, byteCount, usageFlags, false, this->graphicsQueueFamilyIndex, this->physicalDevice, &buffer, &deviceMemory))
	{
		DebugLogError("Couldn't create buffer for light.");
		return -1;
	}

	VulkanLightInfo lightInfo; // Default-constructed
	VulkanLight &light = this->lightPool.get(id);
	light.init(lightInfo.pointX, lightInfo.pointY, lightInfo.pointZ, lightInfo.startRadius, lightInfo.endRadius, buffer, deviceMemory);

	return id;
}

void VulkanRenderBackend::setLightPosition(RenderLightID id, const Double3 &worldPoint)
{
	VulkanLight &light = this->lightPool.get(id);
	// @todo vk copy (allocate/bind/map/etc.)
}

void VulkanRenderBackend::setLightRadius(RenderLightID id, double startRadius, double endRadius)
{
	VulkanLight &light = this->lightPool.get(id);
	// @todo vk copy (allocate/bind/map/etc.)
}

void VulkanRenderBackend::freeLight(RenderLightID id)
{
	VulkanLight *light = this->lightPool.tryGet(id);
	if (light != nullptr)
	{
		if (light->stagingDeviceMemory)
		{
			DebugLogWarningFormat("Freeing light %d that was still locked.", id);
			this->device.freeMemory(light->stagingDeviceMemory);
		}

		if (light->stagingBuffer)
		{
			this->device.destroyBuffer(light->stagingBuffer);
		}

		if (light->deviceMemory)
		{
			this->device.freeMemory(light->deviceMemory);
		}

		if (light->buffer)
		{
			this->device.destroyBuffer(light->buffer);
		}
	}

	this->lightPool.free(id);
}

std::optional<Int2> VulkanRenderBackend::tryGetObjectTextureDims(ObjectTextureID id) const
{
	const VulkanTexture *texture = this->objectTexturePool.tryGet(id);
	if (texture == nullptr)
	{
		return std::nullopt;
	}

	return Int2(texture->width, texture->height);
}

std::optional<Int2> VulkanRenderBackend::tryGetUiTextureDims(UiTextureID id) const
{
	const VulkanTexture *texture = this->uiTexturePool.tryGet(id);
	if (texture == nullptr)
	{
		return std::nullopt;
	}

	return Int2(texture->width, texture->height);
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

	auto matrix4DoubleToFloat = [](const Matrix4d &mat)
	{
		Matrix4f floatMatrix;
		floatMatrix.x = Float4(static_cast<float>(mat.x.x), static_cast<float>(mat.x.y), static_cast<float>(mat.x.z), static_cast<float>(mat.x.w));
		floatMatrix.y = Float4(static_cast<float>(mat.y.x), static_cast<float>(mat.y.y), static_cast<float>(mat.y.z), static_cast<float>(mat.y.w));
		floatMatrix.z = Float4(static_cast<float>(mat.z.x), static_cast<float>(mat.z.y), static_cast<float>(mat.z.z), static_cast<float>(mat.z.w));
		floatMatrix.w = Float4(static_cast<float>(mat.w.x), static_cast<float>(mat.w.y), static_cast<float>(mat.w.z), static_cast<float>(mat.w.w));
		return floatMatrix;
	};

	DebugAssert(this->camera.matrixBytes.getCount() == this->camera.hostMappedBytes.getCount());
	this->camera.model = Matrix4f::identity();
	this->camera.view = matrix4DoubleToFloat(camera.viewMatrix);
	this->camera.projection = matrix4DoubleToFloat(camera.projectionMatrix);
	this->camera.projection.y.y = -this->camera.projection.y.y; // Flip Y so world is not upside down.
	std::copy(this->camera.matrixBytes.begin(), this->camera.matrixBytes.end(), this->camera.hostMappedBytes.begin());

	this->commandBuffer.reset();

	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	const vk::Result commandBufferBeginResult = this->commandBuffer.begin(commandBufferBeginInfo);
	if (commandBufferBeginResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't begin command buffer (%d).", commandBufferBeginResult);
		return;
	}

	vk::ClearValue clearColor;
	clearColor.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

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
