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
#include "Renderer.h"
#include "RendererUtils.h"
#include "VulkanRenderBackend.h"
#include "Window.h"
#include "../Math/MathUtils.h"
#include "../UI/UiCommand.h"
#include "../UI/Surface.h"
#include "../Utilities/Platform.h"
#include "../World/MeshUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"
#include "components/utilities/File.h"
#include "components/utilities/StringView.h"

namespace
{
	constexpr uint32_t INVALID_UINT32 = std::numeric_limits<uint32_t>::max();
	constexpr uint64_t TIMEOUT_UNLIMITED = std::numeric_limits<uint64_t>::max();

	constexpr vk::Format MaxCompatibilityImageFormat8888Unorm = vk::Format::eR8G8B8A8Unorm;
	constexpr vk::Format MaxCompatibilityImageFormat32Uint = vk::Format::eR32Uint;
	constexpr vk::Format DefaultSwapchainSurfaceFormat = vk::Format::eB8G8R8A8Unorm; // 0xAARRGGBB in little endian, note that vkFormats are memory layouts, not channel orders.
	constexpr vk::Format InternalFramebufferFormat = MaxCompatibilityImageFormat32Uint;
	constexpr vk::Format ObjectTextureFormat8Bit = vk::Format::eR8Uint;
	constexpr vk::Format ObjectTextureFormat32Bit = vk::Format::eB8G8R8A8Unorm;
	constexpr vk::Format UiTextureFormat = ObjectTextureFormat32Bit;

	constexpr vk::ColorSpaceKHR DefaultSwapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

	constexpr vk::Format DepthBufferFormat = vk::Format::eD32Sfloat;
	constexpr vk::ImageUsageFlagBits DepthBufferUsageFlags = vk::ImageUsageFlagBits::eDepthStencilAttachment;

	// Size of each new individually-created heap in bytes requested from the driver. A single memory allocation (including alignment) cannot exceed this.
	constexpr int BYTES_PER_HEAP_VERTEX_BUFFERS = 1 << 22;
	constexpr int BYTES_PER_HEAP_INDEX_BUFFERS = BYTES_PER_HEAP_VERTEX_BUFFERS;
	constexpr int BYTES_PER_HEAP_UNIFORM_BUFFERS = 1 << 23;
	constexpr int BYTES_PER_HEAP_TEXTURES = 1 << 24;

	constexpr vk::BufferUsageFlags VertexBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags VertexBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
	constexpr vk::BufferUsageFlags IndexBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags IndexBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
	constexpr vk::BufferUsageFlags UniformBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags UniformBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;

	constexpr vk::BufferUsageFlags ObjectTextureStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::ImageUsageFlags ObjectTextureDeviceLocalUsageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	constexpr vk::BufferUsageFlags UiTextureStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::ImageUsageFlags UiTextureDeviceLocalUsageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

	constexpr int MaxGlobalUniformBufferDescriptorSets = 1;
	constexpr int MaxGlobalImageDescriptorSets = 1;
	constexpr int MaxGlobalPoolDescriptorSets = MaxGlobalUniformBufferDescriptorSets + MaxGlobalImageDescriptorSets;

	constexpr int MaxTransformUniformBufferDynamicDescriptorSets = 24576; // @todo this could be reduced by doing one heap per UniformBufferID which supports 4096 entity transforms etc
	constexpr int MaxTransformPoolDescriptorSets = MaxTransformUniformBufferDynamicDescriptorSets;

	constexpr int MaxMaterialImageDescriptorSets = 32768; // Lots of unique materials for entities.
	constexpr int MaxMaterialPoolDescriptorSets = MaxMaterialImageDescriptorSets;

	constexpr std::pair<VertexShaderType, const char*> VertexShaderTypeFilenames[] =
	{
		{ VertexShaderType::Basic, "Basic" },
		{ VertexShaderType::RaisingDoor, "RaisingDoor" },
		{ VertexShaderType::Entity, "Entity" },
		{ VertexShaderType::UI, "UI" }
	};

	constexpr std::pair<PixelShaderType, const char*> FragmentShaderTypeFilenames[] =
	{
		{ PixelShaderType::Opaque, "Opaque" },
		{ PixelShaderType::OpaqueWithAlphaTestLayer, "OpaqueWithAlphaTestLayer" },
		{ PixelShaderType::OpaqueScreenSpaceAnimation, "OpaqueScreenSpaceAnimation" },
		{ PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer, "OpaqueScreenSpaceAnimationWithAlphaTestLayer" },
		{ PixelShaderType::AlphaTested, "AlphaTested" },
		{ PixelShaderType::AlphaTestedWithVariableTexCoordUMin, "AlphaTestedWithVariableTexCoordUMin" },
		{ PixelShaderType::AlphaTestedWithVariableTexCoordVMin, "AlphaTestedWithVariableTexCoordVMin" },
		{ PixelShaderType::AlphaTestedWithPaletteIndexLookup, "AlphaTestedWithPaletteIndexLookup" },
		{ PixelShaderType::AlphaTestedWithLightLevelColor, "AlphaTestedWithLightLevelColor" },
		{ PixelShaderType::AlphaTestedWithLightLevelOpacity, "AlphaTestedWithLightLevelOpacity" },
		{ PixelShaderType::AlphaTestedWithPreviousBrightnessLimit, "AlphaTestedWithPreviousBrightnessLimit" },
		{ PixelShaderType::AlphaTestedWithHorizonMirrorFirstPass, "AlphaTestedWithHorizonMirrorFirstPass" },
		{ PixelShaderType::AlphaTestedWithHorizonMirrorSecondPass, "AlphaTestedWithHorizonMirrorSecondPass" },
		{ PixelShaderType::UiTexture, "UiTexture" }
	};

	VulkanPipelineKeyCode MakePipelineKeyCode(VertexShaderType vertexShaderType, PixelShaderType fragmentShaderType, bool depthRead, bool depthWrite, bool backFaceCulling, bool alphaBlend)
	{
		constexpr int vertexShaderTypeRequiredBits = Bytes::getRequiredBitCount(VERTEX_SHADER_TYPE_COUNT);
		constexpr int fragmentShaderTypeRequiredBits = Bytes::getRequiredBitCount(TOTAL_PIXEL_SHADER_TYPE_COUNT);
		constexpr int depthReadRequiredBits = 1;
		constexpr int depthWriteRequiredBits = 1;
		constexpr int backFaceCullingRequiredBits = 1;
		constexpr int alphaBlendRequiredBits = 1;
		constexpr int totalRequiredBits = vertexShaderTypeRequiredBits + fragmentShaderTypeRequiredBits + depthReadRequiredBits + depthWriteRequiredBits + backFaceCullingRequiredBits + alphaBlendRequiredBits;
		static_assert((sizeof(VulkanPipelineKeyCode) * CHAR_BIT) >= totalRequiredBits);

		constexpr int vertexShaderTypeBitOffset = 0;
		constexpr int fragmentShaderTypeBitOffset = vertexShaderTypeBitOffset + vertexShaderTypeRequiredBits;
		constexpr int depthReadBitOffset = fragmentShaderTypeBitOffset + fragmentShaderTypeRequiredBits;
		constexpr int depthWriteBitOffset = depthReadBitOffset + depthReadRequiredBits;
		constexpr int backFaceCullingBitOffset = depthWriteBitOffset + depthWriteRequiredBits;
		constexpr int alphaBlendBitOffset = backFaceCullingBitOffset + backFaceCullingRequiredBits;

		const uint32_t vertexShaderTypeBits = static_cast<uint32_t>(vertexShaderType);
		const uint32_t fragmentShaderTypeBits = static_cast<uint32_t>(fragmentShaderType);
		const uint32_t depthReadBits = depthRead ? 1 : 0;
		const uint32_t depthWriteBits = depthWrite ? 1 : 0;
		const uint32_t backFaceCullingBits = backFaceCulling ? 1 : 0;
		const uint32_t alphaBlendBits = alphaBlend ? 1 : 0;

		return vertexShaderTypeBits | (fragmentShaderTypeBits << fragmentShaderTypeBitOffset) | (depthReadBits << depthReadBitOffset) |
			(depthWriteBits << depthWriteBitOffset) | (backFaceCullingBits << backFaceCullingBitOffset) | (alphaBlendBits << alphaBlendBitOffset);
	}

	constexpr VulkanPipelineKey RequiredPipelines[] =
	{
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::Opaque, false, false, false, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::Opaque, true, true, false, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::Opaque, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::OpaqueWithAlphaTestLayer, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::OpaqueScreenSpaceAnimation, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTested, false, false, false, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTested, true, true, false, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTested, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTestedWithVariableTexCoordUMin, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTestedWithLightLevelColor, false, false, false, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTestedWithLightLevelOpacity, false, false, false, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTestedWithPreviousBrightnessLimit, false, false, false, false),

		VulkanPipelineKey(VertexShaderType::RaisingDoor, PixelShaderType::AlphaTestedWithVariableTexCoordVMin, true, true, true, false),

		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTested, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTestedWithPaletteIndexLookup, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTestedWithLightLevelOpacity, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTestedWithHorizonMirrorFirstPass, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTestedWithHorizonMirrorSecondPass, true, true, true, false),

		VulkanPipelineKey(VertexShaderType::UI, PixelShaderType::UiTexture, false, false, false, true)
	};

	constexpr int UiPipelineKeyIndex = static_cast<int>(std::size(RequiredPipelines) - 1);
}

// Vulkan application
namespace
{
	constexpr uint32_t RequiredApiVersion = VK_API_VERSION_1_0;

	// MoltenVK check.
	bool IsPlatformPortabilityRequired()
	{
		if (Platform::getPlatform() == Platform::macOS)
		{
			return true;
		}

		return false;
	}

	std::vector<const char*> GetInstanceValidationLayers(bool enableValidationLayers)
	{
		vk::ResultValue<std::vector<vk::LayerProperties>> availableValidationLayersResult = vk::enumerateInstanceLayerProperties();
		if (availableValidationLayersResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't enumerate validation layers (%d).", availableValidationLayersResult.result);
			return std::vector<const char*>();
		}

		const std::vector<vk::LayerProperties> availableValidationLayers = std::move(availableValidationLayersResult.value);

		std::vector<const char*> validationLayers;

		if (enableValidationLayers)
		{
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

			if (!validationLayers.empty())
			{
				DebugLog("Instance validation layers:");
				for (const char *validationLayer : validationLayers)
				{
					DebugLogFormat("- %s", validationLayer);
				}
			}
		}
		else
		{
			DebugLog("Instance validation layers disabled.");
		}

		return validationLayers;
	}

	bool TryCreateVulkanInstance(SDL_Window *window, bool enableValidationLayers, vk::Instance *outInstance)
	{
		uint32_t instanceExtensionCount;
		if (SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, nullptr) != SDL_TRUE)
		{
			DebugLogError("Couldn't get Vulkan instance extension count. Vulkan is not supported.");
			return false;
		}

		std::vector<const char*> instanceExtensions(instanceExtensionCount);
		if (SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, instanceExtensions.data()) != SDL_TRUE)
		{
			DebugLogErrorFormat("Couldn't get Vulkan instance extensions (expected %d).", instanceExtensionCount);
			return false;
		}

		if (IsPlatformPortabilityRequired())
		{
			instanceExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
			instanceExtensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
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

		const std::vector<const char*> instanceValidationLayers = GetInstanceValidationLayers(enableValidationLayers);

		vk::InstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(instanceValidationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = instanceValidationLayers.data();
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		if (IsPlatformPortabilityRequired())
		{
			instanceCreateInfo.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
		}

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

// Vulkan memory
namespace
{
	vk::MemoryAllocateInfo CreateBufferMemoryAllocateInfo(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, bool isHostVisible, vk::PhysicalDevice physicalDevice)
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
			DebugLogErrorFormat("Couldn't create dummy vk::Buffer with %d bytes (%d).", byteCount, dummyBufferCreateResult.result);
			return false;
		}

		vk::Buffer dummyBuffer = std::move(dummyBufferCreateResult.value);
		const vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(dummyBuffer);
		device.destroyBuffer(dummyBuffer);

		const vk::MemoryPropertyFlags memoryPropertyFlags = isHostVisible ? (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) : vk::MemoryPropertyFlagBits::eDeviceLocal;

		vk::MemoryAllocateInfo memoryAllocateInfo;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = FindPhysicalDeviceMemoryTypeIndex(physicalDevice, memoryRequirements, memoryPropertyFlags);
		if (memoryAllocateInfo.memoryTypeIndex == INVALID_UINT32)
		{
			DebugLogErrorFormat("Couldn't find suitable memory type for buffer.");
			return false;
		}

		return memoryAllocateInfo;
	}

	vk::MemoryAllocateInfo CreateImageMemoryAllocateInfo(vk::Device device, int width, int height, vk::Format format, vk::ImageUsageFlags usageFlags, vk::PhysicalDevice physicalDevice)
	{
		// Create dummy image for memory requirements.
		vk::ImageCreateInfo dummyImageCreateInfo;
		dummyImageCreateInfo.imageType = vk::ImageType::e2D;
		dummyImageCreateInfo.format = format;
		dummyImageCreateInfo.extent = vk::Extent3D(width, height, 1);
		dummyImageCreateInfo.mipLevels = 1;
		dummyImageCreateInfo.arrayLayers = 1;
		dummyImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
		dummyImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
		dummyImageCreateInfo.usage = usageFlags;
		dummyImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		dummyImageCreateInfo.queueFamilyIndexCount = 0;
		dummyImageCreateInfo.pQueueFamilyIndices = nullptr;
		dummyImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

		vk::ResultValue<vk::Image> dummyCreateImageResult = device.createImage(dummyImageCreateInfo);
		if (dummyCreateImageResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create dummy vk::Image with dims %dx%d and format %d (%d).", width, height, format, dummyCreateImageResult.result);
			return false;
		}

		vk::Image image = std::move(dummyCreateImageResult.value);
		const vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(image);
		device.destroyImage(image);

		constexpr vk::MemoryPropertyFlags memoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

		vk::MemoryAllocateInfo memoryAllocateInfo;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = FindPhysicalDeviceMemoryTypeIndex(physicalDevice, memoryRequirements, memoryPropertyFlags);
		if (memoryAllocateInfo.memoryTypeIndex == INVALID_UINT32)
		{
			DebugLogErrorFormat("Couldn't find suitable memory type for image.");
			return false;
		}

		return memoryAllocateInfo;
	}

	bool TryAllocateMemory(vk::Device device, vk::MemoryAllocateInfo memoryAllocateInfo, vk::DeviceMemory *outDeviceMemory)
	{
		vk::ResultValue<vk::DeviceMemory> deviceMemoryCreateResult = device.allocateMemory(memoryAllocateInfo);
		if (deviceMemoryCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't allocate device memory with %d bytes and memory type index %d (%d).", memoryAllocateInfo.allocationSize, memoryAllocateInfo.memoryTypeIndex, deviceMemoryCreateResult.result);
			return false;
		}

		*outDeviceMemory = std::move(deviceMemoryCreateResult.value);
		return true;
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
}

// Vulkan buffers
namespace
{
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

	bool TryCreateBufferAndBindWithHeap(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex, VulkanHeapManager &heapManager,
		vk::Buffer *outBuffer, Span<std::byte> *outHostMappedBytes)
	{
		vk::Buffer buffer;
		if (!TryCreateBuffer(device, byteCount, usageFlags, queueFamilyIndex, &buffer))
		{
			DebugLogError("Couldn't create buffer with heap.");
			return false;
		}

		const vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);
		const VulkanHeapMapping heapMapping = heapManager.addBufferMapping(buffer, memoryRequirements.size, memoryRequirements.alignment);
		if (!heapMapping.isValid())
		{
			DebugLogError("Couldn't add heap block mapping for buffer.");
			device.destroyBuffer(buffer);
			return false;
		}

		const HeapBlock block = heapMapping.block;
		VulkanHeap &heap = heapManager.getHeap(heapMapping.heapIndex);
		if (!TryBindBufferToMemory(device, buffer, heap.deviceMemory, block.offset))
		{
			DebugLogError("Couldn't bind buffer to heap memory.");
			heapManager.freeBufferMapping(buffer);
			device.destroyBuffer(buffer);
			return false;
		}

		*outBuffer = buffer;

		if (outHostMappedBytes)
		{
			DebugAssert(heap.hostMappedBytes.isValid());

			// Memory requirements byte count may be greater than requested but only expose what the caller expects.
			*outHostMappedBytes = Span<std::byte>(heap.hostMappedBytes.begin() + block.offset, byteCount);
		}

		return true;
	}

	template<typename T>
	bool TryCreateBufferAndBindWithHeap(vk::Device device, int elementCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex, VulkanHeapManager &heapManager,
		vk::Buffer *outBuffer, Span<std::byte> *outHostMappedBytes)
	{
		const int byteCount = elementCount * sizeof(T);
		return TryCreateBufferAndBindWithHeap(device, byteCount, usageFlags, queueFamilyIndex, heapManager, outBuffer, outHostMappedBytes);
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

	void CopyToBufferDeviceLocal(vk::Buffer sourceBuffer, vk::Buffer destinationBuffer, int byteOffset, int byteCount, vk::CommandBuffer commandBuffer)
	{
		vk::BufferCopy bufferCopy;
		bufferCopy.srcOffset = byteOffset;
		bufferCopy.dstOffset = byteOffset;
		bufferCopy.size = byteCount;

		commandBuffer.copyBuffer(sourceBuffer, destinationBuffer, bufferCopy);
	}

	bool TryCreateImage(vk::Device device, int width, int height, vk::Format format, vk::ImageUsageFlags usageFlags, uint32_t queueFamilyIndex, vk::Image *outImage)
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

		*outImage = std::move(createImageResult.value);
		return true;
	}

	bool TryBindImageToMemory(vk::Device device, vk::Image image, vk::DeviceMemory deviceMemory, int byteOffset)
	{
		const vk::Result imageBindMemoryResult = device.bindImageMemory(image, deviceMemory, byteOffset);
		if (imageBindMemoryResult != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't bind image to device memory at byte offset %d (%d).", byteOffset, imageBindMemoryResult);
			return false;
		}

		return true;
	}

	bool TryCreateImageAndBindWithHeap(vk::Device device, int width, int height, vk::Format format, vk::ImageUsageFlags usageFlags, uint32_t queueFamilyIndex,
		VulkanHeapManager &heapManager, vk::Image *outImage)
	{
		vk::Image image;
		if (!TryCreateImage(device, width, height, format, usageFlags, queueFamilyIndex, &image))
		{
			DebugLogError("Couldn't create image with heap.");
			return false;
		}

		const vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(image);
		const VulkanHeapMapping heapMapping = heapManager.addImageMapping(image, memoryRequirements.size, memoryRequirements.alignment);
		if (!heapMapping.isValid())
		{
			DebugLogError("Couldn't add heap block mapping for image.");
			device.destroyImage(image);
			return false;
		}

		const HeapBlock block = heapMapping.block;
		VulkanHeap &heap = heapManager.getHeap(heapMapping.heapIndex);
		if (!TryBindImageToMemory(device, image, heap.deviceMemory, block.offset))
		{
			DebugLogError("Couldn't bind image to heap memory.");
			heapManager.freeImageMapping(image);
			device.destroyImage(image);
			return false;
		}

		*outImage = image;
		return true;
	}

	bool TryCreateImageView(vk::Device device, vk::Format format, vk::ImageAspectFlags imageAspectFlags, vk::Image image, vk::ImageView *outImageView)
	{
		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.components = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity };
		imageViewCreateInfo.subresourceRange.aspectMask = imageAspectFlags;
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

		std::vector<const char*> deviceExtensions;
		deviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		if (IsPlatformPortabilityRequired())
		{
			deviceExtensions.emplace_back("VK_KHR_portability_subset");
		}

		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.getCount();
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.begin();
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

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
	// If better present modes are unavailable then FIFO is always a valid fallback on all platforms.
	vk::PresentModeKHR GetBestSwapchainPresentMode(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
	{
		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

		vk::ResultValue<std::vector<vk::PresentModeKHR>> presentModesResult = physicalDevice.getSurfacePresentModesKHR(surface);
		if (presentModesResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't query physical device present modes (%d).", presentModesResult.result);
			return presentMode;
		}

		for (const vk::PresentModeKHR currentPresentMode : presentModesResult.value)
		{
			if (currentPresentMode == vk::PresentModeKHR::eFifoRelaxed)
			{
				presentMode = currentPresentMode;
				break;
			}
		}

		return presentMode;
	}

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
			if (!TryCreateImageView(device, surfaceFormat.format, vk::ImageAspectFlagBits::eColor, swapchainImages[i], &(*outImageViews)[i]))
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

		vk::AttachmentDescription depthAttachmentDescription;
		depthAttachmentDescription.format = DepthBufferFormat;
		depthAttachmentDescription.samples = vk::SampleCountFlagBits::e1;
		depthAttachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
		depthAttachmentDescription.storeOp = vk::AttachmentStoreOp::eDontCare;
		depthAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		depthAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		depthAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
		depthAttachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		const vk::AttachmentDescription attachmentDescriptions[] =
		{
			colorAttachmentDescription,
			depthAttachmentDescription
		};

		vk::AttachmentReference colorAttachmentReference;
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentReference depthAttachmentReference;
		depthAttachmentReference.attachment = 1;
		depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		vk::SubpassDescription sceneSubpassDescription;
		sceneSubpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		sceneSubpassDescription.colorAttachmentCount = 1;
		sceneSubpassDescription.pColorAttachments = &colorAttachmentReference;
		sceneSubpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

		vk::SubpassDescription uiSubpassDescription;
		uiSubpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		uiSubpassDescription.colorAttachmentCount = 1;
		uiSubpassDescription.pColorAttachments = &colorAttachmentReference;

		const vk::SubpassDescription subpassDescriptions[] =
		{
			sceneSubpassDescription,
			uiSubpassDescription
		};

		vk::SubpassDependency subpassDependency;
		subpassDependency.srcSubpass = 0;
		subpassDependency.dstSubpass = 1;
		subpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests;
		subpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		subpassDependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		vk::RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(std::size(attachmentDescriptions));
		renderPassCreateInfo.pAttachments = attachmentDescriptions;
		renderPassCreateInfo.subpassCount = static_cast<uint32_t>(std::size(subpassDescriptions));
		renderPassCreateInfo.pSubpasses = subpassDescriptions;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &subpassDependency;

		vk::ResultValue<vk::RenderPass> renderPassCreateResult = device.createRenderPass(renderPassCreateInfo);
		if (renderPassCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create device render pass (%d).", renderPassCreateResult.result);
			return false;
		}

		*outRenderPass = std::move(renderPassCreateResult.value);
		return true;
	}

	bool TryCreateSwapchainFramebuffers(vk::Device device, Span<const vk::ImageView> swapchainImageViews, vk::ImageView depthImageView,
		vk::Extent2D swapchainExtent, vk::RenderPass renderPass, Buffer<vk::Framebuffer> *outFramebuffers)
	{
		outFramebuffers->init(swapchainImageViews.getCount());

		for (int i = 0; i < swapchainImageViews.getCount(); i++)
		{
			const vk::ImageView attachmentImageViews[] =
			{
				swapchainImageViews[i],
				depthImageView
			};

			vk::FramebufferCreateInfo framebufferCreateInfo;
			framebufferCreateInfo.renderPass = renderPass;
			framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(std::size(attachmentImageViews));
			framebufferCreateInfo.pAttachments = attachmentImageViews;
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
		if (shaderBytes.getCount() == 0)
		{
			DebugLogErrorFormat("Expected SPIR-V shader bytes in \"%s\".", filename);
			return false;
		}

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
	vk::DescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(int bindingIndex, vk::DescriptorType descriptorType, vk::ShaderStageFlagBits stageFlags)
	{
		vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding;
		descriptorSetLayoutBinding.binding = bindingIndex;
		descriptorSetLayoutBinding.descriptorType = descriptorType;
		descriptorSetLayoutBinding.descriptorCount = 1;
		descriptorSetLayoutBinding.stageFlags = stageFlags;
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
		return descriptorSetLayoutBinding;
	}

	bool TryCreateDescriptorSetLayout(vk::Device device, Span<const vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings, vk::DescriptorSetLayout *outDescriptorSetLayout)
	{
		vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
		descriptorSetLayoutCreateInfo.bindingCount = descriptorSetLayoutBindings.getCount();
		descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.begin();

		vk::ResultValue<vk::DescriptorSetLayout> descriptorSetLayoutCreateResult = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
		if (descriptorSetLayoutCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::DescriptorSetLayout (%d).", descriptorSetLayoutCreateResult.result);
			return false;
		}

		*outDescriptorSetLayout = std::move(descriptorSetLayoutCreateResult.value);
		return true;
	}

	vk::DescriptorPoolSize CreateDescriptorPoolSize(vk::DescriptorType descriptorType, int descriptorCount)
	{
		vk::DescriptorPoolSize poolSize;
		poolSize.type = descriptorType;
		poolSize.descriptorCount = descriptorCount;
		return poolSize;
	}

	bool TryCreateDescriptorPool(vk::Device device, Span<const vk::DescriptorPoolSize> poolSizes, int maxDescriptorSets, bool isRecycleable, vk::DescriptorPool *outDescriptorPool)
	{
		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
		descriptorPoolCreateInfo.maxSets = maxDescriptorSets;
		descriptorPoolCreateInfo.poolSizeCount = poolSizes.getCount();
		descriptorPoolCreateInfo.pPoolSizes = poolSizes.begin();

		if (isRecycleable)
		{
			descriptorPoolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
		}

		vk::ResultValue<vk::DescriptorPool> descriptorPoolCreateResult = device.createDescriptorPool(descriptorPoolCreateInfo);
		if (descriptorPoolCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create vk::DescriptorPool with %d pool sizes and %d max descriptor sets (%d).", poolSizes.getCount(), maxDescriptorSets, descriptorPoolCreateResult.result);
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

	void UpdateGlobalDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::Buffer cameraBuffer, vk::ImageView paletteImageView, vk::Sampler paletteSampler)
	{
		vk::DescriptorBufferInfo cameraDescriptorBufferInfo;
		cameraDescriptorBufferInfo.buffer = cameraBuffer;
		cameraDescriptorBufferInfo.offset = 0;
		cameraDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorImageInfo paletteDescriptorImageInfo;
		paletteDescriptorImageInfo.sampler = paletteSampler;
		paletteDescriptorImageInfo.imageView = paletteImageView;
		paletteDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::WriteDescriptorSet writeDescriptorSets[2];

		vk::WriteDescriptorSet &cameraWriteDescriptorSet = writeDescriptorSets[0];
		cameraWriteDescriptorSet.dstSet = descriptorSet;
		cameraWriteDescriptorSet.dstBinding = 0;
		cameraWriteDescriptorSet.dstArrayElement = 0;
		cameraWriteDescriptorSet.descriptorCount = 1;
		cameraWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		cameraWriteDescriptorSet.pBufferInfo = &cameraDescriptorBufferInfo;

		vk::WriteDescriptorSet &paletteWriteDescriptorSet = writeDescriptorSets[1];
		paletteWriteDescriptorSet.dstSet = descriptorSet;
		paletteWriteDescriptorSet.dstBinding = 1;
		paletteWriteDescriptorSet.dstArrayElement = 0;
		paletteWriteDescriptorSet.descriptorCount = 1;
		paletteWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		paletteWriteDescriptorSet.pImageInfo = &paletteDescriptorImageInfo;

		vk::ArrayProxy<vk::CopyDescriptorSet> copyDescriptorSets;
		device.updateDescriptorSets(writeDescriptorSets, copyDescriptorSets);
	}

	void UpdateTransformDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::Buffer transformBuffer, int bytesPerStride)
	{
		vk::DescriptorBufferInfo transformDescriptorBufferInfo;
		transformDescriptorBufferInfo.buffer = transformBuffer;
		transformDescriptorBufferInfo.offset = 0;
		transformDescriptorBufferInfo.range = bytesPerStride;

		vk::WriteDescriptorSet transformWriteDescriptorSet;
		transformWriteDescriptorSet.dstSet = descriptorSet;
		transformWriteDescriptorSet.dstBinding = 0;
		transformWriteDescriptorSet.dstArrayElement = 0;
		transformWriteDescriptorSet.descriptorCount = 1;
		transformWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBufferDynamic;
		transformWriteDescriptorSet.pBufferInfo = &transformDescriptorBufferInfo;

		vk::ArrayProxy<vk::CopyDescriptorSet> copyDescriptorSets;
		device.updateDescriptorSets(transformWriteDescriptorSet, copyDescriptorSets);
	}

	void UpdateMaterialDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::ImageView textureImageView, vk::Sampler textureSampler)
	{
		vk::DescriptorImageInfo textureDescriptorImageInfo;
		textureDescriptorImageInfo.sampler = textureSampler;
		textureDescriptorImageInfo.imageView = textureImageView;
		textureDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::WriteDescriptorSet textureWriteDescriptorSet;
		textureWriteDescriptorSet.dstSet = descriptorSet;
		textureWriteDescriptorSet.dstBinding = 0;
		textureWriteDescriptorSet.dstArrayElement = 0;
		textureWriteDescriptorSet.descriptorCount = 1;
		textureWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		textureWriteDescriptorSet.pImageInfo = &textureDescriptorImageInfo;

		vk::ArrayProxy<vk::CopyDescriptorSet> copyDescriptorSets;
		device.updateDescriptorSets(textureWriteDescriptorSet, copyDescriptorSets);
	}
}

// Vulkan pipelines
namespace
{
	std::vector<vk::PushConstantRange> MakePipelineLayoutPushConstantRanges(VertexShaderType vertexShaderType, PixelShaderType fragmentShaderType)
	{
		std::vector<vk::PushConstantRange> pushConstantRanges;
		uint32_t offset = 0;

		auto addPushConstantRange = [&pushConstantRanges, &offset](vk::ShaderStageFlagBits stageFlags, int byteCount)
		{
			vk::PushConstantRange pushConstantRange;
			pushConstantRange.stageFlags = stageFlags;
			pushConstantRange.offset = offset;
			pushConstantRange.size = byteCount;
			pushConstantRanges.emplace_back(std::move(pushConstantRange));

			offset += byteCount;
		};

		const bool requiresPreScaleTransform = vertexShaderType == VertexShaderType::RaisingDoor;
		if (requiresPreScaleTransform)
		{
			addPushConstantRange(vk::ShaderStageFlagBits::eVertex, sizeof(float) * 4);
		}

		const bool requiresUiRectTransform = vertexShaderType == VertexShaderType::UI;
		if (requiresUiRectTransform)
		{
			addPushConstantRange(vk::ShaderStageFlagBits::eVertex, sizeof(float) * 6);
		}

		// @todo if it needs two floats then either need shader to have two push_constant sections with one value or one section with two values
		const bool requiresScreenSpaceAnimPercent =
			(fragmentShaderType == PixelShaderType::OpaqueScreenSpaceAnimation) ||
			(fragmentShaderType == PixelShaderType::OpaqueScreenSpaceAnimationWithAlphaTestLayer);

		const bool requiresPixelShaderParam =
			(fragmentShaderType == PixelShaderType::AlphaTestedWithVariableTexCoordUMin) ||
			(fragmentShaderType == PixelShaderType::AlphaTestedWithVariableTexCoordVMin);
		if (requiresPixelShaderParam)
		{
			addPushConstantRange(vk::ShaderStageFlagBits::eFragment, sizeof(float));
		}

		// @todo mesh lighting percent

		return pushConstantRanges;
	}

	bool TryCreatePipelineLayout(vk::Device device, Span<const vk::DescriptorSetLayout> descriptorSetLayouts, Span<const vk::PushConstantRange> pushConstantRanges, vk::PipelineLayout *outPipelineLayout)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.getCount();
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.begin();

		if (pushConstantRanges.getCount() > 0)
		{
			pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.getCount();
			pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.begin();
		}

		vk::ResultValue<vk::PipelineLayout> pipelineLayoutResult = device.createPipelineLayout(pipelineLayoutCreateInfo);
		if (pipelineLayoutResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create device vk::PipelineLayout with %d descriptor set layouts (%d).", descriptorSetLayouts.getCount(), pipelineLayoutResult.result);
			return false;
		}

		*outPipelineLayout = std::move(pipelineLayoutResult.value);
		return true;
	}

	bool TryCreateGraphicsPipeline(vk::Device device, vk::ShaderModule vertexShaderModule, vk::ShaderModule fragmentShaderModule,
		int positionComponentsPerVertex, bool enableDepthRead, bool enableDepthWrite, bool enableBackFaceCulling, bool enableAlphaBlend,
		vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, int subpassIndex, vk::Pipeline *outPipeline)
	{
		DebugAssert((positionComponentsPerVertex == 3) || (positionComponentsPerVertex == 2));

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
		positionVertexInputBindingDescription.stride = static_cast<uint32_t>(positionComponentsPerVertex * MeshUtils::POSITION_COMPONENT_SIZE_FLOAT);
		positionVertexInputBindingDescription.inputRate = vk::VertexInputRate::eVertex;

		vk::VertexInputAttributeDescription positionVertexInputAttributeDescription;
		positionVertexInputAttributeDescription.location = 0;
		positionVertexInputAttributeDescription.binding = 0;
		positionVertexInputAttributeDescription.format = (positionComponentsPerVertex == 3) ? vk::Format::eR32G32B32Sfloat : vk::Format::eR32G32Sfloat;
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

		vk::Viewport dummyViewport;
		vk::Rect2D dummyViewportScissor;

		vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
		pipelineViewportStateCreateInfo.viewportCount = 1;
		pipelineViewportStateCreateInfo.pViewports = &dummyViewport;
		pipelineViewportStateCreateInfo.scissorCount = 1;
		pipelineViewportStateCreateInfo.pScissors = &dummyViewportScissor;

		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
		pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
		pipelineRasterizationStateCreateInfo.cullMode = enableBackFaceCulling ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone;
		pipelineRasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;
		pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;

		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo;
		pipelineDepthStencilStateCreateInfo.depthTestEnable = enableDepthRead;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = enableDepthWrite;
		pipelineDepthStencilStateCreateInfo.depthCompareOp = enableDepthRead ? vk::CompareOp::eLess : vk::CompareOp::eNever;
		pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = false;
		pipelineDepthStencilStateCreateInfo.stencilTestEnable = false;
		pipelineDepthStencilStateCreateInfo.front = vk::StencilOp::eKeep;
		pipelineDepthStencilStateCreateInfo.back = vk::StencilOp::eKeep;
		pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f;
		pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f;

		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
		pipelineColorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

		if (enableAlphaBlend)
		{
			pipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
			pipelineColorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			pipelineColorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
			pipelineColorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
			pipelineColorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
			pipelineColorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
			pipelineColorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
		}

		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
		pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		pipelineColorBlendStateCreateInfo.attachmentCount = 1;
		pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;

		constexpr vk::DynamicState dynamicStates[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

		vk::PipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
		pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(std::size(dynamicStates));
		pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates;

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
		graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(std::size(pipelineShaderStageCreateInfos));
		graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos;
		graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
		graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
		graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
		graphicsPipelineCreateInfo.layout = pipelineLayout;
		graphicsPipelineCreateInfo.renderPass = renderPass;
		graphicsPipelineCreateInfo.subpass = subpassIndex;
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

void VulkanBuffer::initUniform(int elementCount, int bytesPerElement, int bytesPerStride, vk::DescriptorSet descriptorSet)
{
	this->type = VulkanBufferType::Uniform;
	this->uniform.elementCount = elementCount;
	this->uniform.bytesPerElement = bytesPerElement;
	this->uniform.bytesPerStride = bytesPerStride;
	this->uniform.descriptorSet = descriptorSet;
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

void VulkanTexture::init(int width, int height, int bytesPerTexel, vk::Image image, vk::ImageView imageView, vk::Sampler sampler, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(bytesPerTexel > 0);
	this->width = width;
	this->height = height;
	this->bytesPerTexel = bytesPerTexel;
	this->image = image;
	this->imageView = imageView;
	this->sampler = sampler;
	this->stagingBuffer = stagingBuffer;
	this->stagingHostMappedBytes = stagingHostMappedBytes;
}

VulkanMaterial::VulkanMaterial()
{
	this->meshLightPercent = 0.0f;
	this->pixelShaderParam0 = 0.0f;
	std::fill(std::begin(this->pushConstantTypes), std::end(this->pushConstantTypes), VulkanMaterialPushConstantType::None);
}

void VulkanMaterial::init(vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, vk::DescriptorSet descriptorSet)
{
	this->pipeline = pipeline;
	this->pipelineLayout = pipelineLayout;
	this->descriptorSet = descriptorSet;
}

VulkanCamera::VulkanCamera()
{
	this->matrixBytes = Span<const std::byte>(reinterpret_cast<const std::byte*>(&this->viewProjection), VulkanCamera::BYTE_COUNT);
}

void VulkanCamera::init(vk::Buffer buffer, vk::DeviceMemory deviceMemory, Span<std::byte> hostMappedBytes)
{
	this->buffer = buffer;
	this->deviceMemory = deviceMemory;
	this->hostMappedBytes = hostMappedBytes;
}

VulkanHeapMapping::VulkanHeapMapping()
{
	this->heapIndex = -1;
}

bool VulkanHeapMapping::isValid() const
{
	return (this->heapIndex >= 0) && this->block.isValid();
}

VulkanHeapManager::VulkanHeapManager()
{
	this->type = static_cast<VulkanHeapType>(-1);
	this->isHostVisible = false;
}

bool VulkanHeapManager::initBufferManager(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, bool isHostVisible, vk::PhysicalDevice physicalDevice)
{
	this->type = VulkanHeapType::Buffer;
	this->device = device;
	this->memoryAllocateInfo = CreateBufferMemoryAllocateInfo(device, byteCount, usageFlags, isHostVisible, physicalDevice);
	this->isHostVisible = isHostVisible;

	const int firstHeapIndex = this->addHeap();
	if (firstHeapIndex != 0)
	{
		DebugLogErrorFormat("Couldn't create first buffer heap with %d bytes.", byteCount);
		return false;
	}

	return true;
}

bool VulkanHeapManager::initImageManager(vk::Device device, int byteCount, vk::ImageUsageFlags usageFlags, vk::PhysicalDevice physicalDevice)
{
	this->type = VulkanHeapType::Image;
	this->device = device;
	this->memoryAllocateInfo = CreateImageMemoryAllocateInfo(device, 1, 1, MaxCompatibilityImageFormat8888Unorm, usageFlags, physicalDevice);
	this->memoryAllocateInfo.allocationSize = byteCount;
	this->isHostVisible = false;

	const int firstHeapIndex = this->addHeap();
	if (firstHeapIndex != 0)
	{
		DebugLogErrorFormat("Couldn't create first image heap with %d bytes.", byteCount);
		return false;
	}

	return true;
}

VulkanHeap &VulkanHeapManager::getHeap(int heapIndex)
{
	DebugAssertIndex(this->heaps, heapIndex);
	return this->heaps[heapIndex];
}

int VulkanHeapManager::findAvailableHeapIndex(int byteCount, int alignment) const
{
	const int estimatedBlockSize = MathUtils::roundToGreaterMultipleOf(byteCount, alignment);

	for (int i = 0; i < static_cast<int>(this->heaps.size()); i++)
	{
		const VulkanHeap &heap = this->heaps[i];
		const int largestFreeBlockByteCount = heap.allocator.getLargestFreeBlockBytes();
		if (largestFreeBlockByteCount >= estimatedBlockSize)
		{
			return i;
		}
	}

	return -1;
}

int VulkanHeapManager::addHeap()
{
	const int byteCount = this->memoryAllocateInfo.allocationSize;

	VulkanHeap heap;
	if (!TryAllocateMemory(this->device, this->memoryAllocateInfo, &heap.deviceMemory))
	{
		DebugLogErrorFormat("Couldn't allocate %d bytes for heap (type %d).", byteCount, this->type);
		return -1;
	}

	if (this->isHostVisible)
	{
		if (!TryMapMemory(this->device, heap.deviceMemory, 0, byteCount, &heap.hostMappedBytes))
		{
			DebugLogErrorFormat("Couldn't map %d bytes for heap (type %d).", byteCount, this->type);
			this->device.freeMemory(heap.deviceMemory);
			return -1;
		}
	}

	heap.allocator.init(0, byteCount);

	const int heapIndex = static_cast<int>(this->heaps.size());
	this->heaps.emplace_back(std::move(heap));

	return heapIndex;
}

VulkanHeapMapping VulkanHeapManager::addBufferMapping(vk::Buffer buffer, int byteCount, int alignment)
{
	DebugAssert(this->type == VulkanHeapType::Buffer);

	const int worstCaseByteCount = byteCount + alignment;
	if (worstCaseByteCount > this->memoryAllocateInfo.allocationSize)
	{
		DebugLogErrorFormat("Buffer mapping of %d bytes alignment %d is too large for heap allocation limit of %d bytes.", byteCount, alignment, this->memoryAllocateInfo.allocationSize);
		return VulkanHeapMapping();
	}

	for (const VulkanHeapBufferMapping &mapping : this->bufferMappings)
	{
		if (mapping.buffer == buffer)
		{
			DebugLogError("Heap buffer mapping already exists.");
			return VulkanHeapMapping();
		}
	}

	int heapIndex = this->findAvailableHeapIndex(byteCount, alignment);
	if (heapIndex < 0)
	{
		heapIndex = this->addHeap();
		if (heapIndex < 0)
		{
			DebugLogErrorFormat("Couldn't add heap for buffer mapping of %d bytes alignment %d.", byteCount, alignment);
			return VulkanHeapMapping();
		}
	}

	VulkanHeap &heap = this->heaps[heapIndex];
	const HeapBlock block = heap.allocator.alloc(byteCount, alignment);
	if (!block.isValid())
	{
		DebugLogWarningFormat("Couldn't allocate block for buffer mapping with %d bytes alignment %d.", byteCount, alignment);
		return VulkanHeapMapping();
	}

	VulkanHeapMapping heapMapping;
	heapMapping.block = block;
	heapMapping.heapIndex = heapIndex;

	VulkanHeapBufferMapping bufferMapping;
	bufferMapping.mapping = heapMapping;
	bufferMapping.buffer = buffer;
	this->bufferMappings.emplace_back(std::move(bufferMapping));

	return heapMapping;
}

VulkanHeapMapping VulkanHeapManager::addImageMapping(vk::Image image, int byteCount, int alignment)
{
	DebugAssert(this->type == VulkanHeapType::Image);

	const int worstCaseByteCount = byteCount + alignment;
	if (worstCaseByteCount > this->memoryAllocateInfo.allocationSize)
	{
		DebugLogErrorFormat("Image mapping of %d bytes alignment %d is too large for heap allocation limit of %d bytes.", byteCount, alignment, this->memoryAllocateInfo.allocationSize);
		return VulkanHeapMapping();
	}

	for (const VulkanHeapImageMapping &mapping : this->imageMappings)
	{
		if (mapping.image == image)
		{
			DebugLogError("Heap image mapping already exists.");
			return VulkanHeapMapping();
		}
	}

	int heapIndex = this->findAvailableHeapIndex(byteCount, alignment);
	if (heapIndex < 0)
	{
		heapIndex = this->addHeap();
		if (heapIndex < 0)
		{
			DebugLogErrorFormat("Couldn't add heap for image mapping of %d bytes alignment %d.", byteCount, alignment);
			return VulkanHeapMapping();
		}
	}

	VulkanHeap &heap = this->heaps[heapIndex];
	const HeapBlock block = heap.allocator.alloc(byteCount, alignment);
	if (!block.isValid())
	{
		DebugLogWarningFormat("Couldn't allocate block for image mapping with %d bytes alignment %d.", byteCount, alignment);
		return VulkanHeapMapping();
	}

	VulkanHeapMapping heapMapping;
	heapMapping.block = block;
	heapMapping.heapIndex = heapIndex;

	VulkanHeapImageMapping imageMapping;
	imageMapping.mapping = heapMapping;
	imageMapping.image = image;
	this->imageMappings.emplace_back(std::move(imageMapping));

	return heapMapping;
}

void VulkanHeapManager::freeBufferMapping(vk::Buffer buffer)
{
	DebugAssert(this->type == VulkanHeapType::Buffer);

	int index = -1;
	for (int i = 0; i < static_cast<int>(this->bufferMappings.size()); i++)
	{
		const VulkanHeapBufferMapping &mapping = this->bufferMappings[i];
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

	const VulkanHeapBufferMapping &bufferMapping = this->bufferMappings[index];
	const VulkanHeapMapping heapMapping = bufferMapping.mapping;
	VulkanHeap &heap = this->heaps[heapMapping.heapIndex];
	heap.allocator.free(heapMapping.block);
	this->bufferMappings.erase(this->bufferMappings.begin() + index);
}

void VulkanHeapManager::freeImageMapping(vk::Image image)
{
	DebugAssert(this->type == VulkanHeapType::Image);

	int index = -1;
	for (int i = 0; i < static_cast<int>(this->imageMappings.size()); i++)
	{
		const VulkanHeapImageMapping &mapping = this->imageMappings[i];
		if (mapping.image == image)
		{
			index = i;
			break;
		}
	}

	if (index < 0)
	{
		DebugLogWarning("No heap image to free.");
		return;
	}

	const VulkanHeapImageMapping &imageMapping = this->imageMappings[index];
	const VulkanHeapMapping heapMapping = imageMapping.mapping;
	VulkanHeap &heap = this->heaps[heapMapping.heapIndex];
	heap.allocator.free(heapMapping.block);
	this->imageMappings.erase(this->imageMappings.begin() + index);
}

void VulkanHeapManager::freeAllocations()
{
	for (VulkanHeap &heap : this->heaps)
	{
		if (heap.deviceMemory)
		{
			this->device.freeMemory(heap.deviceMemory);
		}
	}

	this->heaps.clear();
	this->bufferMappings.clear();
	this->imageMappings.clear();
}

void VulkanHeapManager::clear()
{
	this->heaps.clear();
	this->device = vk::Device(nullptr);
	this->memoryAllocateInfo = vk::MemoryAllocateInfo();
	this->isHostVisible = false;
	this->bufferMappings.clear();
	this->imageMappings.clear();
}

VulkanVertexShader::VulkanVertexShader()
{
	this->type = static_cast<VertexShaderType>(-1);
}

VulkanFragmentShader::VulkanFragmentShader()
{
	this->type = static_cast<PixelShaderType>(-1);
}

VulkanPipelineKey::VulkanPipelineKey()
{
	this->vertexShaderType = static_cast<VertexShaderType>(-1);
	this->fragmentShaderType = static_cast<PixelShaderType>(-1);
	this->depthRead = false;
	this->depthWrite = false;
	this->backFaceCulling = false;
	this->alphaBlend = false;
}

bool VulkanRenderBackend::init(const RenderInitSettings &initSettings)
{
	const Window *window = initSettings.window;
	const std::string &dataFolderPath = initSettings.dataFolderPath;

	if (!TryCreateVulkanInstance(window->window, initSettings.enableValidationLayers, &this->instance))
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
	this->physicalDeviceProperties = this->physicalDevice.getProperties();

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
	if (!TryGetSurfaceFormat(this->physicalDevice, this->surface, DefaultSwapchainSurfaceFormat, DefaultSwapchainColorSpace, &surfaceFormat))
	{
		DebugLogError("Couldn't get surface format for swapchain.");
		return false;
	}

	const vk::PresentModeKHR presentMode = GetBestSwapchainPresentMode(this->physicalDevice, this->surface);

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

	const Int2 sceneViewDims = window->getSceneViewDimensions();
	this->sceneViewExtent = vk::Extent2D(sceneViewDims.x, sceneViewDims.y);

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

	const vk::MemoryAllocateInfo depthMemoryAllocateInfo = CreateImageMemoryAllocateInfo(this->device, this->swapchainExtent.width, this->swapchainExtent.height,
		DepthBufferFormat, DepthBufferUsageFlags, this->physicalDevice);
	if (!TryAllocateMemory(this->device, depthMemoryAllocateInfo, &this->depthDeviceMemory))
	{
		DebugLogError("Couldn't allocate depth image memory.");
		return false;
	}

	if (!TryCreateImage(this->device, this->swapchainExtent.width, this->swapchainExtent.height, DepthBufferFormat, DepthBufferUsageFlags, this->graphicsQueueFamilyIndex, &this->depthImage))
	{
		DebugLogError("Couldn't create depth image.");
		return false;
	}

	if (!TryBindImageToMemory(this->device, this->depthImage, this->depthDeviceMemory, 0))
	{
		DebugLogError("Couldn't bind depth image to memory.");
		return false;
	}

	if (!TryCreateImageView(this->device, DepthBufferFormat, vk::ImageAspectFlagBits::eDepth, this->depthImage, &this->depthImageView))
	{
		DebugLogError("Couldn't create depth image view.");
		return false;
	}

	if (!TryCreateSwapchainRenderPass(this->device, surfaceFormat, &this->renderPass))
	{
		DebugLogError("Couldn't create swapchain render pass.");
		return false;
	}

	if (!TryCreateSwapchainFramebuffers(this->device, this->swapchainImageViews, this->depthImageView, this->swapchainExtent, this->renderPass, &this->swapchainFramebuffers))
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

	const std::string shadersFolderPath = dataFolderPath + "shaders/";

	this->vertexShaders.init(static_cast<int>(std::size(VertexShaderTypeFilenames)));
	for (int i = 0; i < this->vertexShaders.getCount(); i++)
	{
		VulkanVertexShader &shader = this->vertexShaders[i];

		const std::pair<VertexShaderType, const char*> &pair = VertexShaderTypeFilenames[i];
		shader.type = pair.first;

		const char *vertexShaderName = pair.second;
		const std::string vertexShaderBytesFilename = shadersFolderPath + vertexShaderName + ".spv";
		if (!TryCreateShaderModule(this->device, vertexShaderBytesFilename.c_str(), &shader.module))
		{
			DebugLogErrorFormat("Couldn't create vertex shader module \"%s\".", vertexShaderBytesFilename.c_str());
			return false;
		}
	}

	this->fragmentShaders.init(static_cast<int>(std::size(FragmentShaderTypeFilenames)));
	for (int i = 0; i < this->fragmentShaders.getCount(); i++)
	{
		VulkanFragmentShader &shader = this->fragmentShaders[i];

		const std::pair<PixelShaderType, const char*> &pair = FragmentShaderTypeFilenames[i];
		shader.type = pair.first;

		const char *fragmentShaderName = pair.second;
		const std::string fragmentShaderBytesFilename = shadersFolderPath + fragmentShaderName + ".spv";
		if (!TryCreateShaderModule(this->device, fragmentShaderBytesFilename.c_str(), &shader.module))
		{
			DebugLogErrorFormat("Couldn't create fragment shader module \"%s\".", fragmentShaderBytesFilename.c_str());
			return false;
		}
	}

	const vk::DescriptorPoolSize globalDescriptorPoolSizes[] =
	{
		CreateDescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MaxGlobalUniformBufferDescriptorSets),
		CreateDescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MaxGlobalImageDescriptorSets)
	};

	const vk::DescriptorPoolSize transformDescriptorPoolSizes[] =
	{
		CreateDescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, MaxTransformUniformBufferDynamicDescriptorSets)
	};

	const vk::DescriptorPoolSize materialDescriptorPoolSizes[] =
	{
		CreateDescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MaxMaterialImageDescriptorSets)
	};

	if (!TryCreateDescriptorPool(this->device, globalDescriptorPoolSizes, MaxGlobalPoolDescriptorSets, false, &this->globalDescriptorPool))
	{
		DebugLogError("Couldn't create general descriptor pool.");
		return false;
	}

	if (!TryCreateDescriptorPool(this->device, transformDescriptorPoolSizes, MaxTransformPoolDescriptorSets, true, &this->transformDescriptorPool))
	{
		DebugLogError("Couldn't create transform descriptor pool.");
		return false;
	}

	if (!TryCreateDescriptorPool(this->device, materialDescriptorPoolSizes, MaxMaterialPoolDescriptorSets, true, &this->materialDescriptorPool))
	{
		DebugLogError("Couldn't create material descriptor pool.");
		return false;
	}

	const vk::DescriptorSetLayoutBinding globalDescriptorSetLayoutBindings[] =
	{
		// Camera
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex),
		// Palette
		CreateDescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
	};

	const vk::DescriptorSetLayoutBinding transformDescriptorSetLayoutBindings[] =
	{
		// Mesh transform
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBufferDynamic, vk::ShaderStageFlagBits::eVertex)
	};

	const vk::DescriptorSetLayoutBinding materialDescriptorSetLayoutBindings[] =
	{
		// Mesh texture
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
	};

	const vk::DescriptorSetLayoutBinding uiMaterialDescriptorSetLayoutBindings[] =
	{
		// UI texture
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
	};

	if (!TryCreateDescriptorSetLayout(this->device, globalDescriptorSetLayoutBindings, &this->globalDescriptorSetLayout))
	{
		DebugLogError("Couldn't create global descriptor set layout.");
		return false;
	}

	if (!TryCreateDescriptorSetLayout(this->device, transformDescriptorSetLayoutBindings, &this->transformDescriptorSetLayout))
	{
		DebugLogError("Couldn't create transform descriptor set layout.");
		return false;
	}

	if (!TryCreateDescriptorSetLayout(this->device, materialDescriptorSetLayoutBindings, &this->materialDescriptorSetLayout))
	{
		DebugLogError("Couldn't create material descriptor set layout.");
		return false;
	}

	if (!TryCreateDescriptorSetLayout(this->device, uiMaterialDescriptorSetLayoutBindings, &this->uiMaterialDescriptorSetLayout))
	{
		DebugLogError("Couldn't create UI material descriptor set layout.");
		return false;
	}

	if (!TryCreateDescriptorSet(this->device, this->globalDescriptorSetLayout, this->globalDescriptorPool, &this->globalDescriptorSet))
	{
		DebugLogError("Couldn't create global descriptor set.");
		return false;
	}

	const vk::DescriptorSetLayout sceneDescriptorSetLayouts[] =
	{
		this->globalDescriptorSetLayout,
		this->transformDescriptorSetLayout,
		this->materialDescriptorSetLayout
	};

	const vk::DescriptorSetLayout uiDescriptorSetLayouts[] =
	{
		this->uiMaterialDescriptorSetLayout
	};

	this->pipelineLayouts.init(static_cast<int>(std::size(RequiredPipelines)));
	this->graphicsPipelines.init(static_cast<int>(std::size(RequiredPipelines)));
	for (int i = 0; i < this->graphicsPipelines.getCount(); i++)
	{
		const VulkanPipelineKey &requiredPipelineKey = RequiredPipelines[i];
		const VertexShaderType vertexShaderType = requiredPipelineKey.vertexShaderType;
		const PixelShaderType fragmentShaderType = requiredPipelineKey.fragmentShaderType;

		int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
		int subpassIndex = 0;
		Span<const vk::DescriptorSetLayout> descriptorSetLayouts = sceneDescriptorSetLayouts;
		if (fragmentShaderType == PixelShaderType::UiTexture)
		{
			positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX_2D;
			subpassIndex = 1;
			descriptorSetLayouts = uiDescriptorSetLayouts;
		}

		const std::vector<vk::PushConstantRange> pushConstantRanges = MakePipelineLayoutPushConstantRanges(vertexShaderType, fragmentShaderType);
		vk::PipelineLayout &pipelineLayout = this->pipelineLayouts[i];
		if (!TryCreatePipelineLayout(this->device, descriptorSetLayouts, pushConstantRanges, &pipelineLayout))
		{
			DebugLogErrorFormat("Couldn't create pipeline layout for graphics pipeline %d.", i);
			return false;
		}

		const auto vertexShaderIter = std::find_if(this->vertexShaders.begin(), this->vertexShaders.end(),
			[vertexShaderType](const VulkanVertexShader &shader)
		{
			return shader.type == vertexShaderType;
		});

		DebugAssert(vertexShaderIter != this->vertexShaders.end());

		const auto fragmentShaderIter = std::find_if(this->fragmentShaders.begin(), this->fragmentShaders.end(),
			[fragmentShaderType](const VulkanFragmentShader &shader)
		{
			return shader.type == fragmentShaderType;
		});

		DebugAssert(fragmentShaderIter != this->fragmentShaders.end());

		VulkanPipeline &pipeline = this->graphicsPipelines[i];
		pipeline.keyCode = MakePipelineKeyCode(vertexShaderType, fragmentShaderType, requiredPipelineKey.depthRead, requiredPipelineKey.depthWrite, requiredPipelineKey.backFaceCulling, requiredPipelineKey.alphaBlend);

		if (!TryCreateGraphicsPipeline(this->device, vertexShaderIter->module, fragmentShaderIter->module, positionComponentsPerVertex, requiredPipelineKey.depthRead,
			requiredPipelineKey.depthWrite, requiredPipelineKey.backFaceCulling, requiredPipelineKey.alphaBlend, pipelineLayout, this->renderPass, subpassIndex, &pipeline.pipeline))
		{
			DebugLogErrorFormat("Couldn't create graphics pipeline %d.", i);
			return false;
		}
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

	if (!this->vertexBufferHeapManagerDeviceLocal.initBufferManager(this->device, BYTES_PER_HEAP_VERTEX_BUFFERS, VertexBufferDeviceLocalUsageFlags, false, this->physicalDevice))
	{
		DebugLogError("Couldn't create vertex buffer device-local heap.");
		return false;
	}

	if (!this->vertexBufferHeapManagerStaging.initBufferManager(this->device, BYTES_PER_HEAP_VERTEX_BUFFERS, VertexBufferStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create vertex buffer staging heap.");
		return false;
	}

	if (!this->indexBufferHeapManagerDeviceLocal.initBufferManager(this->device, BYTES_PER_HEAP_INDEX_BUFFERS, IndexBufferDeviceLocalUsageFlags, false, this->physicalDevice))
	{
		DebugLogError("Couldn't create index buffer device-local heap.");
		return false;
	}

	if (!this->indexBufferHeapManagerStaging.initBufferManager(this->device, BYTES_PER_HEAP_INDEX_BUFFERS, IndexBufferStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create index buffer staging heap.");
		return false;
	}

	if (!this->uniformBufferHeapManagerDeviceLocal.initBufferManager(this->device, BYTES_PER_HEAP_UNIFORM_BUFFERS, UniformBufferDeviceLocalUsageFlags, false, this->physicalDevice))
	{
		DebugLogError("Couldn't create uniform buffer device-local heap.");
		return false;
	}

	if (!this->uniformBufferHeapManagerStaging.initBufferManager(this->device, BYTES_PER_HEAP_UNIFORM_BUFFERS, UniformBufferStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create uniform buffer staging heap.");
		return false;
	}

	if (!this->objectTextureHeapManagerDeviceLocal.initImageManager(this->device, BYTES_PER_HEAP_TEXTURES, ObjectTextureDeviceLocalUsageFlags, this->physicalDevice))
	{
		DebugLogError("Couldn't create object texture device-local heap.");
		return false;
	}

	if (!this->objectTextureHeapManagerStaging.initBufferManager(this->device, BYTES_PER_HEAP_TEXTURES, ObjectTextureStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create object texture staging heap.");
		return false;
	}

	if (!this->uiTextureHeapManagerDeviceLocal.initImageManager(this->device, BYTES_PER_HEAP_TEXTURES, UiTextureDeviceLocalUsageFlags, this->physicalDevice))
	{
		DebugLogError("Couldn't create UI texture device-local heap.");
		return false;
	}

	if (!this->uiTextureHeapManagerStaging.initBufferManager(this->device, BYTES_PER_HEAP_TEXTURES, UiTextureStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create UI texture staging heap.");
		return false;
	}

	constexpr vk::BufferUsageFlags cameraUsageFlags = vk::BufferUsageFlagBits::eUniformBuffer;
	constexpr int cameraByteCount = VulkanCamera::BYTE_COUNT;
	const vk::MemoryAllocateInfo cameraMemoryAllocateInfo = CreateBufferMemoryAllocateInfo(this->device, cameraByteCount, cameraUsageFlags, true, this->physicalDevice);

	vk::DeviceMemory cameraDeviceMemory;
	if (!TryAllocateMemory(this->device, cameraMemoryAllocateInfo, &cameraDeviceMemory))
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

	constexpr int UiRectangleVertexCount = 6; // Two triangles, no indices.
	constexpr int UiPositionComponentsPerVertex = 2;
	this->uiVertexPositionBufferID = this->createVertexPositionBuffer(UiRectangleVertexCount, UiPositionComponentsPerVertex, sizeof(float));
	LockedBuffer lockedUiVertexPositionBuffer = this->lockVertexPositionBuffer(this->uiVertexPositionBufferID);
	Span<float> uiVertexPositionComponents = lockedUiVertexPositionBuffer.getFloats();
	uiVertexPositionComponents[0] = 0.0f;
	uiVertexPositionComponents[1] = 0.0f;

	uiVertexPositionComponents[2] = 0.0f;
	uiVertexPositionComponents[3] = 1.0f;

	uiVertexPositionComponents[4] = 1.0f;
	uiVertexPositionComponents[5] = 1.0f;

	uiVertexPositionComponents[6] = 1.0f;
	uiVertexPositionComponents[7] = 1.0f;

	uiVertexPositionComponents[8] = 1.0f;
	uiVertexPositionComponents[9] = 0.0f;

	uiVertexPositionComponents[10] = 0.0f;
	uiVertexPositionComponents[11] = 0.0f;

	this->unlockVertexPositionBuffer(this->uiVertexPositionBufferID);

	constexpr int UiTexCoordComponentsPerVertex = 2;
	this->uiVertexAttributeBufferID = this->createVertexAttributeBuffer(UiRectangleVertexCount, UiTexCoordComponentsPerVertex, sizeof(float));
	LockedBuffer lockedUiVertexAttributeBuffer = this->lockVertexAttributeBuffer(this->uiVertexAttributeBufferID);
	Span<float> uiVertexAttributeComponents = lockedUiVertexAttributeBuffer.getFloats();
	uiVertexAttributeComponents[0] = 0.0f;
	uiVertexAttributeComponents[1] = 0.0f;

	uiVertexAttributeComponents[2] = 0.0f;
	uiVertexAttributeComponents[3] = 1.0f;

	uiVertexAttributeComponents[4] = 1.0f;
	uiVertexAttributeComponents[5] = 1.0f;

	uiVertexAttributeComponents[6] = 1.0f;
	uiVertexAttributeComponents[7] = 1.0f;

	uiVertexAttributeComponents[8] = 1.0f;
	uiVertexAttributeComponents[9] = 0.0f;

	uiVertexAttributeComponents[10] = 0.0f;
	uiVertexAttributeComponents[11] = 0.0f;

	this->unlockVertexAttributeBuffer(this->uiVertexAttributeBufferID);

	return true;
}

void VulkanRenderBackend::shutdown()
{
	if (this->device)
	{
		this->uiVertexAttributeBufferID = -1;
		this->uiVertexPositionBufferID = -1;

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

		this->uiTextureHeapManagerStaging.freeAllocations();
		this->uiTextureHeapManagerStaging.clear();

		this->uiTextureHeapManagerDeviceLocal.freeAllocations();
		this->uiTextureHeapManagerDeviceLocal.clear();

		this->objectTextureHeapManagerStaging.freeAllocations();
		this->objectTextureHeapManagerStaging.clear();

		this->objectTextureHeapManagerDeviceLocal.freeAllocations();
		this->objectTextureHeapManagerDeviceLocal.clear();

		this->uniformBufferHeapManagerStaging.freeAllocations();
		this->uniformBufferHeapManagerStaging.clear();

		this->uniformBufferHeapManagerDeviceLocal.freeAllocations();
		this->uniformBufferHeapManagerDeviceLocal.clear();

		this->indexBufferHeapManagerStaging.freeAllocations();
		this->indexBufferHeapManagerStaging.clear();

		this->indexBufferHeapManagerDeviceLocal.freeAllocations();
		this->indexBufferHeapManagerDeviceLocal.clear();

		this->vertexBufferHeapManagerStaging.freeAllocations();
		this->vertexBufferHeapManagerStaging.clear();

		this->vertexBufferHeapManagerDeviceLocal.freeAllocations();
		this->vertexBufferHeapManagerDeviceLocal.clear();

		for (VulkanMaterial &material : this->materialPool.values)
		{
			if (material.descriptorSet)
			{
				this->device.freeDescriptorSets(this->materialDescriptorPool, material.descriptorSet);
			}
		}

		this->materialPool.clear();

		for (VulkanTexture &texture : this->uiTexturePool.values)
		{
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

			if (texture.image)
			{
				this->device.destroyImage(texture.image);
			}
		}

		this->uiTexturePool.clear();

		for (VulkanTexture &texture : this->objectTexturePool.values)
		{
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

		for (VulkanPipeline &pipeline : this->graphicsPipelines)
		{
			if (pipeline.pipeline)
			{
				this->device.destroyPipeline(pipeline.pipeline);
				pipeline.pipeline = nullptr;
			}
		}

		this->graphicsPipelines.clear();

		for (vk::PipelineLayout pipelineLayout : this->pipelineLayouts)
		{
			if (pipelineLayout)
			{
				this->device.destroyPipelineLayout(pipelineLayout);
			}
		}

		this->pipelineLayouts.clear();

		for (vk::DescriptorSet descriptorSet : this->uiTextureDescriptorSets.values)
		{
			if (descriptorSet)
			{
				this->device.freeDescriptorSets(this->materialDescriptorPool, descriptorSet);
			}
		}

		this->uiTextureDescriptorSets.clear();

		if (this->uiMaterialDescriptorSetLayout)
		{
			this->device.destroyDescriptorSetLayout(this->uiMaterialDescriptorSetLayout);
			this->uiMaterialDescriptorSetLayout = nullptr;
		}

		if (this->materialDescriptorSetLayout)
		{
			this->device.destroyDescriptorSetLayout(this->materialDescriptorSetLayout);
			this->materialDescriptorSetLayout = nullptr;
		}

		if (this->transformDescriptorSetLayout)
		{
			this->device.destroyDescriptorSetLayout(this->transformDescriptorSetLayout);
			this->transformDescriptorSetLayout = nullptr;
		}

		if (this->globalDescriptorSetLayout)
		{
			this->device.destroyDescriptorSetLayout(this->globalDescriptorSetLayout);
			this->globalDescriptorSetLayout = nullptr;
		}

		if (this->materialDescriptorPool)
		{
			this->device.destroyDescriptorPool(this->materialDescriptorPool);
			this->materialDescriptorPool = nullptr;
		}

		if (this->transformDescriptorPool)
		{
			this->device.destroyDescriptorPool(this->transformDescriptorPool);
			this->transformDescriptorPool = nullptr;
		}

		if (this->globalDescriptorPool)
		{
			this->globalDescriptorSet = nullptr;

			this->device.destroyDescriptorPool(this->globalDescriptorPool);
			this->globalDescriptorPool = nullptr;
		}

		for (VulkanFragmentShader &shader : this->fragmentShaders)
		{
			if (shader.module)
			{
				this->device.destroyShaderModule(shader.module);
				shader.module = nullptr;
			}
		}

		this->fragmentShaders.clear();

		for (VulkanVertexShader &shader : this->vertexShaders)
		{
			if (shader.module)
			{
				this->device.destroyShaderModule(shader.module);
				shader.module = nullptr;
			}
		}

		this->vertexShaders.clear();

		this->freeCommands.clear();
		this->copyCommands.clear();

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

		if (this->depthImageView)
		{
			this->device.destroyImageView(this->depthImageView);
			this->depthImageView = nullptr;
		}

		if (this->depthImage)
		{
			this->device.destroyImage(this->depthImage);
			this->depthImage = nullptr;
		}

		if (this->depthDeviceMemory)
		{
			this->device.freeMemory(this->depthDeviceMemory);
			this->depthDeviceMemory = nullptr;
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

		this->sceneViewExtent = vk::Extent2D();
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
			this->physicalDeviceProperties = vk::PhysicalDeviceProperties();
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

void VulkanRenderBackend::resize(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight)
{
	for (vk::Framebuffer framebuffer : this->swapchainFramebuffers)
	{
		this->device.destroyFramebuffer(framebuffer);
	}

	this->swapchainFramebuffers.clear();

	if (this->renderPass)
	{
		this->device.destroyRenderPass(this->renderPass);
		this->renderPass = nullptr;
	}

	if (this->depthImageView)
	{
		this->device.destroyImageView(this->depthImageView);
		this->depthImageView = nullptr;
	}

	if (this->depthImage)
	{
		this->device.destroyImage(this->depthImage);
		this->depthImage = nullptr;
	}

	if (this->depthDeviceMemory)
	{
		this->device.freeMemory(this->depthDeviceMemory);
		this->depthDeviceMemory = nullptr;
	}

	for (vk::ImageView imageView : this->swapchainImageViews)
	{
		this->device.destroyImageView(imageView);
	}

	this->swapchainImageViews.clear();

	if (this->swapchain)
	{
		this->device.destroySwapchainKHR(this->swapchain);
		this->swapchain = nullptr;
	}

	vk::SurfaceCapabilitiesKHR surfaceCapabilities;
	if (!TryGetSurfaceCapabilities(this->physicalDevice, this->surface, &surfaceCapabilities))
	{
		DebugLogErrorFormat("Couldn't get surface capabilities for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	const int minWindowWidth = surfaceCapabilities.minImageExtent.width;
	const int maxWindowWidth = surfaceCapabilities.maxImageExtent.width;
	const int minWindowHeight = surfaceCapabilities.minImageExtent.height;
	const int maxWindowHeight = surfaceCapabilities.maxImageExtent.height;
	const bool isSurfaceCapableOfValidDimensions = (maxWindowWidth > 0) && (maxWindowHeight > 0);
	if (!isSurfaceCapableOfValidDimensions)
	{
		// Alt-tabbed out of borderless/exclusive fullscreen.
		return;
	}

	const bool isValidWindowWidth = (windowWidth >= minWindowWidth) && (windowWidth <= maxWindowWidth);
	const bool isValidWindowHeight = (windowHeight >= minWindowHeight) && (windowHeight <= maxWindowHeight);
	if (!isValidWindowWidth || !isValidWindowHeight)
	{
		DebugLogWarningFormat("Requested window dimensions %dx%d are outside of capabilities (min %dx%d, max %dx%d).",
			windowWidth, windowHeight, minWindowWidth, minWindowHeight, maxWindowWidth, maxWindowHeight);
		return;
	}

	this->swapchainExtent = vk::Extent2D(windowWidth, windowHeight);
	this->sceneViewExtent = vk::Extent2D(sceneViewWidth, sceneViewHeight);

	vk::SurfaceFormatKHR surfaceFormat;
	if (!TryGetSurfaceFormat(this->physicalDevice, this->surface, DefaultSwapchainSurfaceFormat, DefaultSwapchainColorSpace, &surfaceFormat))
	{
		DebugLogErrorFormat("Couldn't get surface format for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	const vk::PresentModeKHR presentMode = GetBestSwapchainPresentMode(this->physicalDevice, this->surface);

	if (!TryCreateSwapchain(this->device, this->surface, surfaceFormat, presentMode, surfaceCapabilities, this->swapchainExtent,
		this->graphicsQueueFamilyIndex, this->presentQueueFamilyIndex, &this->swapchain))
	{
		DebugLogErrorFormat("Couldn't create swapchain for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateSwapchainImageViews(this->device, this->swapchain, surfaceFormat, &this->swapchainImageViews))
	{
		DebugLogErrorFormat("Couldn't create swapchain image views for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	const vk::MemoryAllocateInfo depthMemoryAllocateInfo = CreateImageMemoryAllocateInfo(this->device, this->swapchainExtent.width, this->swapchainExtent.height,
		DepthBufferFormat, DepthBufferUsageFlags, this->physicalDevice);
	if (!TryAllocateMemory(this->device, depthMemoryAllocateInfo, &this->depthDeviceMemory))
	{
		DebugLogErrorFormat("Couldn't allocate depth image memory for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateImage(this->device, this->swapchainExtent.width, this->swapchainExtent.height, DepthBufferFormat, DepthBufferUsageFlags, this->graphicsQueueFamilyIndex, &this->depthImage))
	{
		DebugLogErrorFormat("Couldn't create depth image for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryBindImageToMemory(this->device, this->depthImage, this->depthDeviceMemory, 0))
	{
		DebugLogErrorFormat("Couldn't bind depth image to memory for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateImageView(this->device, DepthBufferFormat, vk::ImageAspectFlagBits::eDepth, this->depthImage, &this->depthImageView))
	{
		DebugLogErrorFormat("Couldn't create depth image view for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateSwapchainRenderPass(this->device, surfaceFormat, &this->renderPass))
	{
		DebugLogErrorFormat("Couldn't create swapchain render pass for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateSwapchainFramebuffers(this->device, this->swapchainImageViews, this->depthImageView, this->swapchainExtent, this->renderPass, &this->swapchainFramebuffers))
	{
		DebugLogErrorFormat("Couldn't create swapchain framebuffers for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}
}

void VulkanRenderBackend::handleRenderTargetsReset(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight)
{
	DebugNotImplementedMsg("handleRenderTargetsReset()");
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
	if (!TryCreateBufferAndBindWithHeap<float>(this->device, elementCount, VertexBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->vertexBufferHeapManagerDeviceLocal, &buffer, nullptr))
	{
		DebugLogErrorFormat("Couldn't create vertex position buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexPositionBufferPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap<float>(this->device, elementCount, VertexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->vertexBufferHeapManagerStaging, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create vertex position staging buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexBufferHeapManagerDeviceLocal.freeBufferMapping(buffer);
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
	auto commandBufferFunc = [this, id]()
	{
		VulkanBuffer *vertexPositionBuffer = this->vertexPositionBufferPool.tryGet(id);
		if (vertexPositionBuffer != nullptr)
		{
			if (vertexPositionBuffer->stagingBuffer)
			{
				this->vertexBufferHeapManagerStaging.freeBufferMapping(vertexPositionBuffer->stagingBuffer);
				this->device.destroyBuffer(vertexPositionBuffer->stagingBuffer);
			}

			if (vertexPositionBuffer->buffer)
			{
				this->vertexBufferHeapManagerDeviceLocal.freeBufferMapping(vertexPositionBuffer->buffer);
				this->device.destroyBuffer(vertexPositionBuffer->buffer);
			}

			this->vertexPositionBufferPool.free(id);
		}
	};

	this->freeCommands.emplace_back(std::move(commandBufferFunc));
}

LockedBuffer VulkanRenderBackend::lockVertexPositionBuffer(VertexPositionBufferID id)
{
	VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);
	const VulkanBufferVertexPositionInfo &vertexPositionInfo = vertexPositionBuffer.vertexPosition;
	return LockedBuffer(vertexPositionBuffer.stagingHostMappedBytes, vertexPositionInfo.vertexCount, vertexPositionInfo.bytesPerComponent, vertexPositionInfo.bytesPerComponent);
}

void VulkanRenderBackend::unlockVertexPositionBuffer(VertexPositionBufferID id)
{
	auto commandBufferFunc = [this, id]()
	{
		const VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);
		vk::Buffer buffer = vertexPositionBuffer.buffer;
		vk::Buffer stagingBuffer = vertexPositionBuffer.stagingBuffer;
		const int byteCount = vertexPositionBuffer.stagingHostMappedBytes.getCount();
		CopyToBufferDeviceLocal(stagingBuffer, buffer, 0, byteCount, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
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
	if (!TryCreateBufferAndBindWithHeap<float>(this->device, elementCount, VertexBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->vertexBufferHeapManagerDeviceLocal, &buffer, nullptr))
	{
		DebugLogErrorFormat("Couldn't create vertex attribute buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexAttributeBufferPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap<float>(this->device, elementCount, VertexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->vertexBufferHeapManagerStaging, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create vertex attribute staging buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexBufferHeapManagerDeviceLocal.freeBufferMapping(buffer);
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
	auto commandBufferFunc = [this, id]()
	{
		VulkanBuffer *vertexAttributeBuffer = this->vertexAttributeBufferPool.tryGet(id);
		if (vertexAttributeBuffer != nullptr)
		{
			if (vertexAttributeBuffer->stagingBuffer)
			{
				this->vertexBufferHeapManagerStaging.freeBufferMapping(vertexAttributeBuffer->stagingBuffer);
				this->device.destroyBuffer(vertexAttributeBuffer->stagingBuffer);
			}

			if (vertexAttributeBuffer->buffer)
			{
				this->vertexBufferHeapManagerDeviceLocal.freeBufferMapping(vertexAttributeBuffer->buffer);
				this->device.destroyBuffer(vertexAttributeBuffer->buffer);
			}

			this->vertexAttributeBufferPool.free(id);
		}
	};

	this->freeCommands.emplace_back(std::move(commandBufferFunc));
}

LockedBuffer VulkanRenderBackend::lockVertexAttributeBuffer(VertexAttributeBufferID id)
{
	VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);
	const VulkanBufferVertexAttributeInfo &vertexAttributeInfo = vertexAttributeBuffer.vertexAttribute;
	return LockedBuffer(vertexAttributeBuffer.stagingHostMappedBytes, vertexAttributeInfo.vertexCount, vertexAttributeInfo.bytesPerComponent, vertexAttributeInfo.bytesPerComponent);
}

void VulkanRenderBackend::unlockVertexAttributeBuffer(VertexAttributeBufferID id)
{
	auto commandBufferFunc = [this, id]()
	{
		const VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);
		vk::Buffer buffer = vertexAttributeBuffer.buffer;
		vk::Buffer stagingBuffer = vertexAttributeBuffer.stagingBuffer;
		const int byteCount = vertexAttributeBuffer.stagingHostMappedBytes.getCount();
		CopyToBufferDeviceLocal(stagingBuffer, buffer, 0, byteCount, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
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
	if (!TryCreateBufferAndBindWithHeap<int32_t>(this->device, indexCount, IndexBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->indexBufferHeapManagerDeviceLocal, &buffer, nullptr))
	{
		DebugLogErrorFormat("Couldn't create index buffer (indices: %d).", indexCount);
		this->indexBufferPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap<int32_t>(this->device, indexCount, IndexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->indexBufferHeapManagerStaging, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create index staging buffer (indices: %d).", indexCount);
		this->indexBufferHeapManagerDeviceLocal.freeBufferMapping(buffer);
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
	auto commandBufferFunc = [this, id]()
	{
		VulkanBuffer *indexBuffer = this->indexBufferPool.tryGet(id);
		if (indexBuffer != nullptr)
		{
			if (indexBuffer->stagingBuffer)
			{
				this->indexBufferHeapManagerStaging.freeBufferMapping(indexBuffer->stagingBuffer);
				this->device.destroyBuffer(indexBuffer->stagingBuffer);
			}

			if (indexBuffer->buffer)
			{
				this->indexBufferHeapManagerDeviceLocal.freeBufferMapping(indexBuffer->buffer);
				this->device.destroyBuffer(indexBuffer->buffer);
			}

			this->indexBufferPool.free(id);
		}
	};

	this->freeCommands.emplace_back(std::move(commandBufferFunc));
}

LockedBuffer VulkanRenderBackend::lockIndexBuffer(IndexBufferID id)
{
	VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);
	const VulkanBufferIndexInfo &indexInfo = indexBuffer.index;
	return LockedBuffer(indexBuffer.stagingHostMappedBytes, indexInfo.indexCount, indexInfo.bytesPerIndex, indexInfo.bytesPerIndex);
}

void VulkanRenderBackend::unlockIndexBuffer(IndexBufferID id)
{
	auto commandBufferFunc = [this, id]()
	{
		const VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);
		vk::Buffer buffer = indexBuffer.buffer;
		vk::Buffer stagingBuffer = indexBuffer.stagingBuffer;
		const int byteCount = indexBuffer.stagingHostMappedBytes.getCount();
		CopyToBufferDeviceLocal(stagingBuffer, buffer, 0, byteCount, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
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

	const int bytesPerStride = MathUtils::roundToGreaterMultipleOf(bytesPerElement, this->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);
	const int byteCountWithAlignedElements = elementCount * bytesPerStride;
	vk::Buffer buffer;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCountWithAlignedElements, UniformBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->uniformBufferHeapManagerDeviceLocal, &buffer, nullptr))
	{
		DebugLogErrorFormat("Couldn't create uniform buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, bytesPerElement, alignmentOfElement);
		this->uniformBufferPool.free(id);
		return -1;
	}

	vk::DescriptorSet descriptorSet;
	if (!TryCreateDescriptorSet(this->device, this->transformDescriptorSetLayout, this->transformDescriptorPool, &descriptorSet))
	{
		DebugLogErrorFormat("Couldn't create descriptor set for uniform buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, bytesPerElement, alignmentOfElement);
		this->uniformBufferHeapManagerDeviceLocal.freeBufferMapping(buffer);
		this->device.destroyBuffer(buffer);
		this->uniformBufferPool.free(id);
		return -1;
	}

	UpdateTransformDescriptorSet(this->device, descriptorSet, buffer, bytesPerStride);

	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCountWithAlignedElements, UniformBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->uniformBufferHeapManagerStaging, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create uniform staging buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, bytesPerElement, alignmentOfElement);
		this->uniformBufferHeapManagerDeviceLocal.freeBufferMapping(buffer);
		this->device.destroyBuffer(buffer);
		this->uniformBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	uniformBuffer.init(buffer, stagingBuffer, stagingHostMappedBytes);
	uniformBuffer.initUniform(elementCount, bytesPerElement, bytesPerStride, descriptorSet);

	return id;
}

void VulkanRenderBackend::freeUniformBuffer(UniformBufferID id)
{
	auto commandBufferFunc = [this, id]()
	{
		VulkanBuffer *uniformBuffer = this->uniformBufferPool.tryGet(id);
		if (uniformBuffer != nullptr)
		{
			if (uniformBuffer->stagingBuffer)
			{
				this->uniformBufferHeapManagerStaging.freeBufferMapping(uniformBuffer->stagingBuffer);
				this->device.destroyBuffer(uniformBuffer->stagingBuffer);
			}

			if (uniformBuffer->uniform.descriptorSet)
			{
				this->device.freeDescriptorSets(this->transformDescriptorPool, uniformBuffer->uniform.descriptorSet);
				uniformBuffer->uniform.descriptorSet = nullptr;
			}

			if (uniformBuffer->buffer)
			{
				this->uniformBufferHeapManagerDeviceLocal.freeBufferMapping(uniformBuffer->buffer);
				this->device.destroyBuffer(uniformBuffer->buffer);
			}

			this->uniformBufferPool.free(id);
		}
	};

	this->freeCommands.emplace_back(std::move(commandBufferFunc));
}

LockedBuffer VulkanRenderBackend::lockUniformBuffer(UniformBufferID id)
{
	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
	return LockedBuffer(uniformBuffer.stagingHostMappedBytes, uniformInfo.elementCount, uniformInfo.bytesPerElement, uniformInfo.bytesPerStride);
}

LockedBuffer VulkanRenderBackend::lockUniformBufferIndex(UniformBufferID id, int index)
{
	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
	Span<std::byte> stagingHostMappedBytesSlice(uniformBuffer.stagingHostMappedBytes.begin() + (index * uniformInfo.bytesPerStride), uniformInfo.bytesPerElement);
	return LockedBuffer(stagingHostMappedBytesSlice, 1, uniformInfo.bytesPerElement, uniformInfo.bytesPerStride);
}

void VulkanRenderBackend::unlockUniformBuffer(UniformBufferID id)
{
	auto commandBufferFunc = [this, id]()
	{
		const VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
		vk::Buffer buffer = uniformBuffer.buffer;
		vk::Buffer stagingBuffer = uniformBuffer.stagingBuffer;
		const int byteCount = uniformBuffer.stagingHostMappedBytes.getCount();
		CopyToBufferDeviceLocal(stagingBuffer, buffer, 0, byteCount, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
}

void VulkanRenderBackend::unlockUniformBufferIndex(UniformBufferID id, int index)
{
	auto commandBufferFunc = [this, id, index]()
	{
		const VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
		vk::Buffer buffer = uniformBuffer.buffer;
		vk::Buffer stagingBuffer = uniformBuffer.stagingBuffer;
		const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
		const int byteOffset = index * uniformInfo.bytesPerElement;
		const int byteCount = uniformInfo.bytesPerElement;
		CopyToBufferDeviceLocal(stagingBuffer, buffer, byteOffset, byteCount, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
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
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, UniformBufferDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->uniformBufferHeapManagerDeviceLocal, &buffer, nullptr))
	{
		DebugLogErrorFormat("Couldn't create buffer for light.");
		this->lightPool.free(id);
		return -1;
	}

	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, UniformBufferStagingUsageFlags, this->graphicsQueueFamilyIndex, this->uniformBufferHeapManagerStaging, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create staging buffer for light.");
		this->uniformBufferHeapManagerDeviceLocal.freeBufferMapping(buffer);
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
	auto commandBufferFunc = [this, id]()
	{
		VulkanLight *light = this->lightPool.tryGet(id);
		if (light != nullptr)
		{
			if (light->stagingBuffer)
			{
				this->uniformBufferHeapManagerStaging.freeBufferMapping(light->stagingBuffer);
				this->device.destroyBuffer(light->stagingBuffer);
			}

			if (light->buffer)
			{
				this->uniformBufferHeapManagerDeviceLocal.freeBufferMapping(light->buffer);
				this->device.destroyBuffer(light->buffer);
			}

			this->lightPool.free(id);
		}
	};

	this->freeCommands.emplace_back(std::move(commandBufferFunc));
}

bool VulkanRenderBackend::populateLight(RenderLightID id, const Double3 &point, double startRadius, double endRadius)
{
	// @todo
	//DebugLogWarning("VulkanRenderBackend::populateLight() not implemented");
	return true;
}

ObjectTextureID VulkanRenderBackend::createObjectTexture(int width, int height, int bytesPerTexel)
{
	const ObjectTextureID textureID = this->objectTexturePool.alloc();
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate object texture with dims %dx%d and %d bytes per texel.", width, height, bytesPerTexel);
		return -1;
	}

	const vk::Format format = (bytesPerTexel == 1) ? ObjectTextureFormat8Bit : ObjectTextureFormat32Bit;
	const int byteCount = width * height * bytesPerTexel;

	vk::Image image;
	if (!TryCreateImageAndBindWithHeap(this->device, width, height, format, ObjectTextureDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->objectTextureHeapManagerDeviceLocal, &image))
	{
		DebugLogErrorFormat("Couldn't create image with dims %dx%d.", width, height);
		this->objectTexturePool.free(textureID);
		return -1;
	}

	vk::ImageView imageView;
	if (!TryCreateImageView(this->device, format, vk::ImageAspectFlagBits::eColor, image, &imageView))
	{
		DebugLogErrorFormat("Couldn't create image view with dims %dx%d.", width, height);
		this->objectTextureHeapManagerDeviceLocal.freeImageMapping(image);
		this->device.destroyImage(image);
		this->objectTexturePool.free(textureID);
		return -1;
	}

	vk::Sampler sampler;
	if (!TryCreateSampler(this->device, &sampler))
	{
		DebugLogErrorFormat("Couldn't create sampler for image with dims %dx%d.", width, height);
		this->device.destroyImageView(imageView);
		this->objectTextureHeapManagerDeviceLocal.freeImageMapping(image);
		this->device.destroyImage(image);
		this->objectTexturePool.free(textureID);
		return -1;
	}

	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, ObjectTextureStagingUsageFlags, this->graphicsQueueFamilyIndex, this->objectTextureHeapManagerStaging, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create buffer and map memory for object texture with dims %dx%d.", width, height);
		this->device.destroySampler(sampler);
		this->device.destroyImageView(imageView);
		this->objectTextureHeapManagerDeviceLocal.freeImageMapping(image);
		this->device.destroyImage(image);
		this->objectTexturePool.free(textureID);
		return -1;
	}

	VulkanTexture &texture = this->objectTexturePool.get(textureID);
	texture.init(width, height, bytesPerTexel, image, imageView, sampler, stagingBuffer, stagingHostMappedBytes);

	return textureID;
}

void VulkanRenderBackend::freeObjectTexture(ObjectTextureID id)
{
	auto commandBufferFunc = [this, id]()
	{
		VulkanTexture *texture = this->objectTexturePool.tryGet(id);
		if (texture != nullptr)
		{
			if (texture->stagingBuffer)
			{
				this->objectTextureHeapManagerStaging.freeBufferMapping(texture->stagingBuffer);
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

			if (texture->image)
			{
				this->objectTextureHeapManagerDeviceLocal.freeImageMapping(texture->image);
				this->device.destroyImage(texture->image);
			}

			this->objectTexturePool.free(id);
		}
	};

	this->freeCommands.emplace_back(std::move(commandBufferFunc));
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

LockedTexture VulkanRenderBackend::lockObjectTexture(ObjectTextureID id)
{
	VulkanTexture &texture = this->objectTexturePool.get(id);
	return LockedTexture(texture.stagingHostMappedBytes, texture.width, texture.height, texture.bytesPerTexel);
}

void VulkanRenderBackend::unlockObjectTexture(ObjectTextureID id)
{
	auto commandBufferFunc = [this, id]()
	{
		VulkanTexture &texture = this->objectTexturePool.get(id);
		const int width = texture.width;
		const int height = texture.height;
		const int bytesPerTexel = texture.bytesPerTexel;
		vk::Image image = texture.image;
		vk::Buffer stagingBuffer = texture.stagingBuffer;
		TransitionImageLayout(image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, this->commandBuffer);
		CopyBufferToImage(stagingBuffer, image, width, height, this->commandBuffer);
		TransitionImageLayout(image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
}

UiTextureID VulkanRenderBackend::createUiTexture(int width, int height)
{
	const UiTextureID textureID = this->uiTexturePool.alloc();
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate UI texture with dims %dx%d.", width, height);
		return -1;
	}

	constexpr int bytesPerTexel = 4;
	const int byteCount = width * height * bytesPerTexel;
	constexpr vk::Format format = UiTextureFormat;

	vk::Image image;
	if (!TryCreateImageAndBindWithHeap(this->device, width, height, format, UiTextureDeviceLocalUsageFlags, this->graphicsQueueFamilyIndex, this->uiTextureHeapManagerDeviceLocal, &image))
	{
		DebugLogErrorFormat("Couldn't create image with dims %dx%d.", width, height);
		this->uiTexturePool.free(textureID);
		return -1;
	}

	vk::ImageView imageView;
	if (!TryCreateImageView(this->device, format, vk::ImageAspectFlagBits::eColor, image, &imageView))
	{
		DebugLogErrorFormat("Couldn't create image view with dims %dx%d.", width, height);
		this->uiTextureHeapManagerDeviceLocal.freeImageMapping(image);
		this->device.destroyImage(image);
		this->uiTexturePool.free(textureID);
		return -1;
	}

	vk::Sampler sampler;
	if (!TryCreateSampler(this->device, &sampler))
	{
		DebugLogErrorFormat("Couldn't create sampler for image with dims %dx%d.", width, height);
		this->device.destroyImageView(imageView);
		this->uiTextureHeapManagerDeviceLocal.freeImageMapping(image);
		this->device.destroyImage(image);
		this->uiTexturePool.free(textureID);
		return -1;
	}

	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, UiTextureStagingUsageFlags, this->graphicsQueueFamilyIndex, this->uiTextureHeapManagerStaging, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create buffer and bind memory for UI texture with dims %dx%d.", width, height);
		this->device.destroySampler(sampler);
		this->device.destroyImageView(imageView);
		this->uiTextureHeapManagerDeviceLocal.freeImageMapping(image);
		this->device.destroyImage(image);
		this->uiTexturePool.free(textureID);
		return -1;
	}

	vk::DescriptorSet descriptorSet; // Making a separate mapping from textures since UI shouldn't need materials.
	if (!TryCreateDescriptorSet(this->device, this->uiMaterialDescriptorSetLayout, this->materialDescriptorPool, &descriptorSet))
	{
		DebugLogErrorFormat("Couldn't create descriptor set for UI texture with dims %dx%d.", width, height);
		this->device.destroyBuffer(stagingBuffer);
		this->device.destroySampler(sampler);
		this->device.destroyImageView(imageView);
		this->uiTextureHeapManagerDeviceLocal.freeImageMapping(image);
		this->device.destroyImage(image);
		this->uiTexturePool.free(textureID);
		return -1;
	}

	UpdateMaterialDescriptorSet(this->device, descriptorSet, imageView, sampler);
	this->uiTextureDescriptorSets.emplace(textureID, descriptorSet);

	VulkanTexture &texture = this->uiTexturePool.get(textureID);
	texture.init(width, height, bytesPerTexel, image, imageView, sampler, stagingBuffer, stagingHostMappedBytes);

	return textureID;
}

void VulkanRenderBackend::freeUiTexture(UiTextureID id)
{
	auto commandBufferFunc = [this, id]()
	{
		VulkanTexture *texture = this->uiTexturePool.tryGet(id);
		if (texture != nullptr)
		{
			if (texture->stagingBuffer)
			{
				this->uiTextureHeapManagerStaging.freeBufferMapping(texture->stagingBuffer);
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

			if (texture->image)
			{
				this->uiTextureHeapManagerDeviceLocal.freeImageMapping(texture->image);
				this->device.destroyImage(texture->image);
			}

			this->uiTexturePool.free(id);

			vk::DescriptorSet *descriptorSet = this->uiTextureDescriptorSets.find(id);
			if (descriptorSet != nullptr)
			{
				this->device.freeDescriptorSets(this->materialDescriptorPool, *descriptorSet);
				this->uiTextureDescriptorSets.erase(id);
			}
		}
	};

	this->freeCommands.emplace_back(std::move(commandBufferFunc));
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

LockedTexture VulkanRenderBackend::lockUiTexture(UiTextureID id)
{
	VulkanTexture &texture = this->uiTexturePool.get(id);
	return LockedTexture(texture.stagingHostMappedBytes, texture.width, texture.height, texture.bytesPerTexel);
}

void VulkanRenderBackend::unlockUiTexture(UiTextureID id)
{
	auto commandBufferFunc = [this, id]()
	{
		VulkanTexture &texture = this->uiTexturePool.get(id);
		const int width = texture.width;
		const int height = texture.height;
		DebugAssert(texture.bytesPerTexel == 4);
		vk::Image image = texture.image;
		vk::Buffer stagingBuffer = texture.stagingBuffer;
		TransitionImageLayout(image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, this->commandBuffer);
		CopyBufferToImage(stagingBuffer, image, width, height, this->commandBuffer);
		TransitionImageLayout(image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
}

RenderMaterialID VulkanRenderBackend::createMaterial(RenderMaterialKey key)
{
	const RenderMaterialID materialID = this->materialPool.alloc();
	if (materialID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate material ID for key (vertex shader %d, fragment shader %d, depth read %d, depth write %d, back-face culling %d).",
			key.vertexShaderType, key.pixelShaderType, key.enableDepthRead, key.enableDepthWrite, key.enableBackFaceCulling);
		return -1;
	}

	const VertexShaderType vertexShaderType = key.vertexShaderType;
	const PixelShaderType fragmentShaderType = key.pixelShaderType;
	constexpr bool enableAlphaBlend = false; // Materials don't need alpha blend, only UI does (and just for the reticle).
	const VulkanPipelineKeyCode pipelineKeyCode = MakePipelineKeyCode(vertexShaderType, fragmentShaderType, key.enableDepthRead, key.enableDepthWrite, key.enableBackFaceCulling, enableAlphaBlend);
	int pipelineIndex = -1;
	for (int i = 0; i < this->graphicsPipelines.getCount(); i++)
	{
		const VulkanPipeline &graphicsPipeline = this->graphicsPipelines[i];
		if (graphicsPipeline.keyCode == pipelineKeyCode)
		{
			pipelineIndex = i;
			break;
		}
	}

	if (pipelineIndex < 0)
	{
		DebugLogErrorFormat("Couldn't find pipeline for material key (vertex shader %d, fragment shader %d, depth read %d, depth write %d, back-face culling %d).",
			key.vertexShaderType, key.pixelShaderType, key.enableDepthRead, key.enableDepthWrite, key.enableBackFaceCulling);
		return -1;
	}

	const vk::PipelineLayout pipelineLayout = this->pipelineLayouts[pipelineIndex];
	const vk::Pipeline pipeline = this->graphicsPipelines[pipelineIndex].pipeline;

	vk::DescriptorSet descriptorSet;
	if (!TryCreateDescriptorSet(this->device, this->materialDescriptorSetLayout, this->materialDescriptorPool, &descriptorSet))
	{
		DebugLogErrorFormat("Couldn't create descriptor set for material key (vertex shader %d, fragment shader %d, depth read %d, depth write %d, back-face culling %d).",
			key.vertexShaderType, key.pixelShaderType, key.enableDepthRead, key.enableDepthWrite, key.enableBackFaceCulling);
		return -1;
	}

	const ObjectTextureID textureID = key.textureIDs[0]; // @todo need to get second texture ID if this material requires it
	const VulkanTexture &texture = this->objectTexturePool.get(textureID);
	UpdateMaterialDescriptorSet(this->device, descriptorSet, texture.imageView, texture.sampler);

	VulkanMaterial &material = this->materialPool.get(materialID);
	material.init(pipeline, pipelineLayout, descriptorSet);

	if (RenderShaderUtils::requiresPixelShaderParam(fragmentShaderType))
	{
		material.pushConstantTypes[0] = VulkanMaterialPushConstantType::PixelShaderParam;
	}

	return materialID;
}

void VulkanRenderBackend::freeMaterial(RenderMaterialID id)
{
	auto commandBufferFunc = [this, id]()
	{
		VulkanMaterial *material = this->materialPool.tryGet(id);
		if (material != nullptr)
		{
			if (material->descriptorSet)
			{
				this->device.freeDescriptorSets(this->materialDescriptorPool, material->descriptorSet);
			}

			this->materialPool.free(id);
		}
	};

	this->freeCommands.emplace_back(std::move(commandBufferFunc));
}

void VulkanRenderBackend::setMaterialParameterMeshLightingPercent(RenderMaterialID id, double value)
{
	VulkanMaterial *material = this->materialPool.tryGet(id);
	if (material == nullptr)
	{
		DebugLogErrorFormat("Missing material %d for updating mesh lighting percent to %.2f.", id, value);
		return;
	}

	material->meshLightPercent = static_cast<float>(value);
}

void VulkanRenderBackend::setMaterialParameterPixelShaderParam(RenderMaterialID id, double value)
{
	VulkanMaterial *material = this->materialPool.tryGet(id);
	if (material == nullptr)
	{
		DebugLogErrorFormat("Missing material %d for updating pixel shader param to %.2f.", id, value);
		return;
	}

	material->pixelShaderParam0 = static_cast<float>(value);
}

void VulkanRenderBackend::submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
	const RenderCamera &camera, const RenderFrameSettings &frameSettings)
{
	vk::SurfaceCapabilitiesKHR surfaceCapabilities;
	if (!TryGetSurfaceCapabilities(this->physicalDevice, this->surface, &surfaceCapabilities))
	{
		DebugLogErrorFormat("Couldn't get surface capabilities for checking window.");
		return;
	}

	const vk::Extent2D currentSwapchainExtent = surfaceCapabilities.currentExtent;
	const bool isWindowMinimized = currentSwapchainExtent.width == 0 || currentSwapchainExtent.height == 0;
	if (isWindowMinimized)
	{
		return;
	}

	constexpr uint64_t acquireTimeout = TIMEOUT_UNLIMITED;
	vk::ResultValue<uint32_t> acquiredSwapchainImageIndexResult = this->device.acquireNextImageKHR(this->swapchain, acquireTimeout, this->imageIsAvailableSemaphore);
	if (acquiredSwapchainImageIndexResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't acquire next swapchain image (%d).", acquiredSwapchainImageIndexResult.result);
		return;
	}

	const uint32_t acquiredSwapchainImageIndex = std::move(acquiredSwapchainImageIndexResult.value);
	const vk::Framebuffer acquiredSwapchainFramebuffer = this->swapchainFramebuffers[acquiredSwapchainImageIndex];

	const vk::Result commandBufferResetResult = this->commandBuffer.reset();
	if (commandBufferResetResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't reset command buffer (%d).", commandBufferResetResult);
		return;
	}

	vk::CommandBufferBeginInfo commandBufferBeginInfo;
	const vk::Result commandBufferBeginResult = this->commandBuffer.begin(commandBufferBeginInfo);
	if (commandBufferBeginResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't begin command buffer (%d).", commandBufferBeginResult);
		return;
	}

	if (!this->copyCommands.empty())
	{
		for (const std::function<void()> &copyCommand : this->copyCommands)
		{
			copyCommand();
		}

		this->copyCommands.clear();

		const vk::DependencyFlags dependencyFlags;
		vk::ArrayProxy<vk::MemoryBarrier> memoryBarriers;
		vk::ArrayProxy<vk::BufferMemoryBarrier> bufferMemoryBarriers;
		vk::ArrayProxy<vk::ImageMemoryBarrier> imageMemoryBarriers;
		this->commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eVertexShader,
			dependencyFlags,
			memoryBarriers,
			bufferMemoryBarriers,
			imageMemoryBarriers);
	}

	const Float4 clearColor(
		static_cast<float>(frameSettings.clearColor.r) / 255.0f,
		static_cast<float>(frameSettings.clearColor.g) / 255.0f,
		static_cast<float>(frameSettings.clearColor.b) / 255.0f,
		static_cast<float>(frameSettings.clearColor.a) / 255.0f);

	vk::ClearValue clearValues[2];
	clearValues[0].color = vk::ClearColorValue(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	vk::RenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.renderPass = this->renderPass;
	renderPassBeginInfo.framebuffer = acquiredSwapchainFramebuffer;
	renderPassBeginInfo.renderArea.extent = this->swapchainExtent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(std::size(clearValues));
	renderPassBeginInfo.pClearValues = clearValues;

	this->commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

	constexpr vk::PipelineBindPoint pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	constexpr vk::ArrayProxy<const uint32_t> emptyDynamicOffsets;

	if (renderCommandList.entryCount > 0)
	{
		vk::Viewport sceneViewport;
		sceneViewport.width = static_cast<float>(this->sceneViewExtent.width);
		sceneViewport.height = static_cast<float>(this->sceneViewExtent.height);
		sceneViewport.minDepth = 0.0f;
		sceneViewport.maxDepth = 1.0f;

		vk::Rect2D sceneViewportScissor;
		sceneViewportScissor.extent = this->sceneViewExtent;

		this->commandBuffer.setViewport(0, sceneViewport);
		this->commandBuffer.setScissor(0, sceneViewportScissor);

		DebugAssert(this->camera.matrixBytes.getCount() == this->camera.hostMappedBytes.getCount());
		Matrix4d projectionMatrix = camera.projectionMatrix;
		projectionMatrix.y.y = -projectionMatrix.y.y; // Flip Y so world is not upside down.
		this->camera.viewProjection = RendererUtils::matrix4DoubleToFloat(projectionMatrix * camera.viewMatrix);
		std::copy(this->camera.matrixBytes.begin(), this->camera.matrixBytes.end(), this->camera.hostMappedBytes.begin());

		// @todo light table + light level calculation
		const VulkanTexture &paletteTexture = this->objectTexturePool.get(frameSettings.paletteTextureID);
		UpdateGlobalDescriptorSet(this->device, this->globalDescriptorSet, this->camera.buffer, paletteTexture.imageView, paletteTexture.sampler);

		vk::Pipeline currentPipeline;
		VertexPositionBufferID currentVertexPositionBufferID = -1;
		VertexAttributeBufferID currentVertexTexCoordBufferID = -1;
		IndexBufferID currentIndexBufferID = -1;
		int currentIndexBufferIndexCount = 0;
		ObjectTextureID currentTextureID = -1;
		for (int i = 0; i < renderCommandList.entryCount; i++)
		{
			for (const RenderDrawCall &drawCall : renderCommandList.entries[i])
			{
				const VulkanMaterial &material = this->materialPool.get(drawCall.materialID);
				const vk::Pipeline pipeline = material.pipeline;
				const vk::PipelineLayout pipelineLayout = material.pipelineLayout;

				if (pipeline != currentPipeline)
				{
					currentPipeline = pipeline;
					this->commandBuffer.bindPipeline(pipelineBindPoint, pipeline);

					constexpr uint32_t globalDescriptorSetIndex = 0;
					this->commandBuffer.bindDescriptorSets(pipelineBindPoint, pipelineLayout, globalDescriptorSetIndex, this->globalDescriptorSet, emptyDynamicOffsets);
				}

				constexpr vk::DeviceSize bufferOffset = 0;

				const VertexPositionBufferID vertexPositionBufferID = drawCall.positionBufferID;
				if (vertexPositionBufferID != currentVertexPositionBufferID)
				{
					currentVertexPositionBufferID = vertexPositionBufferID;

					const VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(vertexPositionBufferID);
					const VulkanBufferVertexPositionInfo &vertexPositionInfo = vertexPositionBuffer.vertexPosition;
					this->commandBuffer.bindVertexBuffers(0, vertexPositionBuffer.buffer, bufferOffset);
				}

				const VertexAttributeBufferID vertexTexCoordsBufferID = drawCall.texCoordBufferID;
				if (vertexTexCoordsBufferID != currentVertexTexCoordBufferID)
				{
					currentVertexTexCoordBufferID = vertexTexCoordsBufferID;

					const VulkanBuffer &vertexTexCoordsBuffer = this->vertexAttributeBufferPool.get(vertexTexCoordsBufferID);
					const VulkanBufferVertexAttributeInfo &vertexTexCoordsInfo = vertexTexCoordsBuffer.vertexAttribute;
					this->commandBuffer.bindVertexBuffers(1, vertexTexCoordsBuffer.buffer, bufferOffset);
				}

				const IndexBufferID indexBufferID = drawCall.indexBufferID;
				if (indexBufferID != currentIndexBufferID)
				{
					currentIndexBufferID = indexBufferID;

					const VulkanBuffer &indexBuffer = this->indexBufferPool.get(indexBufferID);
					const VulkanBufferIndexInfo &indexInfo = indexBuffer.index;
					currentIndexBufferIndexCount = indexInfo.indexCount;

					this->commandBuffer.bindIndexBuffer(indexBuffer.buffer, bufferOffset, vk::IndexType::eUint32);
				}

				const VulkanBuffer &transformBuffer = this->uniformBufferPool.get(drawCall.transformBufferID);
				const VulkanBufferUniformInfo &transformBufferInfo = transformBuffer.uniform;
				constexpr uint32_t transformDescriptorSetIndex = 1;
				uint32_t transformBufferDynamicOffset = drawCall.transformIndex * transformBufferInfo.bytesPerStride;
				this->commandBuffer.bindDescriptorSets(pipelineBindPoint, pipelineLayout, transformDescriptorSetIndex, transformBufferInfo.descriptorSet, transformBufferDynamicOffset);

				constexpr uint32_t materialDescriptorSetIndex = 2;
				this->commandBuffer.bindDescriptorSets(pipelineBindPoint, pipelineLayout, materialDescriptorSetIndex, material.descriptorSet, emptyDynamicOffsets);

				uint32_t pushConstantOffset = 0;
				if (drawCall.preScaleTranslationBufferID >= 0)
				{
					const VulkanBuffer &preScaleTranslationBuffer = this->uniformBufferPool.get(drawCall.preScaleTranslationBufferID);
					const float *preScaleTranslationComponents = reinterpret_cast<const float*>(preScaleTranslationBuffer.stagingHostMappedBytes.begin());
					const float preScaleTranslation[] =
					{
						preScaleTranslationComponents[0],
						preScaleTranslationComponents[1],
						preScaleTranslationComponents[2],
						0.0f
					};

					this->commandBuffer.pushConstants<float>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, pushConstantOffset, preScaleTranslation);
					pushConstantOffset += sizeof(preScaleTranslation);
				}

				for (VulkanMaterialPushConstantType materialPushConstantType : material.pushConstantTypes)
				{
					switch (materialPushConstantType)
					{
					case VulkanMaterialPushConstantType::MeshLightPercent:
						this->commandBuffer.pushConstants<float>(pipelineLayout, vk::ShaderStageFlagBits::eFragment, pushConstantOffset, material.meshLightPercent);
						pushConstantOffset += sizeof(float);
						break;
					case VulkanMaterialPushConstantType::PixelShaderParam:
						this->commandBuffer.pushConstants<float>(pipelineLayout, vk::ShaderStageFlagBits::eFragment, pushConstantOffset, material.pixelShaderParam0);
						pushConstantOffset += sizeof(float);
						break;
					}
				}

				constexpr uint32_t meshInstanceCount = 1;
				this->commandBuffer.drawIndexed(currentIndexBufferIndexCount, meshInstanceCount, 0, 0, 0);
			}
		}
	}

	this->commandBuffer.nextSubpass(vk::SubpassContents::eInline);

	if (uiCommandList.entryCount > 0)
	{
		const vk::PipelineLayout uiPipelineLayout = this->pipelineLayouts[UiPipelineKeyIndex];
		const VulkanPipeline &uiPipeline = this->graphicsPipelines.get(UiPipelineKeyIndex);
		this->commandBuffer.bindPipeline(pipelineBindPoint, uiPipeline.pipeline);

		vk::Viewport uiViewport;
		uiViewport.width = static_cast<float>(this->swapchainExtent.width);
		uiViewport.height = static_cast<float>(this->swapchainExtent.height);
		uiViewport.minDepth = 0.0f;
		uiViewport.maxDepth = 1.0f;

		vk::Rect2D uiViewportScissor;
		uiViewportScissor.extent = this->swapchainExtent;

		this->commandBuffer.setViewport(0, uiViewport);
		this->commandBuffer.setScissor(0, uiViewportScissor);

		constexpr vk::DeviceSize bufferOffset = 0;
		const VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(this->uiVertexPositionBufferID);
		this->commandBuffer.bindVertexBuffers(0, vertexPositionBuffer.buffer, bufferOffset);

		const VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(this->uiVertexPositionBufferID);
		this->commandBuffer.bindVertexBuffers(1, vertexAttributeBuffer.buffer, bufferOffset);

		for (int i = 0; i < uiCommandList.entryCount; i++)
		{
			for (const RenderElement2D &renderElement : uiCommandList.entries[i])
			{
				const Rect presentClipRect = renderElement.clipRect;
				if (!presentClipRect.isEmpty())
				{
					const vk::Offset2D clipOffset(presentClipRect.x, presentClipRect.y);
					const vk::Extent2D clipExtent(presentClipRect.width, presentClipRect.height);
					const vk::Rect2D clipScissor(clipOffset, clipExtent);
					this->commandBuffer.setScissor(0, clipScissor);
				}

				const UiTextureID textureID = renderElement.id;
				const vk::DescriptorSet *textureDescriptorSet = this->uiTextureDescriptorSets.find(textureID);
				if (textureDescriptorSet == nullptr)
				{
					DebugLogErrorFormat("Couldn't find descriptor set for UI texture %d.", textureID);
					continue;
				}

				this->commandBuffer.bindDescriptorSets(pipelineBindPoint, uiPipelineLayout, 0, *textureDescriptorSet, emptyDynamicOffsets);

				const Rect presentRect = renderElement.rect;
				const float uiVertexShaderPushConstants[] =
				{
					static_cast<float>(presentRect.x),
					static_cast<float>(presentRect.y),
					static_cast<float>(presentRect.width),
					static_cast<float>(presentRect.height),
					static_cast<float>(this->swapchainExtent.width),
					static_cast<float>(this->swapchainExtent.height)
				};

				this->commandBuffer.pushConstants<float>(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, uiVertexShaderPushConstants);

				const int uiVertexCount = vertexPositionBuffer.vertexPosition.vertexCount;
				constexpr int uiInstanceCount = 1;
				this->commandBuffer.draw(uiVertexCount, uiInstanceCount, 0, 0);

				if (!presentClipRect.isEmpty())
				{
					this->commandBuffer.setScissor(0, uiViewportScissor);
				}
			}
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

	const vk::Result waitForFrameCompletionResult = this->presentQueue.waitIdle();
	if (waitForFrameCompletionResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't wait idle for frame completion (%d).", waitForFrameCompletionResult);
		return;
	}

	if (!this->freeCommands.empty())
	{
		for (const std::function<void()> &func : this->freeCommands)
		{
			func();
		}

		this->freeCommands.clear();
	}
}
