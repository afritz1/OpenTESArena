#include <algorithm>
#include <cstring>
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
#include "../Math/BoundingBox.h"
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
	constexpr vk::Format SwapchainImageFormat = vk::Format::eB8G8R8A8Unorm; // 0xAARRGGBB in little endian, note that vkFormats are memory layouts, not channel orders.
	constexpr vk::Format ColorBufferFormat = vk::Format::eR8Uint;
	constexpr vk::Format DepthBufferFormat = vk::Format::eD32Sfloat;
	constexpr vk::Format ObjectTextureFormat8Bit = vk::Format::eR8Uint;
	constexpr vk::Format ObjectTextureFormat32Bit = vk::Format::eB8G8R8A8Unorm;
	constexpr vk::Format UiTextureFormat = ObjectTextureFormat32Bit;

	constexpr vk::ImageUsageFlags SwapchainImageUsageFlags = vk::ImageUsageFlagBits::eColorAttachment;
	constexpr vk::ImageUsageFlags ColorBufferUsageFlags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment;
	constexpr vk::ImageUsageFlags DepthBufferUsageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eDepthStencilAttachment;

	constexpr vk::ColorSpaceKHR SwapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

	// Size of each new individually-created heap in bytes requested from the driver. A single memory allocation (including alignment) cannot exceed this.
	constexpr int BYTES_PER_HEAP_VERTEX_BUFFERS = 1 << 22;
	constexpr int BYTES_PER_HEAP_INDEX_BUFFERS = BYTES_PER_HEAP_VERTEX_BUFFERS;
	constexpr int BYTES_PER_HEAP_UNIFORM_BUFFERS = 1 << 23;
	constexpr int BYTES_PER_HEAP_STORAGE_BUFFERS = 1 << 27;
	constexpr int BYTES_PER_HEAP_TEXTURES = 1 << 24;

	constexpr vk::BufferUsageFlags VertexBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags VertexBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer;
	constexpr vk::BufferUsageFlags IndexBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags IndexBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer;
	constexpr vk::BufferUsageFlags UniformBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags UniformBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer;
	constexpr vk::BufferUsageFlags StorageBufferStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::BufferUsageFlags StorageBufferDeviceLocalUsageFlags = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer;

	constexpr vk::BufferUsageFlags ObjectTextureStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::ImageUsageFlags ObjectTextureDeviceLocalUsageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
	constexpr vk::BufferUsageFlags UiTextureStagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;
	constexpr vk::ImageUsageFlags UiTextureDeviceLocalUsageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;

	constexpr int MaxGlobalUniformBufferDescriptors = 48;
	constexpr int MaxGlobalStorageBufferDescriptors = 16;
	constexpr int MaxGlobalImageDescriptors = 32;
	constexpr int MaxGlobalPoolDescriptorSets = MaxGlobalUniformBufferDescriptors + MaxGlobalStorageBufferDescriptors + MaxGlobalImageDescriptors;

	constexpr int MaxTransformUniformBufferDynamicDescriptors = 32768; // @todo this could be reduced by doing one heap per UniformBufferID which supports 4096 entity transforms etc
	constexpr int MaxTransformPoolDescriptorSets = MaxTransformUniformBufferDynamicDescriptors;

	constexpr int MaxMaterialImageDescriptors = 65536; // Lots of unique materials for entities. @todo texture atlasing
	constexpr int MaxMaterialUniformBufferDescriptors = 32768; // Need per-pixel/per-mesh lighting mode descriptor per material :/ @todo texture atlasing
	constexpr int MaxMaterialPoolDescriptorSets = MaxMaterialImageDescriptors + MaxMaterialUniformBufferDescriptors;

	// Scene descriptor set layout indices.
	constexpr int GlobalDescriptorSetLayoutIndex = 0;
	constexpr int LightDescriptorSetLayoutIndex = 1;
	constexpr int TransformDescriptorSetLayoutIndex = 2;
	constexpr int MaterialDescriptorSetLayoutIndex = 3;

	// Compute descriptor set layout indices.
	constexpr int LightBinningDescriptorSetLayoutIndex = 0;

	// UI descriptor set layout indices.
	constexpr int ConversionDescriptorSetLayoutIndex = 0;
	constexpr int UiMaterialDescriptorSetLayoutIndex = 1;

	constexpr std::pair<VertexShaderType, const char*> VertexShaderTypeFilenames[] =
	{
		{ VertexShaderType::Basic, "Basic" },
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
		{ PixelShaderType::AlphaTestedWithLightLevelOpacity, "AlphaTestedWithLightLevelOpacity" },
		{ PixelShaderType::AlphaTestedWithPreviousBrightnessLimit, "AlphaTestedWithPreviousBrightnessLimit" },
		{ PixelShaderType::AlphaTestedWithHorizonMirrorFirstPass, "AlphaTestedWithHorizonMirrorFirstPass" },
		{ PixelShaderType::AlphaTestedWithHorizonMirrorSecondPass, "AlphaTestedWithHorizonMirrorSecondPass" },
		{ PixelShaderType::UiTexture, "UiTexture" }
	};

	constexpr const char *LightBinningComputeShaderFilename = "LightBinning";

	constexpr const char *ConversionFragmentShaderFilename = "ColorBufferToSwapchainImage";

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
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTestedWithVariableTexCoordVMin, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTestedWithLightLevelOpacity, false, false, false, false),
		VulkanPipelineKey(VertexShaderType::Basic, PixelShaderType::AlphaTestedWithPreviousBrightnessLimit, false, false, false, false),

		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTested, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTestedWithPaletteIndexLookup, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTestedWithLightLevelOpacity, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTestedWithHorizonMirrorFirstPass, true, true, true, false),
		VulkanPipelineKey(VertexShaderType::Entity, PixelShaderType::AlphaTestedWithHorizonMirrorSecondPass, true, true, true, false),

		VulkanPipelineKey(VertexShaderType::UI, PixelShaderType::UiTexture, false, false, false, true)
	};

	constexpr int GetPipelineKeyIndex(VertexShaderType vertexShaderType, PixelShaderType fragmentShaderType, bool depthRead, bool depthWrite, bool backFaceCulling, bool alphaBlend)
	{
		for (int i = 0; i < static_cast<int>(std::size(RequiredPipelines)); i++)
		{
			const VulkanPipelineKey key = RequiredPipelines[i];
			if ((key.vertexShaderType == vertexShaderType) && (key.fragmentShaderType == fragmentShaderType) &&
				(key.depthRead == depthRead) && (key.depthWrite == depthWrite) &&
				(key.backFaceCulling == backFaceCulling && key.alphaBlend == alphaBlend))
			{
				return i;
			}
		}

		return -1;
	}

	constexpr int UiPipelineKeyIndex = GetPipelineKeyIndex(VertexShaderType::UI, PixelShaderType::UiTexture, false, false, false, true);
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
			const vk::PhysicalDevice physicalDevice = physicalDevices[i];
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

	bool TryCreateBuffersAndBindWithHeaps(vk::Device device, int byteCount, vk::BufferUsageFlags deviceLocalUsageFlags, vk::BufferUsageFlags stagingUsageFlags, uint32_t queueFamilyIndex,
		VulkanHeapManager &deviceLocalHeapManager, VulkanHeapManager &stagingHeapManager, vk::Buffer *outDeviceLocalBuffer, vk::Buffer *outStagingBuffer, Span<std::byte> *outHostMappedBytes)
	{
		vk::Buffer deviceLocalBuffer;
		if (!TryCreateBufferAndBindWithHeap(device, byteCount, deviceLocalUsageFlags, queueFamilyIndex, deviceLocalHeapManager, &deviceLocalBuffer, nullptr))
		{
			DebugLogError("Couldn't create and bind device-local buffer.");
			return false;
		}

		vk::Buffer stagingBuffer;
		Span<std::byte> hostMappedBytes;
		if (!TryCreateBufferAndBindWithHeap(device, byteCount, stagingUsageFlags, queueFamilyIndex, stagingHeapManager, &stagingBuffer, &hostMappedBytes))
		{
			DebugLogError("Couldn't create and bind staging buffer.");
			deviceLocalHeapManager.freeBufferMapping(deviceLocalBuffer);
			device.destroyBuffer(deviceLocalBuffer);
			return false;
		}

		*outDeviceLocalBuffer = deviceLocalBuffer;
		*outStagingBuffer = stagingBuffer;
		*outHostMappedBytes = hostMappedBytes;
		return true;
	}

	bool TryCreateBufferStagingAndDevice(vk::Device device, VulkanBuffer &buffer, int byteCount, vk::BufferUsageFlags usageFlags, uint32_t queueFamilyIndex,
		VulkanHeapManager &deviceLocalHeapManager, VulkanHeapManager &stagingHeapManager)
	{
		const vk::BufferUsageFlags deviceLocalUsageFlags = usageFlags | vk::BufferUsageFlagBits::eTransferDst;
		const vk::BufferUsageFlags stagingUsageFlags = vk::BufferUsageFlagBits::eTransferSrc;

		vk::Buffer deviceLocalBuffer;
		vk::Buffer stagingBuffer;
		Span<std::byte> stagingHostMappedBytes;
		if (!TryCreateBuffersAndBindWithHeaps(device, byteCount, deviceLocalUsageFlags, stagingUsageFlags, queueFamilyIndex,
			deviceLocalHeapManager, stagingHeapManager, &deviceLocalBuffer, &stagingBuffer, &stagingHostMappedBytes))
		{
			DebugLogError("Couldn't create buffers for host and device-local buffer.");
			return false;
		}

		buffer.init(deviceLocalBuffer, stagingBuffer, stagingHostMappedBytes);
		return true;
	};

	void CopyBufferToBuffer(vk::Buffer sourceBuffer, vk::Buffer destinationBuffer, int byteOffset, int byteCount, vk::CommandBuffer commandBuffer)
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

	void ApplyImageLayoutTransition(vk::Image image, vk::ImageAspectFlags imageAspectFlags, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask,
		vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::CommandBuffer commandBuffer)
	{
		vk::ImageMemoryBarrier barrier;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = imageAspectFlags;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		commandBuffer.pipelineBarrier(
			srcStageMask,
			dstStageMask,
			vk::DependencyFlags(),
			vk::ArrayProxy<vk::MemoryBarrier>(),
			vk::ArrayProxy<vk::BufferMemoryBarrier>(),
			barrier);
	}

	void ApplyColorImageLayoutTransition(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask,
		vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::CommandBuffer commandBuffer)
	{
		ApplyImageLayoutTransition(image, vk::ImageAspectFlagBits::eColor, oldLayout, newLayout, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask, commandBuffer);
	}

	void ApplyDepthImageLayoutTransition(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask,
		vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::CommandBuffer commandBuffer)
	{
		ApplyImageLayoutTransition(image, vk::ImageAspectFlagBits::eDepth, oldLayout, newLayout, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask, commandBuffer);
	}

	void CopyBufferToImage(vk::Buffer sourceBuffer, vk::Image destinationImage, int imageWidth, int imageHeight, vk::CommandBuffer commandBuffer)
	{
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

		commandBuffer.copyBufferToImage(sourceBuffer, destinationImage, vk::ImageLayout::eTransferDstOptimal, bufferImageCopy);
	}

	void CopyColorImageToImage(vk::Image srcImage, vk::Image dstImage, vk::Extent2D extent, vk::CommandBuffer commandBuffer)
	{
		vk::ImageCopy imageCopy;
		imageCopy.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageCopy.srcSubresource.mipLevel = 0;
		imageCopy.srcSubresource.baseArrayLayer = 0;
		imageCopy.srcSubresource.layerCount = 1;
		imageCopy.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageCopy.dstSubresource.mipLevel = 0;
		imageCopy.dstSubresource.baseArrayLayer = 0;
		imageCopy.dstSubresource.layerCount = 1;
		imageCopy.extent = vk::Extent3D(extent.width, extent.height, 1);

		commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, dstImage, vk::ImageLayout::eTransferDstOptimal, imageCopy);
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
			SDL_Vulkan_GetDrawableSize(window, &windowWidth, &windowHeight);
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
		swapchainCreateInfo.imageUsage = SwapchainImageUsageFlags;

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

	// Owned by swapchain, do not free.
	std::vector<vk::Image> GetSwapchainImages(vk::Device device, vk::SwapchainKHR swapchain)
	{
		vk::ResultValue<std::vector<vk::Image>> swapchainImagesResult = device.getSwapchainImagesKHR(swapchain);
		if (swapchainImagesResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't query device getSwapchainImagesKHR() (%d).", swapchainImagesResult.result);
			return std::vector<vk::Image>();
		}

		const std::vector<vk::Image> swapchainImages = std::move(swapchainImagesResult.value);
		if (swapchainImages.empty())
		{
			DebugLogErrorFormat("No swapchain images available.");
			return std::vector<vk::Image>();
		}

		return swapchainImages;
	}

	bool TryCreateSwapchainImageViews(vk::Device device, Span<const vk::Image> swapchainImages, vk::SurfaceFormatKHR surfaceFormat, Buffer<vk::ImageView> *outImageViews)
	{
		outImageViews->init(swapchainImages.getCount());

		for (int i = 0; i < swapchainImages.getCount(); i++)
		{
			if (!TryCreateImageView(device, surfaceFormat.format, vk::ImageAspectFlagBits::eColor, swapchainImages[i], &(*outImageViews)[i]))
			{
				DebugLogErrorFormat("Couldn't create swapchain image view index %d.", i);
				return false;
			}
		}

		return true;
	}

	bool TryCreateSceneRenderPass(vk::Device device, vk::RenderPass *outRenderPass)
	{
		vk::AttachmentDescription colorAttachmentDescription;
		colorAttachmentDescription.format = ColorBufferFormat;
		colorAttachmentDescription.samples = vk::SampleCountFlagBits::e1;
		colorAttachmentDescription.loadOp = vk::AttachmentLoadOp::eLoad;
		colorAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachmentDescription.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
		colorAttachmentDescription.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentDescription depthAttachmentDescription;
		depthAttachmentDescription.format = DepthBufferFormat;
		depthAttachmentDescription.samples = vk::SampleCountFlagBits::e1;
		depthAttachmentDescription.loadOp = vk::AttachmentLoadOp::eLoad;
		depthAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
		depthAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		depthAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		depthAttachmentDescription.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthAttachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		const vk::AttachmentDescription attachmentDescriptions[] =
		{
			colorAttachmentDescription,
			depthAttachmentDescription
		};

		vk::AttachmentReference subpassColorAttachmentReference;
		subpassColorAttachmentReference.attachment = 0;
		subpassColorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal; // During rendering (doesn't have to match final layout).

		vk::AttachmentReference subpassDepthAttachmentReference;
		subpassDepthAttachmentReference.attachment = 1;
		subpassDepthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		vk::SubpassDescription subpassDescription;
		subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &subpassColorAttachmentReference;
		subpassDescription.pDepthStencilAttachment = &subpassDepthAttachmentReference;

		vk::SubpassDependency subpassDependency;
		subpassDependency.srcSubpass = 0;
		subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL; // Ensure color attachment writes are done before UI render pass reads it.
		subpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests;
		subpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eFragmentShader;
		subpassDependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eShaderRead;

		vk::RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(std::size(attachmentDescriptions));
		renderPassCreateInfo.pAttachments = attachmentDescriptions;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &subpassDependency;

		vk::ResultValue<vk::RenderPass> renderPassCreateResult = device.createRenderPass(renderPassCreateInfo);
		if (renderPassCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create scene render pass (%d).", renderPassCreateResult.result);
			return false;
		}

		*outRenderPass = std::move(renderPassCreateResult.value);
		return true;
	}

	bool TryCreateUiRenderPass(vk::Device device, vk::RenderPass *outRenderPass)
	{
		vk::AttachmentDescription colorAttachmentDescription;
		colorAttachmentDescription.format = SwapchainImageFormat;
		colorAttachmentDescription.samples = vk::SampleCountFlagBits::e1;
		colorAttachmentDescription.loadOp = vk::AttachmentLoadOp::eDontCare; // Conditionally cleared based on scene view.
		colorAttachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachmentDescription.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference subpassColorAttachmentReference;
		subpassColorAttachmentReference.attachment = 0;
		subpassColorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription subpassDescription;
		subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &subpassColorAttachmentReference;

		vk::RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colorAttachmentDescription;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpassDescription;

		vk::ResultValue<vk::RenderPass> renderPassCreateResult = device.createRenderPass(renderPassCreateInfo);
		if (renderPassCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create UI render pass (%d).", renderPassCreateResult.result);
			return false;
		}

		*outRenderPass = std::move(renderPassCreateResult.value);
		return true;
	}

	bool TryCreateSceneFramebuffer(vk::Device device, vk::ImageView colorImageView, vk::ImageView depthImageView, vk::Extent2D extent, vk::RenderPass renderPass, vk::Framebuffer *outFramebuffer)
	{
		const vk::ImageView attachmentImageViews[] = { colorImageView, depthImageView };

		vk::FramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(std::size(attachmentImageViews));
		framebufferCreateInfo.pAttachments = attachmentImageViews;
		framebufferCreateInfo.width = extent.width;
		framebufferCreateInfo.height = extent.height;
		framebufferCreateInfo.layers = 1;

		vk::ResultValue<vk::Framebuffer> framebufferCreateResult = device.createFramebuffer(framebufferCreateInfo);
		if (framebufferCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create scene framebuffer (%d).", framebufferCreateResult.result);
			return false;
		}

		*outFramebuffer = std::move(framebufferCreateResult.value);
		return true;
	}

	bool TryCreateUiFramebuffer(vk::Device device, vk::ImageView swapchainImageView, vk::Extent2D extent, vk::RenderPass renderPass, vk::Framebuffer *outFramebuffer)
	{
		vk::FramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &swapchainImageView;
		framebufferCreateInfo.width = extent.width;
		framebufferCreateInfo.height = extent.height;
		framebufferCreateInfo.layers = 1;

		vk::ResultValue<vk::Framebuffer> framebufferCreateResult = device.createFramebuffer(framebufferCreateInfo);
		if (framebufferCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create UI framebuffer (%d).", framebufferCreateResult.result);
			return false;
		}

		*outFramebuffer = std::move(framebufferCreateResult.value);
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

	void UpdateGlobalDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::Buffer cameraBuffer, vk::Buffer framebufferDimsBuffer, vk::Buffer ambientLightBuffer,
		vk::Buffer screenSpaceAnimBuffer, vk::ImageView sampledFramebufferImageView, vk::Sampler sampledFramebufferSampler, vk::ImageView paletteImageView, vk::Sampler paletteSampler,
		vk::ImageView lightTableImageView, vk::Sampler lightTableSampler, vk::ImageView skyBgImageView, vk::Sampler skyBgSampler, vk::Buffer horizonMirrorBuffer)
	{
		vk::DescriptorBufferInfo cameraDescriptorBufferInfo;
		cameraDescriptorBufferInfo.buffer = cameraBuffer;
		cameraDescriptorBufferInfo.offset = 0;
		cameraDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo framebufferDimsDescriptorBufferInfo;
		framebufferDimsDescriptorBufferInfo.buffer = framebufferDimsBuffer;
		framebufferDimsDescriptorBufferInfo.offset = 0;
		framebufferDimsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo ambientLightDescriptorBufferInfo;
		ambientLightDescriptorBufferInfo.buffer = ambientLightBuffer;
		ambientLightDescriptorBufferInfo.offset = 0;
		ambientLightDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo screenSpaceAnimDescriptorBufferInfo;
		screenSpaceAnimDescriptorBufferInfo.buffer = screenSpaceAnimBuffer;
		screenSpaceAnimDescriptorBufferInfo.offset = 0;
		screenSpaceAnimDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorImageInfo sampledFramebufferDescriptorImageInfo;
		sampledFramebufferDescriptorImageInfo.sampler = sampledFramebufferSampler;
		sampledFramebufferDescriptorImageInfo.imageView = sampledFramebufferImageView;
		sampledFramebufferDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::DescriptorImageInfo paletteDescriptorImageInfo;
		paletteDescriptorImageInfo.sampler = paletteSampler;
		paletteDescriptorImageInfo.imageView = paletteImageView;
		paletteDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::DescriptorImageInfo lightTableDescriptorImageInfo;
		lightTableDescriptorImageInfo.sampler = lightTableSampler;
		lightTableDescriptorImageInfo.imageView = lightTableImageView;
		lightTableDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::DescriptorImageInfo skyBgDescriptorImageInfo;
		skyBgDescriptorImageInfo.sampler = skyBgSampler;
		skyBgDescriptorImageInfo.imageView = skyBgImageView;
		skyBgDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::DescriptorBufferInfo horizonMirrorDescriptorBufferInfo;
		horizonMirrorDescriptorBufferInfo.buffer = horizonMirrorBuffer;
		horizonMirrorDescriptorBufferInfo.offset = 0;
		horizonMirrorDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet cameraWriteDescriptorSet;
		cameraWriteDescriptorSet.dstSet = descriptorSet;
		cameraWriteDescriptorSet.dstBinding = 0;
		cameraWriteDescriptorSet.dstArrayElement = 0;
		cameraWriteDescriptorSet.descriptorCount = 1;
		cameraWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		cameraWriteDescriptorSet.pBufferInfo = &cameraDescriptorBufferInfo;

		vk::WriteDescriptorSet framebufferDimsWriteDescriptorSet;
		framebufferDimsWriteDescriptorSet.dstSet = descriptorSet;
		framebufferDimsWriteDescriptorSet.dstBinding = 1;
		framebufferDimsWriteDescriptorSet.dstArrayElement = 0;
		framebufferDimsWriteDescriptorSet.descriptorCount = 1;
		framebufferDimsWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		framebufferDimsWriteDescriptorSet.pBufferInfo = &framebufferDimsDescriptorBufferInfo;

		vk::WriteDescriptorSet ambientLightWriteDescriptorSet;
		ambientLightWriteDescriptorSet.dstSet = descriptorSet;
		ambientLightWriteDescriptorSet.dstBinding = 2;
		ambientLightWriteDescriptorSet.dstArrayElement = 0;
		ambientLightWriteDescriptorSet.descriptorCount = 1;
		ambientLightWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		ambientLightWriteDescriptorSet.pBufferInfo = &ambientLightDescriptorBufferInfo;

		vk::WriteDescriptorSet screenSpaceAnimWriteDescriptorSet;
		screenSpaceAnimWriteDescriptorSet.dstSet = descriptorSet;
		screenSpaceAnimWriteDescriptorSet.dstBinding = 3;
		screenSpaceAnimWriteDescriptorSet.dstArrayElement = 0;
		screenSpaceAnimWriteDescriptorSet.descriptorCount = 1;
		screenSpaceAnimWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		screenSpaceAnimWriteDescriptorSet.pBufferInfo = &screenSpaceAnimDescriptorBufferInfo;

		vk::WriteDescriptorSet sampledFramebufferWriteDescriptorSet;
		sampledFramebufferWriteDescriptorSet.dstSet = descriptorSet;
		sampledFramebufferWriteDescriptorSet.dstBinding = 4;
		sampledFramebufferWriteDescriptorSet.dstArrayElement = 0;
		sampledFramebufferWriteDescriptorSet.descriptorCount = 1;
		sampledFramebufferWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		sampledFramebufferWriteDescriptorSet.pImageInfo = &sampledFramebufferDescriptorImageInfo;

		vk::WriteDescriptorSet paletteWriteDescriptorSet;
		paletteWriteDescriptorSet.dstSet = descriptorSet;
		paletteWriteDescriptorSet.dstBinding = 5;
		paletteWriteDescriptorSet.dstArrayElement = 0;
		paletteWriteDescriptorSet.descriptorCount = 1;
		paletteWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		paletteWriteDescriptorSet.pImageInfo = &paletteDescriptorImageInfo;

		vk::WriteDescriptorSet lightTableWriteDescriptorSet;
		lightTableWriteDescriptorSet.dstSet = descriptorSet;
		lightTableWriteDescriptorSet.dstBinding = 6;
		lightTableWriteDescriptorSet.dstArrayElement = 0;
		lightTableWriteDescriptorSet.descriptorCount = 1;
		lightTableWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		lightTableWriteDescriptorSet.pImageInfo = &lightTableDescriptorImageInfo;

		vk::WriteDescriptorSet skyBgWriteDescriptorSet;
		skyBgWriteDescriptorSet.dstSet = descriptorSet;
		skyBgWriteDescriptorSet.dstBinding = 7;
		skyBgWriteDescriptorSet.dstArrayElement = 0;
		skyBgWriteDescriptorSet.descriptorCount = 1;
		skyBgWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		skyBgWriteDescriptorSet.pImageInfo = &skyBgDescriptorImageInfo;

		vk::WriteDescriptorSet horizonMirrorWriteDescriptorSet;
		horizonMirrorWriteDescriptorSet.dstSet = descriptorSet;
		horizonMirrorWriteDescriptorSet.dstBinding = 8;
		horizonMirrorWriteDescriptorSet.dstArrayElement = 0;
		horizonMirrorWriteDescriptorSet.descriptorCount = 1;
		horizonMirrorWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		horizonMirrorWriteDescriptorSet.pBufferInfo = &horizonMirrorDescriptorBufferInfo;

		const vk::WriteDescriptorSet writeDescriptorSets[] =
		{
			cameraWriteDescriptorSet,
			framebufferDimsWriteDescriptorSet,
			ambientLightWriteDescriptorSet,
			screenSpaceAnimWriteDescriptorSet,
			sampledFramebufferWriteDescriptorSet,
			paletteWriteDescriptorSet,
			lightTableWriteDescriptorSet,
			skyBgWriteDescriptorSet,
			horizonMirrorWriteDescriptorSet
		};

		device.updateDescriptorSets(writeDescriptorSets, vk::ArrayProxy<vk::CopyDescriptorSet>());
	}

	void UpdateLightDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::Buffer lightsBuffer, vk::Buffer lightBinsBuffer, vk::Buffer lightBinLightCountsBuffer,
		vk::Buffer lightBinDimsBuffer, vk::Buffer ditherBuffer)
	{
		vk::DescriptorBufferInfo lightsDescriptorBufferInfo;
		lightsDescriptorBufferInfo.buffer = lightsBuffer;
		lightsDescriptorBufferInfo.offset = 0;
		lightsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo lightBinsDescriptorBufferInfo;
		lightBinsDescriptorBufferInfo.buffer = lightBinsBuffer;
		lightBinsDescriptorBufferInfo.offset = 0;
		lightBinsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo lightBinLightCountsDescriptorBufferInfo;
		lightBinLightCountsDescriptorBufferInfo.buffer = lightBinLightCountsBuffer;
		lightBinLightCountsDescriptorBufferInfo.offset = 0;
		lightBinLightCountsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo lightBinDimsDescriptorBufferInfo;
		lightBinDimsDescriptorBufferInfo.buffer = lightBinDimsBuffer;
		lightBinDimsDescriptorBufferInfo.offset = 0;
		lightBinDimsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo ditherBufferDescriptorBufferInfo;
		ditherBufferDescriptorBufferInfo.buffer = ditherBuffer;
		ditherBufferDescriptorBufferInfo.offset = 0;
		ditherBufferDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet lightsWriteDescriptorSet;
		lightsWriteDescriptorSet.dstSet = descriptorSet;
		lightsWriteDescriptorSet.dstBinding = 0;
		lightsWriteDescriptorSet.dstArrayElement = 0;
		lightsWriteDescriptorSet.descriptorCount = 1;
		lightsWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		lightsWriteDescriptorSet.pBufferInfo = &lightsDescriptorBufferInfo;

		vk::WriteDescriptorSet lightBinsWriteDescriptorSet;
		lightBinsWriteDescriptorSet.dstSet = descriptorSet;
		lightBinsWriteDescriptorSet.dstBinding = 1;
		lightBinsWriteDescriptorSet.dstArrayElement = 0;
		lightBinsWriteDescriptorSet.descriptorCount = 1;
		lightBinsWriteDescriptorSet.descriptorType = vk::DescriptorType::eStorageBuffer;
		lightBinsWriteDescriptorSet.pBufferInfo = &lightBinsDescriptorBufferInfo;

		vk::WriteDescriptorSet lightBinLightCountsWriteDescriptorSet;
		lightBinLightCountsWriteDescriptorSet.dstSet = descriptorSet;
		lightBinLightCountsWriteDescriptorSet.dstBinding = 2;
		lightBinLightCountsWriteDescriptorSet.dstArrayElement = 0;
		lightBinLightCountsWriteDescriptorSet.descriptorCount = 1;
		lightBinLightCountsWriteDescriptorSet.descriptorType = vk::DescriptorType::eStorageBuffer;
		lightBinLightCountsWriteDescriptorSet.pBufferInfo = &lightBinLightCountsDescriptorBufferInfo;

		vk::WriteDescriptorSet ditherBufferWriteDescriptorSet;
		ditherBufferWriteDescriptorSet.dstSet = descriptorSet;
		ditherBufferWriteDescriptorSet.dstBinding = 3;
		ditherBufferWriteDescriptorSet.dstArrayElement = 0;
		ditherBufferWriteDescriptorSet.descriptorCount = 1;
		ditherBufferWriteDescriptorSet.descriptorType = vk::DescriptorType::eStorageBuffer;
		ditherBufferWriteDescriptorSet.pBufferInfo = &ditherBufferDescriptorBufferInfo;

		vk::WriteDescriptorSet lightBinDimsWriteDescriptorSet;
		lightBinDimsWriteDescriptorSet.dstSet = descriptorSet;
		lightBinDimsWriteDescriptorSet.dstBinding = 4;
		lightBinDimsWriteDescriptorSet.dstArrayElement = 0;
		lightBinDimsWriteDescriptorSet.descriptorCount = 1;
		lightBinDimsWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		lightBinDimsWriteDescriptorSet.pBufferInfo = &lightBinDimsDescriptorBufferInfo;

		const vk::WriteDescriptorSet writeDescriptorSets[] =
		{
			lightsWriteDescriptorSet,
			lightBinsWriteDescriptorSet,
			lightBinLightCountsWriteDescriptorSet,
			ditherBufferWriteDescriptorSet,
			lightBinDimsWriteDescriptorSet
		};

		device.updateDescriptorSets(writeDescriptorSets, vk::ArrayProxy<vk::CopyDescriptorSet>());
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

		device.updateDescriptorSets(transformWriteDescriptorSet, vk::ArrayProxy<vk::CopyDescriptorSet>());
	}

	void UpdateMaterialDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::ImageView texture0ImageView, vk::ImageView texture1ImageView, vk::Sampler textureSampler,
		vk::Buffer lightingModeBuffer)
	{
		vk::DescriptorImageInfo texture0DescriptorImageInfo;
		texture0DescriptorImageInfo.sampler = textureSampler;
		texture0DescriptorImageInfo.imageView = texture0ImageView;
		texture0DescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::DescriptorImageInfo texture1DescriptorImageInfo;
		texture1DescriptorImageInfo.sampler = textureSampler;
		texture1DescriptorImageInfo.imageView = texture1ImageView;
		texture1DescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::DescriptorBufferInfo lightingModeDescriptorBufferInfo;
		lightingModeDescriptorBufferInfo.buffer = lightingModeBuffer;
		lightingModeDescriptorBufferInfo.offset = 0;
		lightingModeDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet texture0WriteDescriptorSet;
		texture0WriteDescriptorSet.dstSet = descriptorSet;
		texture0WriteDescriptorSet.dstBinding = 0;
		texture0WriteDescriptorSet.dstArrayElement = 0;
		texture0WriteDescriptorSet.descriptorCount = 1;
		texture0WriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		texture0WriteDescriptorSet.pImageInfo = &texture0DescriptorImageInfo;

		vk::WriteDescriptorSet texture1WriteDescriptorSet;
		texture1WriteDescriptorSet.dstSet = descriptorSet;
		texture1WriteDescriptorSet.dstBinding = 1;
		texture1WriteDescriptorSet.dstArrayElement = 0;
		texture1WriteDescriptorSet.descriptorCount = 1;
		texture1WriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		texture1WriteDescriptorSet.pImageInfo = &texture1DescriptorImageInfo;

		vk::WriteDescriptorSet lightingModeWriteDescriptorSet;
		lightingModeWriteDescriptorSet.dstSet = descriptorSet;
		lightingModeWriteDescriptorSet.dstBinding = 2;
		lightingModeWriteDescriptorSet.dstArrayElement = 0;
		lightingModeWriteDescriptorSet.descriptorCount = 1;
		lightingModeWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		lightingModeWriteDescriptorSet.pBufferInfo = &lightingModeDescriptorBufferInfo;

		const vk::WriteDescriptorSet writeDescriptorSets[] =
		{
			texture0WriteDescriptorSet,
			texture1WriteDescriptorSet,
			lightingModeWriteDescriptorSet
		};

		device.updateDescriptorSets(writeDescriptorSets, vk::ArrayProxy<vk::CopyDescriptorSet>());
	}

	void UpdateLightBinningDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::Buffer cameraBuffer, vk::Buffer framebufferDimsBuffer,
		vk::Buffer lightsBuffer, vk::Buffer lightBinsBuffer, vk::Buffer lightBinLightCountsBuffer, vk::Buffer lightBinDimsBuffer)
	{
		vk::DescriptorBufferInfo cameraDescriptorBufferInfo;
		cameraDescriptorBufferInfo.buffer = cameraBuffer;
		cameraDescriptorBufferInfo.offset = 0;
		cameraDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo framebufferDimsDescriptorBufferInfo;
		framebufferDimsDescriptorBufferInfo.buffer = framebufferDimsBuffer;
		framebufferDimsDescriptorBufferInfo.offset = 0;
		framebufferDimsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo lightsDescriptorBufferInfo;
		lightsDescriptorBufferInfo.buffer = lightsBuffer;
		lightsDescriptorBufferInfo.offset = 0;
		lightsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo lightBinsDescriptorBufferInfo;
		lightBinsDescriptorBufferInfo.buffer = lightBinsBuffer;
		lightBinsDescriptorBufferInfo.offset = 0;
		lightBinsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo lightBinLightCountsDescriptorBufferInfo;
		lightBinLightCountsDescriptorBufferInfo.buffer = lightBinLightCountsBuffer;
		lightBinLightCountsDescriptorBufferInfo.offset = 0;
		lightBinLightCountsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::DescriptorBufferInfo lightBinDimsDescriptorBufferInfo;
		lightBinDimsDescriptorBufferInfo.buffer = lightBinDimsBuffer;
		lightBinDimsDescriptorBufferInfo.offset = 0;
		lightBinDimsDescriptorBufferInfo.range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet cameraWriteDescriptorSet;
		cameraWriteDescriptorSet.dstSet = descriptorSet;
		cameraWriteDescriptorSet.dstBinding = 0;
		cameraWriteDescriptorSet.dstArrayElement = 0;
		cameraWriteDescriptorSet.descriptorCount = 1;
		cameraWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		cameraWriteDescriptorSet.pBufferInfo = &cameraDescriptorBufferInfo;

		vk::WriteDescriptorSet framebufferDimsWriteDescriptorSet;
		framebufferDimsWriteDescriptorSet.dstSet = descriptorSet;
		framebufferDimsWriteDescriptorSet.dstBinding = 1;
		framebufferDimsWriteDescriptorSet.dstArrayElement = 0;
		framebufferDimsWriteDescriptorSet.descriptorCount = 1;
		framebufferDimsWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		framebufferDimsWriteDescriptorSet.pBufferInfo = &framebufferDimsDescriptorBufferInfo;

		vk::WriteDescriptorSet lightsWriteDescriptorSet;
		lightsWriteDescriptorSet.dstSet = descriptorSet;
		lightsWriteDescriptorSet.dstBinding = 2;
		lightsWriteDescriptorSet.dstArrayElement = 0;
		lightsWriteDescriptorSet.descriptorCount = 1;
		lightsWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		lightsWriteDescriptorSet.pBufferInfo = &lightsDescriptorBufferInfo;

		vk::WriteDescriptorSet lightBinsWriteDescriptorSet;
		lightBinsWriteDescriptorSet.dstSet = descriptorSet;
		lightBinsWriteDescriptorSet.dstBinding = 3;
		lightBinsWriteDescriptorSet.dstArrayElement = 0;
		lightBinsWriteDescriptorSet.descriptorCount = 1;
		lightBinsWriteDescriptorSet.descriptorType = vk::DescriptorType::eStorageBuffer;
		lightBinsWriteDescriptorSet.pBufferInfo = &lightBinsDescriptorBufferInfo;

		vk::WriteDescriptorSet lightBinLightCountsWriteDescriptorSet;
		lightBinLightCountsWriteDescriptorSet.dstSet = descriptorSet;
		lightBinLightCountsWriteDescriptorSet.dstBinding = 4;
		lightBinLightCountsWriteDescriptorSet.dstArrayElement = 0;
		lightBinLightCountsWriteDescriptorSet.descriptorCount = 1;
		lightBinLightCountsWriteDescriptorSet.descriptorType = vk::DescriptorType::eStorageBuffer;
		lightBinLightCountsWriteDescriptorSet.pBufferInfo = &lightBinLightCountsDescriptorBufferInfo;

		vk::WriteDescriptorSet lightBinDimsWriteDescriptorSet;
		lightBinDimsWriteDescriptorSet.dstSet = descriptorSet;
		lightBinDimsWriteDescriptorSet.dstBinding = 5;
		lightBinDimsWriteDescriptorSet.dstArrayElement = 0;
		lightBinDimsWriteDescriptorSet.descriptorCount = 1;
		lightBinDimsWriteDescriptorSet.descriptorType = vk::DescriptorType::eUniformBuffer;
		lightBinDimsWriteDescriptorSet.pBufferInfo = &lightBinDimsDescriptorBufferInfo;

		const vk::WriteDescriptorSet writeDescriptorSets[] =
		{
			cameraWriteDescriptorSet,
			framebufferDimsWriteDescriptorSet,
			lightsWriteDescriptorSet,
			lightBinsWriteDescriptorSet,
			lightBinLightCountsWriteDescriptorSet,
			lightBinDimsWriteDescriptorSet
		};

		device.updateDescriptorSets(writeDescriptorSets, vk::ArrayProxy<vk::CopyDescriptorSet>());
	}

	void UpdateConversionDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::ImageView framebufferImageView, vk::Sampler framebufferSampler,
		vk::ImageView paletteImageView, vk::Sampler paletteSampler)
	{
		vk::DescriptorImageInfo framebufferDescriptorImageInfo;
		framebufferDescriptorImageInfo.sampler = framebufferSampler;
		framebufferDescriptorImageInfo.imageView = framebufferImageView;
		framebufferDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::DescriptorImageInfo paletteDescriptorImageInfo;
		paletteDescriptorImageInfo.sampler = paletteSampler;
		paletteDescriptorImageInfo.imageView = paletteImageView;
		paletteDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::WriteDescriptorSet framebufferWriteDescriptorSet;
		framebufferWriteDescriptorSet.dstSet = descriptorSet;
		framebufferWriteDescriptorSet.dstBinding = 0;
		framebufferWriteDescriptorSet.dstArrayElement = 0;
		framebufferWriteDescriptorSet.descriptorCount = 1;
		framebufferWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		framebufferWriteDescriptorSet.pImageInfo = &framebufferDescriptorImageInfo;

		vk::WriteDescriptorSet paletteWriteDescriptorSet;
		paletteWriteDescriptorSet.dstSet = descriptorSet;
		paletteWriteDescriptorSet.dstBinding = 1;
		paletteWriteDescriptorSet.dstArrayElement = 0;
		paletteWriteDescriptorSet.descriptorCount = 1;
		paletteWriteDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		paletteWriteDescriptorSet.pImageInfo = &paletteDescriptorImageInfo;

		const vk::WriteDescriptorSet writeDescriptorSets[] = { framebufferWriteDescriptorSet, paletteWriteDescriptorSet };

		device.updateDescriptorSets(writeDescriptorSets, vk::ArrayProxy<vk::CopyDescriptorSet>());
	}

	void UpdateUiMaterialDescriptorSet(vk::Device device, vk::DescriptorSet descriptorSet, vk::ImageView textureImageView, vk::Sampler textureSampler)
	{
		vk::DescriptorImageInfo textureDescriptorImageInfo;
		textureDescriptorImageInfo.sampler = textureSampler;
		textureDescriptorImageInfo.imageView = textureImageView;
		textureDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

		vk::WriteDescriptorSet writeDescriptorSet;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.dstArrayElement = 0;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		writeDescriptorSet.pImageInfo = &textureDescriptorImageInfo;

		device.updateDescriptorSets(writeDescriptorSet, vk::ArrayProxy<vk::CopyDescriptorSet>());
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

		const bool requiresUiRectTransform = vertexShaderType == VertexShaderType::UI;
		if (requiresUiRectTransform)
		{
			addPushConstantRange(vk::ShaderStageFlagBits::eVertex, sizeof(float) * 6);
		}

		if (RenderShaderUtils::requiresMeshLightPercent(fragmentShaderType))
		{
			int byteCount = sizeof(float);
			if (RenderShaderUtils::requiresPixelShaderParam(fragmentShaderType))
			{
				byteCount += sizeof(float);
			}

			addPushConstantRange(vk::ShaderStageFlagBits::eFragment, byteCount);
		}
		else if (RenderShaderUtils::requiresPixelShaderParam(fragmentShaderType))
		{
			addPushConstantRange(vk::ShaderStageFlagBits::eFragment, sizeof(float));
		}

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
		vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass, vk::Pipeline *outPipeline)
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
		graphicsPipelineCreateInfo.subpass = 0;
		graphicsPipelineCreateInfo.basePipelineHandle = nullptr;

		vk::PipelineCache pipelineCache;
		vk::ResultValue<vk::Pipeline> graphicsPipelineResult = device.createGraphicsPipeline(pipelineCache, graphicsPipelineCreateInfo);
		if (graphicsPipelineResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create graphics pipeline (%d).", graphicsPipelineResult.result);
			return false;
		}

		*outPipeline = std::move(graphicsPipelineResult.value);
		return true;
	}

	bool TryCreateComputePipeline(vk::Device device, vk::ShaderModule computeShaderModule, vk::PipelineLayout pipelineLayout, vk::Pipeline *outPipeline)
	{
		vk::PipelineShaderStageCreateInfo computePipelineShaderStageCreateInfo;
		computePipelineShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eCompute;
		computePipelineShaderStageCreateInfo.module = computeShaderModule;
		computePipelineShaderStageCreateInfo.pName = "main";

		vk::ComputePipelineCreateInfo computePipelineCreateInfo;
		computePipelineCreateInfo.stage = computePipelineShaderStageCreateInfo;
		computePipelineCreateInfo.layout = pipelineLayout;

		vk::PipelineCache pipelineCache;
		vk::ResultValue<vk::Pipeline> computePipelineResult = device.createComputePipeline(pipelineCache, computePipelineCreateInfo);
		if (computePipelineResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create compute pipeline (%d).", computePipelineResult.result);
			return false;
		}

		*outPipeline = std::move(computePipelineResult.value);
		return true;
	}
}

// Vulkan lights
namespace
{
	static constexpr int MAX_LIGHTS_IN_FRUSTUM = 256; // Total allowed in frustum each frame, already sorted by distance to camera.
	static constexpr int MAX_LIGHTS_PER_LIGHT_BIN = 32; // Fraction of max frustum lights for a light bin.
	static constexpr int FLOATS_PER_OPTIMIZED_LIGHT = 8;

	// @todo these VulkanBuffers should probably be correctly initialized so their .uniform member contains the bytesPerStride from the memory requirements
	static constexpr int BYTES_PER_OPTIMIZED_LIGHT = sizeof(float) * FLOATS_PER_OPTIMIZED_LIGHT;
	static constexpr int BYTES_PER_LIGHT_BIN = sizeof(int) * MAX_LIGHTS_PER_LIGHT_BIN;
	static constexpr int BYTES_PER_LIGHT_BIN_LIGHT_COUNT = sizeof(int);

	constexpr int LIGHT_BIN_MIN_WIDTH = 16;
	constexpr int LIGHT_BIN_MAX_WIDTH = 32;
	constexpr int LIGHT_BIN_MIN_HEIGHT = LIGHT_BIN_MIN_WIDTH;
	constexpr int LIGHT_BIN_MAX_HEIGHT = LIGHT_BIN_MAX_WIDTH;
	constexpr int LIGHT_TYPICAL_BINS_PER_FRAME_BUFFER_WIDTH = 60;
	constexpr int LIGHT_TYPICAL_BINS_PER_FRAME_BUFFER_HEIGHT = 34;
	static_assert(MathUtils::isPowerOf2(LIGHT_BIN_MIN_WIDTH));
	static_assert(MathUtils::isPowerOf2(LIGHT_BIN_MAX_WIDTH));
	static_assert(MathUtils::isPowerOf2(LIGHT_BIN_MIN_HEIGHT));
	static_assert(MathUtils::isPowerOf2(LIGHT_BIN_MAX_HEIGHT));

	int GetLightBinWidth(int frameBufferWidth)
	{
		const int estimatedBinWidth = frameBufferWidth / LIGHT_TYPICAL_BINS_PER_FRAME_BUFFER_WIDTH;
		const int powerOfTwoBinWidth = MathUtils::roundToGreaterPowerOf2(estimatedBinWidth);
		return std::clamp(powerOfTwoBinWidth, LIGHT_BIN_MIN_WIDTH, LIGHT_BIN_MAX_WIDTH);
	}

	int GetLightBinHeight(int frameBufferHeight)
	{
		const int estimatedBinHeight = frameBufferHeight / LIGHT_TYPICAL_BINS_PER_FRAME_BUFFER_HEIGHT;
		const int powerOfTwoBinHeight = MathUtils::roundToGreaterPowerOf2(estimatedBinHeight);
		return std::clamp(powerOfTwoBinHeight, LIGHT_BIN_MIN_HEIGHT, LIGHT_BIN_MAX_HEIGHT);
	}

	int GetLightBinCountX(int frameBufferWidth, int binWidth)
	{
		return 1 + (frameBufferWidth / binWidth);
	}

	int GetLightBinCountY(int frameBufferHeight, int binHeight)
	{
		return 1 + (frameBufferHeight / binHeight);
	}

	int BinPixelToFrameBufferPixel(int bin, int binPixel, int binDimension)
	{
		return (bin * binDimension) + binPixel;
	}

	void PopulateLightGlobals(const VulkanBuffer &inputVisibleLightsBuffer, int clampedVisibleLightCount, const RenderCamera &camera, int frameBufferWidth, int frameBufferHeight,
		VulkanBuffer &optimizedVisibleLightsBuffer, VulkanBuffer &visibleLightBinsBuffer, VulkanBuffer &visibleLightBinLightCountsBuffer)
	{
		optimizedVisibleLightsBuffer.stagingHostMappedBytes.fill(std::byte(0));
		visibleLightBinsBuffer.stagingHostMappedBytes.fill(std::byte(0));
		visibleLightBinLightCountsBuffer.stagingHostMappedBytes.fill(std::byte(0));

		const std::byte *inputVisibleLightsBytes = inputVisibleLightsBuffer.stagingHostMappedBytes.begin();
		std::byte *optimizedVisibleLightsBytes = optimizedVisibleLightsBuffer.stagingHostMappedBytes.begin();

		// Read visible lights from uniform buffer and cache values to reduce shading work.
		for (int i = 0; i < clampedVisibleLightCount; i++)
		{
			constexpr int FloatsPerInputLight = 5;

			Span<const float> inputVisibleLightValues(reinterpret_cast<const float*>(inputVisibleLightsBytes + (inputVisibleLightsBuffer.uniform.bytesPerStride * i)), FloatsPerInputLight);
			const float inputVisibleLightPointX = inputVisibleLightValues[0];
			const float inputVisibleLightPointY = inputVisibleLightValues[1];
			const float inputVisibleLightPointZ = inputVisibleLightValues[2];
			const float inputVisibleLightStartRadius = inputVisibleLightValues[3];
			const float inputVisibleLightEndRadius = inputVisibleLightValues[4];

			Span<float> optimizedVisibleLightValues(reinterpret_cast<float*>(optimizedVisibleLightsBytes + (BYTES_PER_OPTIMIZED_LIGHT * i)), FLOATS_PER_OPTIMIZED_LIGHT);
			optimizedVisibleLightValues[0] = inputVisibleLightPointX;
			optimizedVisibleLightValues[1] = inputVisibleLightPointY;
			optimizedVisibleLightValues[2] = inputVisibleLightPointZ;
			optimizedVisibleLightValues[3] = inputVisibleLightStartRadius;
			optimizedVisibleLightValues[4] = inputVisibleLightStartRadius * inputVisibleLightStartRadius;
			optimizedVisibleLightValues[5] = inputVisibleLightEndRadius;
			optimizedVisibleLightValues[6] = inputVisibleLightEndRadius * inputVisibleLightEndRadius;
			optimizedVisibleLightValues[7] = 1.0 / (inputVisibleLightEndRadius - inputVisibleLightStartRadius);
		}
	}

	bool TryCreateDitherBuffers(Span<VulkanBuffer> ditherBuffers, vk::Device device, vk::Extent2D framebufferExtent, uint32_t queueFamilyIndex,
		VulkanHeapManager &deviceLocalHeapManager, VulkanHeapManager &stagingHeapManager, vk::CommandBuffer commandBuffer, VulkanPendingCommands &copyCommands)
	{
		for (int i = 0; i < ditherBuffers.getCount(); i++)
		{
			const DitheringMode ditheringMode = static_cast<DitheringMode>(i);
			const int ditherBufferPixelCount = framebufferExtent.width * framebufferExtent.height;

			int ditherBufferByteCount = sizeof(int); // Dummy value for None.
			if (ditheringMode == DitheringMode::Classic)
			{
				ditherBufferByteCount = ditherBufferPixelCount * sizeof(int);
			}
			else if (ditheringMode == DitheringMode::Modern)
			{
				ditherBufferByteCount = (ditherBufferPixelCount * DITHERING_MODERN_MASK_COUNT) * sizeof(int);
			}

			VulkanBuffer &ditherBuffer = ditherBuffers[i];
			if (!TryCreateBufferStagingAndDevice(device, ditherBuffer, ditherBufferByteCount, vk::BufferUsageFlagBits::eStorageBuffer, queueFamilyIndex, deviceLocalHeapManager, stagingHeapManager))
			{
				DebugLogErrorFormat("Couldn't create dither buffer for dithering mode %d.", ditheringMode);
				return false;
			}

			Buffer3D<bool> ditherBufferBools;
			RendererUtils::initDitherBuffer(ditherBufferBools, framebufferExtent.width, framebufferExtent.height, ditheringMode);

			int *ditherBufferValues = reinterpret_cast<int*>(ditherBuffer.stagingHostMappedBytes.begin());
			std::transform(ditherBufferBools.begin(), ditherBufferBools.end(), ditherBufferValues,
				[](bool isPixelDithered)
			{
				return static_cast<int>(isPixelDithered);
			});

			auto ditherBufferCopyCommand = [ditherBufferByteCount, &ditherBuffer, commandBuffer]()
			{
				CopyBufferToBuffer(ditherBuffer.stagingBuffer, ditherBuffer.deviceLocalBuffer, 0, ditherBufferByteCount, commandBuffer);
			};

			copyCommands.emplace_back(std::move(ditherBufferCopyCommand));
		}

		return true;
	}
}

VulkanBuffer::VulkanBuffer()
{
	this->type = static_cast<VulkanBufferType>(-1);
}

void VulkanBuffer::init(vk::Buffer deviceLocalBuffer, vk::Buffer stagingBuffer, Span<std::byte> stagingHostMappedBytes)
{
	this->deviceLocalBuffer = deviceLocalBuffer;
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

void VulkanBuffer::freeAllocations(vk::Device device)
{
	this->stagingHostMappedBytes.reset();

	if (this->stagingBuffer)
	{
		device.destroyBuffer(this->stagingBuffer);
		this->stagingBuffer = nullptr;
	}

	if (this->deviceLocalBuffer)
	{
		device.destroyBuffer(this->deviceLocalBuffer);
		this->deviceLocalBuffer = nullptr;
	}
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

void VulkanTexture::freeAllocations(vk::Device device)
{
	this->stagingHostMappedBytes.reset();

	if (this->stagingBuffer)
	{
		device.destroyBuffer(this->stagingBuffer);
		this->stagingBuffer = nullptr;
	}

	if (this->sampler)
	{
		device.destroySampler(this->sampler);
		this->sampler = nullptr;
	}

	if (this->imageView)
	{
		device.destroyImageView(this->imageView);
		this->imageView = nullptr;
	}

	if (this->image)
	{
		device.destroyImage(this->image);
		this->image = nullptr;
	}
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

bool VulkanRenderBackend::initContext(const RenderContextSettings &contextSettings)
{
	if (!TryCreateVulkanInstance(contextSettings.window->window, contextSettings.enableValidationLayers, &this->instance))
	{
		DebugLogError("Couldn't create Vulkan instance.");
		return false;
	}

	return true;
}

bool VulkanRenderBackend::initRendering(const RenderInitSettings &initSettings)
{
	const Window *window = initSettings.window;
	const std::string &dataFolderPath = initSettings.dataFolderPath;

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
	if (!TryGetSurfaceFormat(this->physicalDevice, this->surface, SwapchainImageFormat, SwapchainColorSpace, &surfaceFormat))
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

	this->swapchainImages = GetSwapchainImages(this->device, this->swapchain);

	if (!TryCreateSwapchainImageViews(this->device, this->swapchainImages, surfaceFormat, &this->swapchainImageViews))
	{
		DebugLogError("Couldn't create swapchain image views.");
		return false;
	}

	this->internalExtent = vk::Extent2D(initSettings.internalWidth, initSettings.internalHeight);

	for (int i = 0; i < VulkanRenderBackend::MAX_SCENE_FRAMEBUFFERS; i++)
	{
		const vk::MemoryAllocateInfo colorMemoryAllocateInfo = CreateImageMemoryAllocateInfo(this->device, this->internalExtent.width, this->internalExtent.height, ColorBufferFormat, ColorBufferUsageFlags, this->physicalDevice);
		if (!TryAllocateMemory(this->device, colorMemoryAllocateInfo, &this->colorDeviceMemories[i]))
		{
			DebugLogError("Couldn't allocate color buffer image memory.");
			return false;
		}

		if (!TryCreateImage(this->device, this->internalExtent.width, this->internalExtent.height, ColorBufferFormat, ColorBufferUsageFlags, this->graphicsQueueFamilyIndex, &this->colorImages[i]))
		{
			DebugLogError("Couldn't create color buffer image.");
			return false;
		}

		if (!TryBindImageToMemory(this->device, this->colorImages[i], this->colorDeviceMemories[i], 0))
		{
			DebugLogError("Couldn't bind color buffer image to memory.");
			return false;
		}

		if (!TryCreateImageView(this->device, ColorBufferFormat, vk::ImageAspectFlagBits::eColor, this->colorImages[i], &this->colorImageViews[i]))
		{
			DebugLogError("Couldn't create color buffer image view.");
			return false;
		}
	}

	if (!TryCreateSampler(this->device, &this->colorSampler))
	{
		DebugLogErrorFormat("Couldn't create color buffer sampler.");
		return false;
	}

	const vk::MemoryAllocateInfo depthMemoryAllocateInfo = CreateImageMemoryAllocateInfo(this->device, this->internalExtent.width, this->internalExtent.height, DepthBufferFormat, DepthBufferUsageFlags, this->physicalDevice);
	if (!TryAllocateMemory(this->device, depthMemoryAllocateInfo, &this->depthDeviceMemory))
	{
		DebugLogError("Couldn't allocate depth buffer image memory.");
		return false;
	}

	if (!TryCreateImage(this->device, this->internalExtent.width, this->internalExtent.height, DepthBufferFormat, DepthBufferUsageFlags, this->graphicsQueueFamilyIndex, &this->depthImage))
	{
		DebugLogError("Couldn't create depth buffer image.");
		return false;
	}

	if (!TryBindImageToMemory(this->device, this->depthImage, this->depthDeviceMemory, 0))
	{
		DebugLogError("Couldn't bind depth buffer image to memory.");
		return false;
	}

	if (!TryCreateImageView(this->device, DepthBufferFormat, vk::ImageAspectFlagBits::eDepth, this->depthImage, &this->depthImageView))
	{
		DebugLogError("Couldn't create depth buffer image view.");
		return false;
	}

	if (!TryCreateSceneRenderPass(this->device, &this->sceneRenderPass))
	{
		DebugLogError("Couldn't create scene render pass.");
		return false;
	}

	if (!TryCreateUiRenderPass(this->device, &this->uiRenderPass))
	{
		DebugLogError("Couldn't create UI render pass.");
		return false;
	}

	for (int i = 0; i < VulkanRenderBackend::MAX_SCENE_FRAMEBUFFERS; i++)
	{
		if (!TryCreateSceneFramebuffer(this->device, this->colorImageViews[i], this->depthImageView, this->internalExtent, this->sceneRenderPass, &this->sceneFramebuffers[i]))
		{
			DebugLogErrorFormat("Couldn't create scene framebuffer %d.", i);
			return false;
		}
	}

	this->uiFramebuffers.init(this->swapchainImageViews.getCount());
	for (int i = 0; i < this->swapchainImageViews.getCount(); i++)
	{
		if (!TryCreateUiFramebuffer(this->device, this->swapchainImageViews[i], this->swapchainExtent, this->uiRenderPass, &this->uiFramebuffers[i]))
		{
			DebugLogErrorFormat("Couldn't create UI framebuffer index %d.", i);
			return false;
		}
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

	const std::string lightBinningComputeShaderBytesFilename = shadersFolderPath + LightBinningComputeShaderFilename + ".spv";
	if (!TryCreateShaderModule(this->device, lightBinningComputeShaderBytesFilename.c_str(), &this->lightBinningComputeShader))
	{
		DebugLogErrorFormat("Couldn't create light binning compute shader module \"%s\".", lightBinningComputeShaderBytesFilename.c_str());
		return false;
	}

	const std::string conversionFragmentShaderBytesFilename = shadersFolderPath + ConversionFragmentShaderFilename + ".spv";
	if (!TryCreateShaderModule(this->device, conversionFragmentShaderBytesFilename.c_str(), &this->conversionShader))
	{
		DebugLogErrorFormat("Couldn't create conversion fragment shader module \"%s\".", conversionFragmentShaderBytesFilename.c_str());
		return false;
	}

	const vk::DescriptorPoolSize globalDescriptorPoolSizes[] =
	{
		CreateDescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MaxGlobalUniformBufferDescriptors),
		CreateDescriptorPoolSize(vk::DescriptorType::eStorageBuffer, MaxGlobalStorageBufferDescriptors),
		CreateDescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MaxGlobalImageDescriptors)
	};

	const vk::DescriptorPoolSize transformDescriptorPoolSizes[] =
	{
		CreateDescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, MaxTransformUniformBufferDynamicDescriptors)
	};

	const vk::DescriptorPoolSize materialDescriptorPoolSizes[] =
	{
		CreateDescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MaxMaterialImageDescriptors),
		CreateDescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MaxMaterialUniformBufferDescriptors)
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
		// Framebuffer dimensions
		CreateDescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment),
		// Ambient percent
		CreateDescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment),
		// Screen space animation
		CreateDescriptorSetLayoutBinding(3, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment),
		// Sampled framebuffer
		CreateDescriptorSetLayoutBinding(4, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
		// Palette
		CreateDescriptorSetLayoutBinding(5, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
		// Light table
		CreateDescriptorSetLayoutBinding(6, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
		// Sky texture (puddle fallback color)
		CreateDescriptorSetLayoutBinding(7, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
		// Horizon mirror point
		CreateDescriptorSetLayoutBinding(8, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment)
	};

	const vk::DescriptorSetLayoutBinding lightDescriptorSetLayoutBindings[] =
	{
		// Lights
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment),
		// Light index bins (depends on framebuffer size)
		CreateDescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment),
		// Light count per bin (depends on framebuffer size)
		CreateDescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment),
		// Dither buffer
		CreateDescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment),
		// Light bin dimensions
		CreateDescriptorSetLayoutBinding(4, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment)
	};

	const vk::DescriptorSetLayoutBinding transformDescriptorSetLayoutBindings[] =
	{
		// Mesh transform
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBufferDynamic, vk::ShaderStageFlagBits::eVertex)
	};

	const vk::DescriptorSetLayoutBinding materialDescriptorSetLayoutBindings[] =
	{
		// Mesh texture
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
		// Mesh texture
		CreateDescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
		// Lighting mode
		CreateDescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eFragment)
	};

	const vk::DescriptorSetLayoutBinding lightBinningDescriptorSetLayoutBindings[] =
	{
		// Camera
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute),
		// Framebuffer dimensions
		CreateDescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute),
		// Lights
		CreateDescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute),
		// Light index bins (depends on framebuffer size)
		CreateDescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute),
		// Light count per bin (depends on framebuffer size)
		CreateDescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute),
		// Light bin dimensions
		CreateDescriptorSetLayoutBinding(5, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eCompute)
	};

	const vk::DescriptorSetLayoutBinding conversionDescriptorSetLayoutBindings[] =
	{
		// Scene framebuffer
		CreateDescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
		// Palette
		CreateDescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
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

	if (!TryCreateDescriptorSetLayout(this->device, lightDescriptorSetLayoutBindings, &this->lightDescriptorSetLayout))
	{
		DebugLogError("Couldn't create light descriptor set layout.");
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

	if (!TryCreateDescriptorSetLayout(this->device, lightBinningDescriptorSetLayoutBindings, &this->lightBinningDescriptorSetLayout))
	{
		DebugLogError("Couldn't create light binning descriptor set layout.");
		return false;
	}

	if (!TryCreateDescriptorSetLayout(this->device, conversionDescriptorSetLayoutBindings, &this->conversionDescriptorSetLayout))
	{
		DebugLogError("Couldn't create conversion descriptor set layout.");
		return false;
	}

	if (!TryCreateDescriptorSetLayout(this->device, uiMaterialDescriptorSetLayoutBindings, &this->uiMaterialDescriptorSetLayout))
	{
		DebugLogError("Couldn't create UI material descriptor set layout.");
		return false;
	}

	for (int i = 0; i < VulkanRenderBackend::MAX_SCENE_FRAMEBUFFERS; i++)
	{
		if (!TryCreateDescriptorSet(this->device, this->globalDescriptorSetLayout, this->globalDescriptorPool, &this->globalDescriptorSets[i]))
		{
			DebugLogErrorFormat("Couldn't create global descriptor set %d.", i);
			return false;
		}
	}

	if (!TryCreateDescriptorSet(this->device, this->lightDescriptorSetLayout, this->globalDescriptorPool, &this->lightDescriptorSet))
	{
		DebugLogError("Couldn't create light descriptor set.");
		return false;
	}

	if (!TryCreateDescriptorSet(this->device, this->lightBinningDescriptorSetLayout, this->globalDescriptorPool, &this->lightBinningDescriptorSet))
	{
		DebugLogError("Couldn't create light binning descriptor set.");
		return false;
	}

	if (!TryCreateDescriptorSet(this->device, this->conversionDescriptorSetLayout, this->globalDescriptorPool, &this->conversionDescriptorSet))
	{
		DebugLogError("Couldn't create conversion descriptor set.");
		return false;
	}

	const vk::DescriptorSetLayout sceneDescriptorSetLayouts[] =
	{
		this->globalDescriptorSetLayout,
		this->lightDescriptorSetLayout,
		this->transformDescriptorSetLayout,
		this->materialDescriptorSetLayout
	};

	const vk::DescriptorSetLayout uiDescriptorSetLayouts[] =
	{
		this->conversionDescriptorSetLayout,
		this->uiMaterialDescriptorSetLayout
	};

	this->pipelineLayouts.init(static_cast<int>(std::size(RequiredPipelines)));
	this->graphicsPipelines.init(static_cast<int>(std::size(RequiredPipelines)));
	for (int i = 0; i < this->graphicsPipelines.getCount(); i++)
	{
		const VulkanPipelineKey requiredPipelineKey = RequiredPipelines[i];
		VulkanPipeline &pipeline = this->graphicsPipelines[i];

		const VertexShaderType vertexShaderType = requiredPipelineKey.vertexShaderType;
		const PixelShaderType fragmentShaderType = requiredPipelineKey.fragmentShaderType;

		int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
		vk::RenderPass renderPass = this->sceneRenderPass;
		Span<const vk::DescriptorSetLayout> descriptorSetLayouts = sceneDescriptorSetLayouts;
		if (fragmentShaderType == PixelShaderType::UiTexture)
		{
			positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX_2D;
			renderPass = this->uiRenderPass;
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

		pipeline.keyCode = MakePipelineKeyCode(vertexShaderType, fragmentShaderType, requiredPipelineKey.depthRead, requiredPipelineKey.depthWrite, requiredPipelineKey.backFaceCulling, requiredPipelineKey.alphaBlend);

		if (!TryCreateGraphicsPipeline(this->device, vertexShaderIter->module, fragmentShaderIter->module, positionComponentsPerVertex, requiredPipelineKey.depthRead,
			requiredPipelineKey.depthWrite, requiredPipelineKey.backFaceCulling, requiredPipelineKey.alphaBlend, pipelineLayout, renderPass, &pipeline.pipeline))
		{
			DebugLogErrorFormat("Couldn't create graphics pipeline %d.", i);
			return false;
		}
	}

	const vk::DescriptorSetLayout computeDescriptorSetLayouts[] =
	{
		this->lightBinningDescriptorSetLayout
	};

	if (!TryCreatePipelineLayout(this->device, computeDescriptorSetLayouts, Span<const vk::PushConstantRange>(), &this->lightBinningPipelineLayout))
	{
		DebugLogError("Couldn't create pipeline layout for compute pipeline.");
		return false;
	}

	if (!TryCreateComputePipeline(this->device, this->lightBinningComputeShader, this->lightBinningPipelineLayout, &this->lightBinningPipeline))
	{
		DebugLogError("Couldn't create compute pipeline for light binning.");
		return false;
	}

	const auto uiVertexShaderIter = std::find_if(this->vertexShaders.begin(), this->vertexShaders.end(),
		[](const VulkanVertexShader &shader)
	{
		return shader.type == VertexShaderType::UI;
	});

	DebugAssert(uiVertexShaderIter != this->vertexShaders.end());

	const vk::PipelineLayout uiPipelineLayout = this->pipelineLayouts[UiPipelineKeyIndex];
	if (!TryCreateGraphicsPipeline(this->device, uiVertexShaderIter->module, this->conversionShader, MeshUtils::POSITION_COMPONENTS_PER_VERTEX_2D,
		false, false, false, false, uiPipelineLayout, this->uiRenderPass, &this->conversionPipeline))
	{
		DebugLogErrorFormat("Couldn't create conversion graphics pipeline.");
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

	if (!this->storageBufferHeapManagerDeviceLocal.initBufferManager(this->device, BYTES_PER_HEAP_STORAGE_BUFFERS, StorageBufferDeviceLocalUsageFlags, false, this->physicalDevice))
	{
		DebugLogError("Couldn't create storage buffer device-local heap.");
		return false;
	}

	if (!this->storageBufferHeapManagerStaging.initBufferManager(this->device, BYTES_PER_HEAP_STORAGE_BUFFERS, StorageBufferStagingUsageFlags, true, this->physicalDevice))
	{
		DebugLogError("Couldn't create storage buffer staging heap.");
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

	auto tryCreateBufferStagingOnly = [this](VulkanBuffer &buffer, int byteCount, vk::BufferUsageFlags usageFlags)
	{
		VulkanHeapManager *heapManager = nullptr;
		if (usageFlags & vk::BufferUsageFlagBits::eUniformBuffer)
		{
			heapManager = &this->uniformBufferHeapManagerStaging;
		}
		else if (usageFlags & vk::BufferUsageFlagBits::eStorageBuffer)
		{
			heapManager = &this->storageBufferHeapManagerStaging;
		}

		vk::Buffer stagingBuffer;
		Span<std::byte> stagingHostMappedBytes;
		if (!TryCreateBufferAndBindWithHeap(this->device, byteCount, usageFlags, this->graphicsQueueFamilyIndex, *heapManager,
			&stagingBuffer, &stagingHostMappedBytes))
		{
			DebugLogError("Couldn't create buffer for host-coherent buffer.");
			return false;
		}

		buffer.init(nullptr, stagingBuffer, stagingHostMappedBytes);
		return true;
	};

	constexpr int cameraByteCount = sizeof(Matrix4f) + (sizeof(Float4) * 7); // View-projection, eye, forward + forwardScaled, right + rightScaled, up + upScaledRecip.
	if (!tryCreateBufferStagingOnly(this->camera, cameraByteCount, vk::BufferUsageFlagBits::eUniformBuffer))
	{
		DebugLogError("Couldn't create camera buffer.");
		return false;
	}

	constexpr int framebufferDimsByteCount = (sizeof(int) * 2) + (sizeof(float) * 2); // Width x height, widthReal x heightReal
	if (!tryCreateBufferStagingOnly(this->framebufferDims, framebufferDimsByteCount, vk::BufferUsageFlagBits::eUniformBuffer))
	{
		DebugLogError("Couldn't create framebuffer dimensions buffer.");
		return false;
	}

	constexpr int ambientLightByteCount = sizeof(float);
	if (!tryCreateBufferStagingOnly(this->ambientLight, ambientLightByteCount, vk::BufferUsageFlagBits::eUniformBuffer))
	{
		DebugLogError("Couldn't create ambient light buffer.");
		return false;
	}

	constexpr int screenSpaceAnimByteCount = sizeof(float); // Anim percent
	if (!tryCreateBufferStagingOnly(this->screenSpaceAnim, screenSpaceAnimByteCount, vk::BufferUsageFlagBits::eUniformBuffer))
	{
		DebugLogError("Couldn't create screen space animation buffer.");
		return false;
	}

	constexpr int horizonMirrorByteCount = sizeof(float) * 2; // Horizon screen space point.
	if (!tryCreateBufferStagingOnly(this->horizonMirror, horizonMirrorByteCount, vk::BufferUsageFlagBits::eUniformBuffer))
	{
		DebugLogError("Couldn't create horizon mirror buffer.");
		return false;
	}

	constexpr int optimizedVisibleLightsByteCount = (sizeof(float) * FLOATS_PER_OPTIMIZED_LIGHT) * MAX_LIGHTS_IN_FRUSTUM;
	if (!TryCreateBufferStagingAndDevice(this->device, this->optimizedVisibleLights, optimizedVisibleLightsByteCount, vk::BufferUsageFlagBits::eUniformBuffer,
		this->graphicsQueueFamilyIndex, this->uniformBufferHeapManagerDeviceLocal, this->uniformBufferHeapManagerStaging))
	{
		DebugLogError("Couldn't create optimized visible lights buffer.");
		return false;
	}

	const int lightBinWidth = GetLightBinWidth(initSettings.internalWidth);
	const int lightBinHeight = GetLightBinHeight(initSettings.internalHeight);
	const int lightBinCountX = GetLightBinCountX(initSettings.internalWidth, lightBinWidth);
	const int lightBinCountY = GetLightBinCountY(initSettings.internalHeight, lightBinHeight);
	const int lightBinCount = lightBinCountX * lightBinCountY;
	const int lightBinsByteCount = BYTES_PER_LIGHT_BIN * lightBinCount;
	if (!TryCreateBufferStagingAndDevice(this->device, this->lightBins, lightBinsByteCount, vk::BufferUsageFlagBits::eStorageBuffer,
		this->graphicsQueueFamilyIndex, this->storageBufferHeapManagerDeviceLocal, this->storageBufferHeapManagerStaging))
	{
		DebugLogError("Couldn't create light bins buffer.");
		return false;
	}

	const int lightBinLightCountsByteCount = BYTES_PER_LIGHT_BIN_LIGHT_COUNT * lightBinCount;
	if (!TryCreateBufferStagingAndDevice(this->device, this->lightBinLightCounts, lightBinLightCountsByteCount, vk::BufferUsageFlagBits::eStorageBuffer,
		this->graphicsQueueFamilyIndex, this->storageBufferHeapManagerDeviceLocal, this->storageBufferHeapManagerStaging))
	{
		DebugLogError("Couldn't create light bin light counts buffer.");
		return false;
	}

	constexpr int lightBinDimsByteCount = sizeof(int) * 6; // Bin width and height, bin count X and Y, visible light count, dither mode.
	if (!tryCreateBufferStagingOnly(this->lightBinDims, lightBinDimsByteCount, vk::BufferUsageFlagBits::eUniformBuffer))
	{
		DebugLogError("Couldn't create light bin dimensions buffer.");
		return false;
	}

	constexpr int lightModeByteCount = sizeof(int); // Bool must be 4 bytes for GLSL.
	if (!TryCreateBufferStagingAndDevice(this->device, this->perPixelLightMode, lightModeByteCount, vk::BufferUsageFlagBits::eUniformBuffer,
		this->graphicsQueueFamilyIndex, this->uniformBufferHeapManagerDeviceLocal, this->uniformBufferHeapManagerStaging))
	{
		DebugLogError("Couldn't create per-pixel light mode buffer.");
		return false;
	}

	if (!TryCreateBufferStagingAndDevice(this->device, this->perMeshLightMode, lightModeByteCount, vk::BufferUsageFlagBits::eUniformBuffer,
		this->graphicsQueueFamilyIndex, this->uniformBufferHeapManagerDeviceLocal, this->uniformBufferHeapManagerStaging))
	{
		DebugLogError("Couldn't create per-mesh light mode buffer.");
		return false;
	}

	auto lightingModeCopyCommand = [this, lightModeByteCount]()
	{
		int *perPixelLightModeValues = reinterpret_cast<int*>(this->perPixelLightMode.stagingHostMappedBytes.begin());
		perPixelLightModeValues[0] = 1;

		int *perMeshLightModeValues = reinterpret_cast<int*>(this->perMeshLightMode.stagingHostMappedBytes.begin());
		perMeshLightModeValues[0] = 0;

		CopyBufferToBuffer(this->perPixelLightMode.stagingBuffer, this->perPixelLightMode.deviceLocalBuffer, 0, lightModeByteCount, this->commandBuffer);
		CopyBufferToBuffer(this->perMeshLightMode.stagingBuffer, this->perMeshLightMode.deviceLocalBuffer, 0, lightModeByteCount, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(lightingModeCopyCommand));

	if (!TryCreateDitherBuffers(this->ditherBuffers, this->device, this->internalExtent, this->graphicsQueueFamilyIndex,
		this->storageBufferHeapManagerDeviceLocal, this->storageBufferHeapManagerStaging, this->commandBuffer, this->copyCommands))
	{
		DebugLogError("Couldn't create dither buffers.");
		return false;
	}

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

	constexpr int dummyImageWidth = 1;
	constexpr int dummyImageHeight = 1;
	if (!TryCreateImageAndBindWithHeap(this->device, dummyImageWidth, dummyImageHeight, ObjectTextureFormat8Bit, ObjectTextureDeviceLocalUsageFlags,
		this->graphicsQueueFamilyIndex, this->objectTextureHeapManagerDeviceLocal, &this->dummyImage))
	{
		DebugLogError("Couldn't create dummy image for object materials.");
		return false;
	}

	if (!TryCreateImageView(this->device, ObjectTextureFormat8Bit, vk::ImageAspectFlagBits::eColor, this->dummyImage, &this->dummyImageView))
	{
		DebugLogError("Couldn't create dummy image view for object materials.");
		return false;
	}

	return true;
}

void VulkanRenderBackend::shutdown()
{
	if (this->device)
	{
		if (this->dummyImageView)
		{
			this->device.destroyImageView(this->dummyImageView);
			this->dummyImageView = nullptr;
		}

		if (this->dummyImage)
		{
			this->device.destroyImage(this->dummyImage);
			this->dummyImage = nullptr;
		}

		this->uiVertexAttributeBufferID = -1;
		this->uiVertexPositionBufferID = -1;

		for (VulkanBuffer &buffer : this->ditherBuffers)
		{
			buffer.freeAllocations(this->device);
		}

		this->perMeshLightMode.freeAllocations(this->device);
		this->perPixelLightMode.freeAllocations(this->device);
		this->lightBinDims.freeAllocations(this->device);
		this->lightBinLightCounts.freeAllocations(this->device);
		this->lightBins.freeAllocations(this->device);
		this->optimizedVisibleLights.freeAllocations(this->device);
		this->horizonMirror.freeAllocations(this->device);
		this->screenSpaceAnim.freeAllocations(this->device);
		this->ambientLight.freeAllocations(this->device);
		this->framebufferDims.freeAllocations(this->device);
		this->camera.freeAllocations(this->device);

		this->uiTextureHeapManagerStaging.freeAllocations();
		this->uiTextureHeapManagerStaging.clear();

		this->uiTextureHeapManagerDeviceLocal.freeAllocations();
		this->uiTextureHeapManagerDeviceLocal.clear();

		this->objectTextureHeapManagerStaging.freeAllocations();
		this->objectTextureHeapManagerStaging.clear();

		this->objectTextureHeapManagerDeviceLocal.freeAllocations();
		this->objectTextureHeapManagerDeviceLocal.clear();

		this->storageBufferHeapManagerStaging.freeAllocations();
		this->storageBufferHeapManagerStaging.clear();

		this->storageBufferHeapManagerDeviceLocal.freeAllocations();
		this->storageBufferHeapManagerDeviceLocal.clear();

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
			texture.freeAllocations(this->device);
		}

		this->uiTexturePool.clear();

		for (VulkanTexture &texture : this->objectTexturePool.values)
		{
			texture.freeAllocations(this->device);
		}

		this->objectTexturePool.clear();

		for (VulkanBuffer &buffer : this->uniformBufferPool.values)
		{
			buffer.freeAllocations(this->device);
		}

		this->uniformBufferPool.clear();

		for (VulkanBuffer &buffer : this->indexBufferPool.values)
		{
			buffer.freeAllocations(this->device);
		}

		this->indexBufferPool.clear();

		for (VulkanBuffer &buffer : this->vertexAttributeBufferPool.values)
		{
			buffer.freeAllocations(this->device);
		}

		this->vertexAttributeBufferPool.clear();

		for (VulkanBuffer &buffer : this->vertexPositionBufferPool.values)
		{
			buffer.freeAllocations(this->device);
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

		if (this->conversionPipeline)
		{
			this->device.destroyPipeline(this->conversionPipeline);
			this->conversionPipeline = nullptr;
		}

		if (this->lightBinningPipeline)
		{
			this->device.destroyPipeline(this->lightBinningPipeline);
			this->lightBinningPipeline = nullptr;
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

		if (this->lightBinningPipelineLayout)
		{
			this->device.destroyPipelineLayout(this->lightBinningPipelineLayout);
			this->lightBinningPipelineLayout = nullptr;
		}

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

		if (this->conversionDescriptorSetLayout)
		{
			this->device.destroyDescriptorSetLayout(this->conversionDescriptorSetLayout);
			this->conversionDescriptorSetLayout = nullptr;
		}

		if (this->lightBinningDescriptorSetLayout)
		{
			this->device.destroyDescriptorSetLayout(this->lightBinningDescriptorSetLayout);
			this->lightBinningDescriptorSetLayout = nullptr;
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

		if (this->lightDescriptorSetLayout)
		{
			this->device.destroyDescriptorSetLayout(this->lightDescriptorSetLayout);
			this->lightDescriptorSetLayout = nullptr;
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
			this->conversionDescriptorSet = nullptr;
			this->lightBinningDescriptorSet = nullptr;
			this->lightDescriptorSet = nullptr;

			for (vk::DescriptorSet &descriptorSet : this->globalDescriptorSets)
			{
				descriptorSet = nullptr;
			}

			this->device.destroyDescriptorPool(this->globalDescriptorPool);
			this->globalDescriptorPool = nullptr;
		}

		if (this->conversionShader)
		{
			this->device.destroyShaderModule(this->conversionShader);
			this->conversionShader = nullptr;
		}

		if (this->lightBinningComputeShader)
		{
			this->device.destroyShaderModule(this->lightBinningComputeShader);
			this->lightBinningComputeShader = nullptr;
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

		for (vk::Framebuffer framebuffer : this->uiFramebuffers)
		{
			if (framebuffer)
			{
				this->device.destroyFramebuffer(framebuffer);
			}
		}

		this->uiFramebuffers.clear();

		for (vk::Framebuffer &framebuffer : this->sceneFramebuffers)
		{
			if (framebuffer)
			{
				this->device.destroyFramebuffer(framebuffer);
				framebuffer = nullptr;
			}
		}

		if (this->uiRenderPass)
		{
			this->device.destroyRenderPass(this->uiRenderPass);
			this->uiRenderPass = nullptr;
		}

		if (this->sceneRenderPass)
		{
			this->device.destroyRenderPass(this->sceneRenderPass);
			this->sceneRenderPass = nullptr;
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

		if (this->colorSampler)
		{
			this->device.destroySampler(this->colorSampler);
			this->colorSampler = nullptr;
		}

		for (vk::ImageView &imageView : this->colorImageViews)
		{
			if (imageView)
			{
				this->device.destroyImageView(imageView);
				imageView = nullptr;
			}
		}

		for (vk::Image &image : this->colorImages)
		{
			if (image)
			{
				this->device.destroyImage(image);
				image = nullptr;
			}
		}

		for (vk::DeviceMemory &deviceMemory : this->colorDeviceMemories)
		{
			if (deviceMemory)
			{
				this->device.freeMemory(deviceMemory);
				deviceMemory = nullptr;
			}
		}

		for (vk::ImageView imageView : this->swapchainImageViews)
		{
			if (imageView)
			{
				this->device.destroyImageView(imageView);
			}
		}

		this->swapchainImageViews.clear();
		this->swapchainImages.clear();

		if (this->swapchain)
		{
			this->device.destroySwapchainKHR(this->swapchain);
			this->swapchain = nullptr;
		}

		this->internalExtent = vk::Extent2D();
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
	for (VulkanBuffer &buffer : this->ditherBuffers)
	{
		if (buffer.deviceLocalBuffer)
		{
			this->storageBufferHeapManagerDeviceLocal.freeBufferMapping(buffer.deviceLocalBuffer);
		}
		
		if (buffer.stagingBuffer)
		{
			this->storageBufferHeapManagerStaging.freeBufferMapping(buffer.stagingBuffer);
		}
		
		buffer.freeAllocations(this->device);
	}

	if (this->lightBins.deviceLocalBuffer)
	{
		this->storageBufferHeapManagerDeviceLocal.freeBufferMapping(this->lightBins.deviceLocalBuffer);
	}
	
	if (this->lightBins.stagingBuffer)
	{
		this->storageBufferHeapManagerStaging.freeBufferMapping(this->lightBins.stagingBuffer);
	}
	
	this->lightBins.freeAllocations(this->device);

	if (this->lightBinLightCounts.deviceLocalBuffer)
	{
		this->storageBufferHeapManagerDeviceLocal.freeBufferMapping(this->lightBinLightCounts.deviceLocalBuffer);
	}
	
	if (this->lightBinLightCounts.stagingBuffer)
	{
		this->storageBufferHeapManagerStaging.freeBufferMapping(this->lightBinLightCounts.stagingBuffer);
	}
	
	this->lightBinLightCounts.freeAllocations(this->device);

	for (vk::Framebuffer framebuffer : this->uiFramebuffers)
	{
		if (framebuffer)
		{
			this->device.destroyFramebuffer(framebuffer);
		}
	}

	this->uiFramebuffers.clear();

	for (vk::Framebuffer &framebuffer : this->sceneFramebuffers)
	{
		if (framebuffer)
		{
			this->device.destroyFramebuffer(framebuffer);
			framebuffer = nullptr;
		}
	}

	if (this->uiRenderPass)
	{
		this->device.destroyRenderPass(this->uiRenderPass);
		this->uiRenderPass = nullptr;
	}

	if (this->sceneRenderPass)
	{
		this->device.destroyRenderPass(this->sceneRenderPass);
		this->sceneRenderPass = nullptr;
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

	if (this->colorSampler)
	{
		this->device.destroySampler(this->colorSampler);
		this->colorSampler = nullptr;
	}

	for (vk::ImageView &imageView : this->colorImageViews)
	{
		if (imageView)
		{
			this->device.destroyImageView(imageView);
			imageView = nullptr;
		}
	}

	for (vk::Image &image : this->colorImages)
	{
		if (image)
		{
			this->device.destroyImage(image);
			image = nullptr;
		}
	}

	for (vk::DeviceMemory &deviceMemory : this->colorDeviceMemories)
	{
		if (deviceMemory)
		{
			this->device.freeMemory(deviceMemory);
			deviceMemory = nullptr;
		}
	}

	for (vk::ImageView imageView : this->swapchainImageViews)
	{
		this->device.destroyImageView(imageView);
	}

	this->swapchainImageViews.clear();
	this->swapchainImages.clear();

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
	this->internalExtent = vk::Extent2D(internalWidth, internalHeight);

	vk::SurfaceFormatKHR surfaceFormat;
	if (!TryGetSurfaceFormat(this->physicalDevice, this->surface, SwapchainImageFormat, SwapchainColorSpace, &surfaceFormat))
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

	this->swapchainImages = GetSwapchainImages(this->device, this->swapchain);

	if (!TryCreateSwapchainImageViews(this->device, this->swapchainImages, surfaceFormat, &this->swapchainImageViews))
	{
		DebugLogErrorFormat("Couldn't create swapchain image views for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	for (int i = 0; i < VulkanRenderBackend::MAX_SCENE_FRAMEBUFFERS; i++)
	{
		const vk::MemoryAllocateInfo colorMemoryAllocateInfo = CreateImageMemoryAllocateInfo(this->device, this->internalExtent.width, this->internalExtent.height, ColorBufferFormat, ColorBufferUsageFlags, this->physicalDevice);
		if (!TryAllocateMemory(this->device, colorMemoryAllocateInfo, &this->colorDeviceMemories[i]))
		{
			DebugLogErrorFormat("Couldn't allocate color buffer image memory for resize to %dx%d.", windowWidth, windowHeight);
			return;
		}

		if (!TryCreateImage(this->device, this->internalExtent.width, this->internalExtent.height, ColorBufferFormat, ColorBufferUsageFlags, this->graphicsQueueFamilyIndex, &this->colorImages[i]))
		{
			DebugLogErrorFormat("Couldn't create color buffer image for resize to %dx%d.", windowWidth, windowHeight);
			return;
		}

		if (!TryBindImageToMemory(this->device, this->colorImages[i], this->colorDeviceMemories[i], 0))
		{
			DebugLogErrorFormat("Couldn't bind color buffer image to memory for resize to %dx%d.", windowWidth, windowHeight);
			return;
		}

		if (!TryCreateImageView(this->device, ColorBufferFormat, vk::ImageAspectFlagBits::eColor, this->colorImages[i], &this->colorImageViews[i]))
		{
			DebugLogErrorFormat("Couldn't create color buffer image view for resize to %dx%d.", windowWidth, windowHeight);
			return;
		}
	}

	if (!TryCreateSampler(this->device, &this->colorSampler))
	{
		DebugLogErrorFormat("Couldn't create color buffer sampler for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	const vk::MemoryAllocateInfo depthMemoryAllocateInfo = CreateImageMemoryAllocateInfo(this->device, this->internalExtent.width, this->internalExtent.height, DepthBufferFormat, DepthBufferUsageFlags, this->physicalDevice);
	if (!TryAllocateMemory(this->device, depthMemoryAllocateInfo, &this->depthDeviceMemory))
	{
		DebugLogErrorFormat("Couldn't allocate depth buffer image memory for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateImage(this->device, this->internalExtent.width, this->internalExtent.height, DepthBufferFormat, DepthBufferUsageFlags, this->graphicsQueueFamilyIndex, &this->depthImage))
	{
		DebugLogErrorFormat("Couldn't create depth buffer image for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryBindImageToMemory(this->device, this->depthImage, this->depthDeviceMemory, 0))
	{
		DebugLogErrorFormat("Couldn't bind depth buffer image to memory for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateImageView(this->device, DepthBufferFormat, vk::ImageAspectFlagBits::eDepth, this->depthImage, &this->depthImageView))
	{
		DebugLogErrorFormat("Couldn't create depth buffer image view for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateSceneRenderPass(this->device, &this->sceneRenderPass))
	{
		DebugLogErrorFormat("Couldn't create scene render pass for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateUiRenderPass(this->device, &this->uiRenderPass))
	{
		DebugLogErrorFormat("Couldn't create UI render pass for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	for (int i = 0; i < VulkanRenderBackend::MAX_SCENE_FRAMEBUFFERS; i++)
	{
		if (!TryCreateSceneFramebuffer(this->device, this->colorImageViews[i], this->depthImageView, this->internalExtent, this->sceneRenderPass, &this->sceneFramebuffers[i]))
		{
			DebugLogErrorFormat("Couldn't create framebuffer %d for resize to %dx%d.", i, windowWidth, windowHeight);
			return;
		}
	}

	this->uiFramebuffers.init(this->swapchainImageViews.getCount());
	for (int i = 0; i < this->swapchainImageViews.getCount(); i++)
	{
		if (!TryCreateUiFramebuffer(this->device, this->swapchainImageViews[i], this->swapchainExtent, this->uiRenderPass, &this->uiFramebuffers[i]))
		{
			DebugLogErrorFormat("Couldn't create UI framebuffer index %d for resize to %dx%d.", i, windowWidth, windowHeight);
			return;
		}
	}

	const int lightBinWidth = GetLightBinWidth(internalWidth);
	const int lightBinHeight = GetLightBinHeight(internalHeight);
	const int lightBinCountX = GetLightBinCountX(internalWidth, lightBinWidth);
	const int lightBinCountY = GetLightBinCountY(internalHeight, lightBinHeight);
	const int lightBinCount = lightBinCountX * lightBinCountY;
	const int lightBinsByteCount = BYTES_PER_LIGHT_BIN * lightBinCount;
	if (!TryCreateBufferStagingAndDevice(this->device, this->lightBins, lightBinsByteCount, vk::BufferUsageFlagBits::eStorageBuffer,
		this->graphicsQueueFamilyIndex, this->storageBufferHeapManagerDeviceLocal, this->storageBufferHeapManagerStaging))
	{
		DebugLogErrorFormat("Couldn't create light bins buffer for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	const int lightBinLightCountsByteCount = BYTES_PER_LIGHT_BIN_LIGHT_COUNT * lightBinCount;
	if (!TryCreateBufferStagingAndDevice(this->device, this->lightBinLightCounts, lightBinLightCountsByteCount, vk::BufferUsageFlagBits::eStorageBuffer,
		this->graphicsQueueFamilyIndex, this->storageBufferHeapManagerDeviceLocal, this->storageBufferHeapManagerStaging))
	{
		DebugLogErrorFormat("Couldn't create light bin light counts buffer for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}

	if (!TryCreateDitherBuffers(this->ditherBuffers, this->device, this->internalExtent, this->graphicsQueueFamilyIndex,
		this->storageBufferHeapManagerDeviceLocal, this->storageBufferHeapManagerStaging, this->commandBuffer, this->copyCommands))
	{
		DebugLogErrorFormat("Couldn't create dither buffers for resize to %dx%d.", windowWidth, windowHeight);
		return;
	}
}

void VulkanRenderBackend::handleRenderTargetsReset(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight)
{
	DebugNotImplementedMsg("handleRenderTargetsReset()");
}

RendererProfilerData2D VulkanRenderBackend::getProfilerData2D() const
{
	RendererProfilerData2D profilerData;
	profilerData.drawCallCount = 0;
	profilerData.uiTextureCount = static_cast<int>(this->uiTexturePool.values.size());
	for (const VulkanTexture &texture : this->uiTexturePool.values)
	{
		// Don't worry about staging buffers, we mostly care about in VRAM for profiling.
		const int estimatedDeviceLocalByteCount = texture.width * texture.height * texture.bytesPerTexel;
		profilerData.uiTextureByteCount += estimatedDeviceLocalByteCount;
	}

	return profilerData;
}

RendererProfilerData3D VulkanRenderBackend::getProfilerData3D() const
{
	// @todo maybe revise this listing of data to better suit a general render backend
	// - # of vertex buffers... index buffers... object textures... ui textures... materials...

	RendererProfilerData3D profilerData;
	profilerData.width = this->internalExtent.width;
	profilerData.height = this->internalExtent.height;
	profilerData.threadCount = 1;
	profilerData.drawCallCount = 0;
	profilerData.presentedTriangleCount = 0;
	profilerData.objectTextureCount = static_cast<int>(this->objectTexturePool.values.size());
	for (const VulkanTexture &texture : this->objectTexturePool.values)
	{
		profilerData.objectTextureByteCount += texture.width * texture.height * texture.bytesPerTexel;
	}

	profilerData.materialCount = static_cast<int>(this->materialPool.values.size());
	profilerData.totalLightCount = 0;
	profilerData.totalCoverageTests = 0;
	profilerData.totalDepthTests = 0;
	profilerData.totalColorWrites = 0;

	return profilerData;
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
	const int byteCount = elementCount * bytesPerComponent;

	vk::Buffer deviceLocalBuffer;
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBuffersAndBindWithHeaps(this->device, byteCount, VertexBufferDeviceLocalUsageFlags, VertexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex,
		this->vertexBufferHeapManagerDeviceLocal, this->vertexBufferHeapManagerStaging, &deviceLocalBuffer, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create vertex position buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexPositionBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(id);
	vertexPositionBuffer.init(deviceLocalBuffer, stagingBuffer, stagingHostMappedBytes);
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

			if (vertexPositionBuffer->deviceLocalBuffer)
			{
				this->vertexBufferHeapManagerDeviceLocal.freeBufferMapping(vertexPositionBuffer->deviceLocalBuffer);
				this->device.destroyBuffer(vertexPositionBuffer->deviceLocalBuffer);
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
		vk::Buffer deviceLocalBuffer = vertexPositionBuffer.deviceLocalBuffer;
		vk::Buffer stagingBuffer = vertexPositionBuffer.stagingBuffer;
		const int byteCount = vertexPositionBuffer.stagingHostMappedBytes.getCount();
		CopyBufferToBuffer(stagingBuffer, deviceLocalBuffer, 0, byteCount, this->commandBuffer);
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
	const int byteCount = elementCount * bytesPerComponent;

	vk::Buffer deviceLocalBuffer;
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBuffersAndBindWithHeaps(this->device, byteCount, VertexBufferDeviceLocalUsageFlags, VertexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex,
		this->vertexBufferHeapManagerDeviceLocal, this->vertexBufferHeapManagerStaging, &deviceLocalBuffer, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create vertex attribute buffer (vertices: %d, components: %d).", vertexCount, componentsPerVertex);
		this->vertexAttributeBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &vertexAttributeBuffer = this->vertexAttributeBufferPool.get(id);
	vertexAttributeBuffer.init(deviceLocalBuffer, stagingBuffer, stagingHostMappedBytes);
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

			if (vertexAttributeBuffer->deviceLocalBuffer)
			{
				this->vertexBufferHeapManagerDeviceLocal.freeBufferMapping(vertexAttributeBuffer->deviceLocalBuffer);
				this->device.destroyBuffer(vertexAttributeBuffer->deviceLocalBuffer);
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
		vk::Buffer deviceLocalBuffer = vertexAttributeBuffer.deviceLocalBuffer;
		vk::Buffer stagingBuffer = vertexAttributeBuffer.stagingBuffer;
		const int byteCount = vertexAttributeBuffer.stagingHostMappedBytes.getCount();
		CopyBufferToBuffer(stagingBuffer, deviceLocalBuffer, 0, byteCount, this->commandBuffer);
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

	const int byteCount = indexCount * bytesPerIndex;

	vk::Buffer deviceLocalBuffer;
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBuffersAndBindWithHeaps(this->device, byteCount, IndexBufferDeviceLocalUsageFlags, IndexBufferStagingUsageFlags, this->graphicsQueueFamilyIndex,
		this->indexBufferHeapManagerDeviceLocal, this->indexBufferHeapManagerStaging, &deviceLocalBuffer, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create index buffer (indices: %d).", indexCount);
		this->indexBufferPool.free(id);
		return -1;
	}

	VulkanBuffer &indexBuffer = this->indexBufferPool.get(id);
	indexBuffer.init(deviceLocalBuffer, stagingBuffer, stagingHostMappedBytes);
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

			if (indexBuffer->deviceLocalBuffer)
			{
				this->indexBufferHeapManagerDeviceLocal.freeBufferMapping(indexBuffer->deviceLocalBuffer);
				this->device.destroyBuffer(indexBuffer->deviceLocalBuffer);
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
		vk::Buffer deviceLocalBuffer = indexBuffer.deviceLocalBuffer;
		vk::Buffer stagingBuffer = indexBuffer.stagingBuffer;
		const int byteCount = indexBuffer.stagingHostMappedBytes.getCount();
		CopyBufferToBuffer(stagingBuffer, deviceLocalBuffer, 0, byteCount, this->commandBuffer);
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

	vk::Buffer deviceLocalBuffer;
	vk::Buffer stagingBuffer;
	Span<std::byte> stagingHostMappedBytes;
	if (!TryCreateBuffersAndBindWithHeaps(this->device, byteCountWithAlignedElements, UniformBufferDeviceLocalUsageFlags, UniformBufferStagingUsageFlags,
		this->graphicsQueueFamilyIndex, this->uniformBufferHeapManagerDeviceLocal, this->uniformBufferHeapManagerStaging, &deviceLocalBuffer, &stagingBuffer, &stagingHostMappedBytes))
	{
		DebugLogErrorFormat("Couldn't create uniform buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, bytesPerElement, alignmentOfElement);
		this->uniformBufferPool.free(id);
		return -1;
	}

	vk::DescriptorSet descriptorSet;
	if (!TryCreateDescriptorSet(this->device, this->transformDescriptorSetLayout, this->transformDescriptorPool, &descriptorSet))
	{
		DebugLogErrorFormat("Couldn't create descriptor set for uniform buffer (elements: %d, sizeof: %d, alignment: %d).", elementCount, bytesPerElement, alignmentOfElement);
		this->uniformBufferHeapManagerStaging.freeBufferMapping(stagingBuffer);
		this->device.destroyBuffer(stagingBuffer);
		this->uniformBufferHeapManagerDeviceLocal.freeBufferMapping(deviceLocalBuffer);
		this->device.destroyBuffer(deviceLocalBuffer);
		this->uniformBufferPool.free(id);
		return -1;
	}

	UpdateTransformDescriptorSet(this->device, descriptorSet, deviceLocalBuffer, bytesPerStride);

	VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
	uniformBuffer.init(deviceLocalBuffer, stagingBuffer, stagingHostMappedBytes);
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

			if (uniformBuffer->deviceLocalBuffer)
			{
				this->uniformBufferHeapManagerDeviceLocal.freeBufferMapping(uniformBuffer->deviceLocalBuffer);
				this->device.destroyBuffer(uniformBuffer->deviceLocalBuffer);
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
		vk::Buffer deviceLocalBuffer = uniformBuffer.deviceLocalBuffer;
		vk::Buffer stagingBuffer = uniformBuffer.stagingBuffer;
		const int byteCount = uniformBuffer.stagingHostMappedBytes.getCount();
		CopyBufferToBuffer(stagingBuffer, deviceLocalBuffer, 0, byteCount, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
}

void VulkanRenderBackend::unlockUniformBufferIndex(UniformBufferID id, int index)
{
	auto commandBufferFunc = [this, id, index]()
	{
		const VulkanBuffer &uniformBuffer = this->uniformBufferPool.get(id);
		vk::Buffer deviceLocalBuffer = uniformBuffer.deviceLocalBuffer;
		vk::Buffer stagingBuffer = uniformBuffer.stagingBuffer;
		const VulkanBufferUniformInfo &uniformInfo = uniformBuffer.uniform;
		const int byteOffset = index * uniformInfo.bytesPerElement;
		const int byteCount = uniformInfo.bytesPerElement;
		CopyBufferToBuffer(stagingBuffer, deviceLocalBuffer, byteOffset, byteCount, this->commandBuffer);
	};

	this->copyCommands.emplace_back(std::move(commandBufferFunc));
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

		ApplyColorImageLayoutTransition(
			image,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			vk::AccessFlagBits::eNone,
			vk::AccessFlagBits::eTransferWrite,
			this->commandBuffer);

		CopyBufferToImage(stagingBuffer, image, width, height, this->commandBuffer);

		ApplyColorImageLayoutTransition(
			image,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eShaderRead,
			this->commandBuffer);
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

	UpdateUiMaterialDescriptorSet(this->device, descriptorSet, imageView, sampler);
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

		ApplyColorImageLayoutTransition(
			image,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			vk::AccessFlagBits::eNone,
			vk::AccessFlagBits::eTransferWrite,
			this->commandBuffer);

		CopyBufferToImage(stagingBuffer, image, width, height, this->commandBuffer);

		ApplyColorImageLayoutTransition(
			image,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eShaderRead,
			this->commandBuffer);
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
	const VulkanPipeline &pipeline = this->graphicsPipelines[pipelineIndex];

	vk::DescriptorSet descriptorSet;
	if (!TryCreateDescriptorSet(this->device, this->materialDescriptorSetLayout, this->materialDescriptorPool, &descriptorSet))
	{
		DebugLogErrorFormat("Couldn't create descriptor set for material key (vertex shader %d, fragment shader %d, depth read %d, depth write %d, back-face culling %d).",
			key.vertexShaderType, key.pixelShaderType, key.enableDepthRead, key.enableDepthWrite, key.enableBackFaceCulling);
		return -1;
	}

	const ObjectTextureID textureID0 = key.textureIDs[0];
	const VulkanTexture &texture0 = this->objectTexturePool.get(textureID0);
	const vk::ImageView texture0ImageView = texture0.imageView;

	vk::ImageView texture1ImageView = this->dummyImageView;
	if (key.textureCount == 2)
	{
		const ObjectTextureID textureID1 = key.textureIDs[1];
		const VulkanTexture &texture1 = this->objectTexturePool.get(textureID1);
		texture1ImageView = texture1.imageView;
	}

	vk::Buffer lightingModeBuffer = this->perPixelLightMode.deviceLocalBuffer;
	if (key.lightingType == RenderLightingType::PerMesh)
	{
		lightingModeBuffer = this->perMeshLightMode.deviceLocalBuffer;
	}

	UpdateMaterialDescriptorSet(this->device, descriptorSet, texture0ImageView, texture1ImageView, texture0.sampler, lightingModeBuffer);

	VulkanMaterial &material = this->materialPool.get(materialID);
	material.init(pipeline.pipeline, pipelineLayout, descriptorSet);

	int typeIndex = 0;
	if (RenderShaderUtils::requiresMeshLightPercent(fragmentShaderType))
	{
		material.pushConstantTypes[typeIndex] = VulkanMaterialPushConstantType::MeshLightPercent;
		typeIndex++;
	}

	if (RenderShaderUtils::requiresPixelShaderParam(fragmentShaderType))
	{
		material.pushConstantTypes[typeIndex] = VulkanMaterialPushConstantType::PixelShaderParam;
		typeIndex++;
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
	const vk::Image acquiredSwapchainImage = this->swapchainImages[acquiredSwapchainImageIndex];

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

	const VulkanTexture *paletteTexture = nullptr;
	const int lightBinWidth = GetLightBinWidth(this->internalExtent.width);
	const int lightBinHeight = GetLightBinHeight(this->internalExtent.height);
	const int lightBinCountX = GetLightBinCountX(this->internalExtent.width, lightBinWidth);
	const int lightBinCountY = GetLightBinCountY(this->internalExtent.height, lightBinHeight);

	const bool anySceneDrawCalls = renderCommandList.entryCount > 0;
	if (anySceneDrawCalls)
	{
		auto double3ToFloat4 = [](const Double3 xyz, double w)
		{
			return Float4(static_cast<float>(xyz.x), static_cast<float>(xyz.y), static_cast<float>(xyz.z), static_cast<float>(w));
		};

		// Update global shader values.
		Matrix4d projectionMatrix = camera.projectionMatrix;
		projectionMatrix.y.y = -projectionMatrix.y.y; // Flip Y so world is not upside down.
		const Matrix4f viewProjection = RendererUtils::matrix4DoubleToFloat(projectionMatrix * camera.viewMatrix);
		const Float4 cameraPoint = double3ToFloat4(camera.worldPoint, 1.0);
		const Float4 cameraForward = double3ToFloat4(camera.forward, 0.0);
		const Float4 cameraForwardScaled = double3ToFloat4(camera.forwardScaled, 0.0);
		const Float4 cameraRight = double3ToFloat4(camera.right, 0.0);
		const Float4 cameraRightScaled = double3ToFloat4(camera.rightScaled, 0.0);
		const Float4 cameraUp = double3ToFloat4(camera.up, 0.0);
		const Float4 cameraUpScaledRecip = double3ToFloat4(camera.upScaledRecip, 0.0);

		float *cameraValues = reinterpret_cast<float*>(this->camera.stagingHostMappedBytes.begin());
		std::memcpy(cameraValues, &viewProjection.x, sizeof(Float4));
		std::memcpy(cameraValues + 4, &viewProjection.y, sizeof(Float4));
		std::memcpy(cameraValues + 8, &viewProjection.z, sizeof(Float4));
		std::memcpy(cameraValues + 12, &viewProjection.w, sizeof(Float4));
		std::memcpy(cameraValues + 16, &cameraPoint, sizeof(Float4));
		std::memcpy(cameraValues + 20, &cameraForward, sizeof(Float4));
		std::memcpy(cameraValues + 24, &cameraForwardScaled, sizeof(Float4));
		std::memcpy(cameraValues + 28, &cameraRight, sizeof(Float4));
		std::memcpy(cameraValues + 32, &cameraRightScaled, sizeof(Float4));
		std::memcpy(cameraValues + 36, &cameraUp, sizeof(Float4));
		std::memcpy(cameraValues + 40, &cameraUpScaledRecip, sizeof(Float4));

		int *framebufferDimsValues = reinterpret_cast<int*>(this->framebufferDims.stagingHostMappedBytes.begin());
		framebufferDimsValues[0] = this->internalExtent.width;
		framebufferDimsValues[1] = this->internalExtent.height;

		float *framebufferDimsRealValues = reinterpret_cast<float*>(framebufferDimsValues + 2);
		framebufferDimsRealValues[0] = static_cast<float>(this->internalExtent.width);
		framebufferDimsRealValues[1] = static_cast<float>(this->internalExtent.height);

		float *ambientLightValues = reinterpret_cast<float*>(this->ambientLight.stagingHostMappedBytes.begin());
		ambientLightValues[0] = static_cast<float>(frameSettings.ambientPercent);

		float *screenSpaceAnimValues = reinterpret_cast<float*>(this->screenSpaceAnim.stagingHostMappedBytes.begin());
		screenSpaceAnimValues[0] = static_cast<float>(frameSettings.screenSpaceAnimPercent);

		paletteTexture = &this->objectTexturePool.get(frameSettings.paletteTextureID);
		const VulkanTexture &lightTableTexture = this->objectTexturePool.get(frameSettings.lightTableTextureID);
		const VulkanTexture &skyBgTexture = this->objectTexturePool.get(frameSettings.skyBgTextureID);

		const Double2 horizonScreenSpacePoint = RendererUtils::ndcToScreenSpace(camera.horizonNdcPoint, this->internalExtent.width, this->internalExtent.height);
		float *horizonMirrorValues = reinterpret_cast<float*>(this->horizonMirror.stagingHostMappedBytes.begin());
		horizonMirrorValues[0] = static_cast<float>(horizonScreenSpacePoint.x);
		horizonMirrorValues[1] = static_cast<float>(horizonScreenSpacePoint.y);

		for (int i = 0; i < VulkanRenderBackend::MAX_SCENE_FRAMEBUFFERS; i++)
		{
			UpdateGlobalDescriptorSet(this->device, this->globalDescriptorSets[i], this->camera.stagingBuffer, this->framebufferDims.stagingBuffer, this->ambientLight.stagingBuffer,
				this->screenSpaceAnim.stagingBuffer, this->colorImageViews[i], this->colorSampler, paletteTexture->imageView, paletteTexture->sampler, lightTableTexture.imageView,
				lightTableTexture.sampler, skyBgTexture.imageView, skyBgTexture.sampler, this->horizonMirror.stagingBuffer);
		}

		// Update visible lights.
		const int clampedVisibleLightCount = std::min(frameSettings.visibleLightCount, MAX_LIGHTS_IN_FRUSTUM);
		const VulkanBuffer &inputVisibleLightsBuffer = this->uniformBufferPool.get(frameSettings.visibleLightsBufferID);
		PopulateLightGlobals(inputVisibleLightsBuffer, clampedVisibleLightCount, camera, this->internalExtent.width, this->internalExtent.height,
			this->optimizedVisibleLights, this->lightBins, this->lightBinLightCounts);

		const int ditherBufferIndex = static_cast<int>(frameSettings.ditheringMode);
		DebugAssertIndex(this->ditherBuffers, ditherBufferIndex);
		const VulkanBuffer &ditherBuffer = this->ditherBuffers[ditherBufferIndex];
		UpdateLightDescriptorSet(this->device, this->lightDescriptorSet, this->optimizedVisibleLights.deviceLocalBuffer, this->lightBins.deviceLocalBuffer,
			this->lightBinLightCounts.deviceLocalBuffer, this->lightBinDims.stagingBuffer, ditherBuffer.deviceLocalBuffer);

		UpdateLightBinningDescriptorSet(this->device, this->lightBinningDescriptorSet, this->camera.stagingBuffer, this->framebufferDims.stagingBuffer, this->optimizedVisibleLights.deviceLocalBuffer,
			this->lightBins.deviceLocalBuffer, this->lightBinLightCounts.deviceLocalBuffer, this->lightBinDims.stagingBuffer);

		Span<int> lightBinDimsValues(reinterpret_cast<int*>(this->lightBinDims.stagingHostMappedBytes.begin()), 6);
		lightBinDimsValues[0] = lightBinWidth;
		lightBinDimsValues[1] = lightBinHeight;
		lightBinDimsValues[2] = lightBinCountX;
		lightBinDimsValues[3] = lightBinCountY;
		lightBinDimsValues[4] = clampedVisibleLightCount;
		lightBinDimsValues[5] = ditherBufferIndex;

		auto copyCommand = [this]()
		{
			const int optimizedLightsByteCount = this->optimizedVisibleLights.stagingHostMappedBytes.getCount();
			CopyBufferToBuffer(this->optimizedVisibleLights.stagingBuffer, this->optimizedVisibleLights.deviceLocalBuffer, 0, optimizedLightsByteCount, this->commandBuffer);

			const int lightBinsByteCount = this->lightBins.stagingHostMappedBytes.getCount();
			CopyBufferToBuffer(this->lightBins.stagingBuffer, this->lightBins.deviceLocalBuffer, 0, lightBinsByteCount, this->commandBuffer);

			const int lightBinLightCountsByteCount = this->lightBinLightCounts.stagingHostMappedBytes.getCount();
			CopyBufferToBuffer(this->lightBinLightCounts.stagingBuffer, this->lightBinLightCounts.deviceLocalBuffer, 0, lightBinLightCountsByteCount, this->commandBuffer);
		};

		this->copyCommands.emplace_back(std::move(copyCommand));

		vk::MemoryBarrier hostCoherentMemoryBarrier;
		hostCoherentMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
		hostCoherentMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eUniformRead | vk::AccessFlagBits::eShaderRead;

		this->commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eHost,
			vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader,
			vk::DependencyFlags(),
			hostCoherentMemoryBarrier,
			vk::ArrayProxy<const vk::BufferMemoryBarrier>(),
			vk::ArrayProxy<const vk::ImageMemoryBarrier>());
	}

	if (!this->copyCommands.empty())
	{
		for (const std::function<void()> &copyCommand : this->copyCommands)
		{
			copyCommand();
		}

		this->copyCommands.clear();

		vk::MemoryBarrier copyMemoryBarrier;
		copyMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		copyMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eIndexRead | vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eShaderRead;

		this->commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader,
			vk::DependencyFlags(),
			copyMemoryBarrier,
			vk::ArrayProxy<vk::BufferMemoryBarrier>(),
			vk::ArrayProxy<vk::ImageMemoryBarrier>());
	}

	if (anySceneDrawCalls)
	{
		// Calculate visible light binning.
		constexpr vk::PipelineBindPoint computePipelineBindPoint = vk::PipelineBindPoint::eCompute;

		this->commandBuffer.bindPipeline(computePipelineBindPoint, this->lightBinningPipeline);
		this->commandBuffer.bindDescriptorSets(computePipelineBindPoint, this->lightBinningPipelineLayout, LightBinningDescriptorSetLayoutIndex, this->lightBinningDescriptorSet, vk::ArrayProxy<const uint32_t>());
		this->commandBuffer.dispatch(lightBinCountX, lightBinCountY, 1);

		vk::MemoryBarrier lightBinningComputeMemoryBarrier;
		lightBinningComputeMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		lightBinningComputeMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		this->commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::DependencyFlags(),
			lightBinningComputeMemoryBarrier,
			vk::ArrayProxy<vk::BufferMemoryBarrier>(),
			vk::ArrayProxy<vk::ImageMemoryBarrier>());
	}

	for (int i = 0; i < VulkanRenderBackend::MAX_SCENE_FRAMEBUFFERS; i++)
	{
		constexpr vk::ClearColorValue sceneClearColor(0, 0, 0, 0); // Only uses R channel.

		vk::ImageSubresourceRange clearColorImageSubresourceRange;
		clearColorImageSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		clearColorImageSubresourceRange.baseMipLevel = 0;
		clearColorImageSubresourceRange.levelCount = 1;
		clearColorImageSubresourceRange.baseArrayLayer = 0;
		clearColorImageSubresourceRange.layerCount = 1;

		ApplyColorImageLayoutTransition(
			this->colorImages[i],
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			vk::AccessFlagBits::eNone,
			vk::AccessFlagBits::eTransferWrite,
			this->commandBuffer);

		this->commandBuffer.clearColorImage(this->colorImages[i], vk::ImageLayout::eTransferDstOptimal, sceneClearColor, clearColorImageSubresourceRange);

		ApplyColorImageLayoutTransition(
			this->colorImages[i],
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlagBits::eTransferWrite,
			vk::AccessFlagBits::eColorAttachmentWrite,
			this->commandBuffer);
	}

	constexpr vk::ClearDepthStencilValue sceneClearDepthStencil(1.0f, 0);

	vk::ImageSubresourceRange clearDepthImageSubresourceRange;
	clearDepthImageSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	clearDepthImageSubresourceRange.baseMipLevel = 0;
	clearDepthImageSubresourceRange.levelCount = 1;
	clearDepthImageSubresourceRange.baseArrayLayer = 0;
	clearDepthImageSubresourceRange.layerCount = 1;

	ApplyDepthImageLayoutTransition(
		this->depthImage,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eTransferWrite,
		this->commandBuffer);

	this->commandBuffer.clearDepthStencilImage(this->depthImage, vk::ImageLayout::eTransferDstOptimal, sceneClearDepthStencil, clearDepthImageSubresourceRange);

	ApplyDepthImageLayoutTransition(
		this->depthImage,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eDepthStencilAttachmentOptimal,
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eEarlyFragmentTests,
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eDepthStencilAttachmentWrite,
		this->commandBuffer);

	constexpr vk::PipelineBindPoint graphicsPipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	int targetFramebufferIndex = 0; // Ping-pong depending on current scene render pass.
	int inputFramebufferIndex = targetFramebufferIndex ^ 1;

	if (anySceneDrawCalls)
	{
		vk::Viewport sceneViewport;
		sceneViewport.width = static_cast<float>(this->internalExtent.width);
		sceneViewport.height = static_cast<float>(this->internalExtent.height);
		sceneViewport.minDepth = 0.0f;
		sceneViewport.maxDepth = 1.0f;

		vk::Rect2D sceneViewportScissor;
		sceneViewportScissor.extent = this->internalExtent;

		this->commandBuffer.setViewport(0, sceneViewport);
		this->commandBuffer.setScissor(0, sceneViewportScissor);

		ApplyColorImageLayoutTransition(
			this->colorImages[inputFramebufferIndex],
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eShaderRead,
			this->commandBuffer);

		vk::Pipeline currentPipeline;
		RenderMultipassType currentMultipassType = RenderMultipassType::None;
		VertexPositionBufferID currentVertexPositionBufferID = -1;
		VertexAttributeBufferID currentVertexTexCoordBufferID = -1;
		IndexBufferID currentIndexBufferID = -1;
		int currentIndexBufferIndexCount = 0;
		ObjectTextureID currentTextureID = -1;
		for (int i = 0; i < renderCommandList.entryCount; i++)
		{
			for (const RenderDrawCall &drawCall : renderCommandList.entries[i])
			{
				const RenderMultipassType multipassType = drawCall.multipassType;
				const bool isStarsBegin = (currentMultipassType != RenderMultipassType::Stars) && (multipassType == RenderMultipassType::Stars);
				const bool isStarsEnd = (currentMultipassType == RenderMultipassType::Stars) && (multipassType != RenderMultipassType::Stars);
				const bool isGhostsBegin = (currentMultipassType != RenderMultipassType::Ghosts) && (multipassType == RenderMultipassType::Ghosts);
				const bool isGhostsEnd = (currentMultipassType == RenderMultipassType::Ghosts) && (multipassType != RenderMultipassType::Ghosts);
				const bool isPuddlesBegin = (currentMultipassType != RenderMultipassType::Puddles) && (multipassType == RenderMultipassType::Puddles);
				const bool isPuddlesEnd = (currentMultipassType == RenderMultipassType::Puddles) && (multipassType != RenderMultipassType::Puddles);

				const bool shouldStartRenderPass = !currentPipeline || isStarsBegin || isStarsEnd || isGhostsBegin || isGhostsEnd || isPuddlesBegin || isPuddlesEnd;
				const bool shouldPingPong = isStarsBegin || isGhostsBegin || isPuddlesBegin;

				const VulkanMaterial &material = this->materialPool.get(drawCall.materialID);
				const vk::PipelineLayout pipelineLayout = material.pipelineLayout;
				const vk::Pipeline pipeline = material.pipeline;

				if (pipeline != currentPipeline)
				{
					if (shouldStartRenderPass)
					{
						if (currentPipeline)
						{
							this->commandBuffer.endRenderPass();
						}

						if (shouldPingPong)
						{
							// Copy sampled image framebuffer into color attachment framebuffer (unfortunate side effect of ping-pong pattern is having to also copy src -> dst).
							ApplyColorImageLayoutTransition(
								this->colorImages[targetFramebufferIndex],
								vk::ImageLayout::eColorAttachmentOptimal,
								vk::ImageLayout::eTransferSrcOptimal,
								vk::PipelineStageFlagBits::eColorAttachmentOutput,
								vk::PipelineStageFlagBits::eTransfer,
								vk::AccessFlagBits::eColorAttachmentWrite,
								vk::AccessFlagBits::eTransferRead,
								this->commandBuffer);

							ApplyColorImageLayoutTransition(
								this->colorImages[inputFramebufferIndex],
								vk::ImageLayout::eShaderReadOnlyOptimal,
								vk::ImageLayout::eTransferDstOptimal,
								vk::PipelineStageFlagBits::eFragmentShader,
								vk::PipelineStageFlagBits::eTransfer,
								vk::AccessFlagBits::eShaderRead,
								vk::AccessFlagBits::eTransferWrite,
								this->commandBuffer);

							CopyColorImageToImage(this->colorImages[targetFramebufferIndex], this->colorImages[inputFramebufferIndex], this->internalExtent, this->commandBuffer);

							ApplyColorImageLayoutTransition(
								this->colorImages[targetFramebufferIndex],
								vk::ImageLayout::eTransferSrcOptimal,
								vk::ImageLayout::eShaderReadOnlyOptimal,
								vk::PipelineStageFlagBits::eTransfer,
								vk::PipelineStageFlagBits::eFragmentShader,
								vk::AccessFlagBits::eTransferRead,
								vk::AccessFlagBits::eShaderRead,
								this->commandBuffer);

							ApplyColorImageLayoutTransition(
								this->colorImages[inputFramebufferIndex],
								vk::ImageLayout::eTransferDstOptimal,
								vk::ImageLayout::eColorAttachmentOptimal,
								vk::PipelineStageFlagBits::eTransfer,
								vk::PipelineStageFlagBits::eColorAttachmentOutput,
								vk::AccessFlagBits::eTransferWrite,
								vk::AccessFlagBits::eColorAttachmentWrite,
								this->commandBuffer);

							targetFramebufferIndex ^= 1;
							inputFramebufferIndex ^= 1;
						}

						vk::RenderPassBeginInfo sceneRenderPassBeginInfo;
						sceneRenderPassBeginInfo.renderPass = this->sceneRenderPass;
						sceneRenderPassBeginInfo.framebuffer = this->sceneFramebuffers[targetFramebufferIndex];
						sceneRenderPassBeginInfo.renderArea.extent = this->internalExtent;

						this->commandBuffer.beginRenderPass(sceneRenderPassBeginInfo, vk::SubpassContents::eInline);
					}

					currentPipeline = pipeline;
					this->commandBuffer.bindPipeline(graphicsPipelineBindPoint, pipeline);
					this->commandBuffer.bindDescriptorSets(graphicsPipelineBindPoint, pipelineLayout, GlobalDescriptorSetLayoutIndex, this->globalDescriptorSets[inputFramebufferIndex], vk::ArrayProxy<const uint32_t>());
					this->commandBuffer.bindDescriptorSets(graphicsPipelineBindPoint, pipelineLayout, LightDescriptorSetLayoutIndex, this->lightDescriptorSet, vk::ArrayProxy<const uint32_t>());

					currentMultipassType = multipassType;
				}

				constexpr vk::DeviceSize bufferOffset = 0;

				const VertexPositionBufferID vertexPositionBufferID = drawCall.positionBufferID;
				if (vertexPositionBufferID != currentVertexPositionBufferID)
				{
					currentVertexPositionBufferID = vertexPositionBufferID;

					const VulkanBuffer &vertexPositionBuffer = this->vertexPositionBufferPool.get(vertexPositionBufferID);
					const VulkanBufferVertexPositionInfo &vertexPositionInfo = vertexPositionBuffer.vertexPosition;
					this->commandBuffer.bindVertexBuffers(0, vertexPositionBuffer.deviceLocalBuffer, bufferOffset);
				}

				const VertexAttributeBufferID vertexTexCoordsBufferID = drawCall.texCoordBufferID;
				if (vertexTexCoordsBufferID != currentVertexTexCoordBufferID)
				{
					currentVertexTexCoordBufferID = vertexTexCoordsBufferID;

					const VulkanBuffer &vertexTexCoordsBuffer = this->vertexAttributeBufferPool.get(vertexTexCoordsBufferID);
					const VulkanBufferVertexAttributeInfo &vertexTexCoordsInfo = vertexTexCoordsBuffer.vertexAttribute;
					this->commandBuffer.bindVertexBuffers(1, vertexTexCoordsBuffer.deviceLocalBuffer, bufferOffset);
				}

				const IndexBufferID indexBufferID = drawCall.indexBufferID;
				if (indexBufferID != currentIndexBufferID)
				{
					currentIndexBufferID = indexBufferID;

					const VulkanBuffer &indexBuffer = this->indexBufferPool.get(indexBufferID);
					const VulkanBufferIndexInfo &indexInfo = indexBuffer.index;
					currentIndexBufferIndexCount = indexInfo.indexCount;

					this->commandBuffer.bindIndexBuffer(indexBuffer.deviceLocalBuffer, bufferOffset, vk::IndexType::eUint32);
				}

				const VulkanBuffer &transformBuffer = this->uniformBufferPool.get(drawCall.transformBufferID);
				const VulkanBufferUniformInfo &transformBufferInfo = transformBuffer.uniform;
				uint32_t transformBufferDynamicOffset = drawCall.transformIndex * transformBufferInfo.bytesPerStride;
				this->commandBuffer.bindDescriptorSets(graphicsPipelineBindPoint, pipelineLayout, TransformDescriptorSetLayoutIndex, transformBufferInfo.descriptorSet, transformBufferDynamicOffset);
				this->commandBuffer.bindDescriptorSets(graphicsPipelineBindPoint, pipelineLayout, MaterialDescriptorSetLayoutIndex, material.descriptorSet, vk::ArrayProxy<const uint32_t>());

				uint32_t pushConstantOffset = 0;
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

		this->commandBuffer.endRenderPass();

		// Prepare final scene image for UI pass.
		ApplyColorImageLayoutTransition(
			this->colorImages[targetFramebufferIndex],
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eShaderRead,
			this->commandBuffer);
	}

	vk::RenderPassBeginInfo uiRenderPassBeginInfo;
	uiRenderPassBeginInfo.renderPass = this->uiRenderPass;
	uiRenderPassBeginInfo.framebuffer = this->uiFramebuffers[acquiredSwapchainImageIndex];
	uiRenderPassBeginInfo.renderArea.extent = this->swapchainExtent;

	this->commandBuffer.beginRenderPass(uiRenderPassBeginInfo, vk::SubpassContents::eInline);

	// Conditionally clear UI framebuffer area depending on scene view.
	vk::ClearRect uiClearRect;
	uiClearRect.baseArrayLayer = 0;
	uiClearRect.layerCount = 1;

	if (!anySceneDrawCalls)
	{
		uiClearRect.rect.offset = vk::Offset2D();
		uiClearRect.rect.extent = vk::Extent2D(this->swapchainExtent.width, this->swapchainExtent.height);
	}
	else if (this->sceneViewExtent.height < this->swapchainExtent.height)
	{
		// Clear non-scene-view portion for classic mode interface.
		uiClearRect.rect.offset = vk::Offset2D(0, this->sceneViewExtent.height);
		uiClearRect.rect.extent = vk::Extent2D(this->swapchainExtent.width, this->swapchainExtent.height - this->sceneViewExtent.height);
	}

	if (uiClearRect.rect.extent.height > 0)
	{
		vk::ClearValue uiClearValue;
		uiClearValue.color = vk::ClearColorValue(frameSettings.clearColor.r, frameSettings.clearColor.g, frameSettings.clearColor.b, frameSettings.clearColor.a);

		vk::ClearAttachment uiClearAttachment;
		uiClearAttachment.aspectMask = vk::ImageAspectFlagBits::eColor;
		uiClearAttachment.colorAttachment = 0;
		uiClearAttachment.clearValue = uiClearValue;

		this->commandBuffer.clearAttachments(uiClearAttachment, uiClearRect);
	}

	const vk::PipelineLayout uiPipelineLayout = this->pipelineLayouts[UiPipelineKeyIndex];

	constexpr vk::DeviceSize zeroBufferOffset = 0;
	const VulkanBuffer &uiVertexPositionBuffer = this->vertexPositionBufferPool.get(this->uiVertexPositionBufferID);
	this->commandBuffer.bindVertexBuffers(0, uiVertexPositionBuffer.deviceLocalBuffer, zeroBufferOffset);

	const VulkanBuffer &uiVertexAttributeBuffer = this->vertexAttributeBufferPool.get(this->uiVertexAttributeBufferID);
	this->commandBuffer.bindVertexBuffers(1, uiVertexAttributeBuffer.deviceLocalBuffer, zeroBufferOffset);

	if (anySceneDrawCalls)
	{
		// Draw scene view into the UI.
		vk::Viewport conversionViewport;
		conversionViewport.width = static_cast<float>(this->sceneViewExtent.width);
		conversionViewport.height = static_cast<float>(this->sceneViewExtent.height);
		conversionViewport.minDepth = 0.0f;
		conversionViewport.maxDepth = 1.0f;

		vk::Rect2D conversionViewportScissor;
		conversionViewportScissor.extent = this->sceneViewExtent;

		this->commandBuffer.setViewport(0, conversionViewport);
		this->commandBuffer.setScissor(0, conversionViewportScissor);

		this->commandBuffer.bindPipeline(graphicsPipelineBindPoint, this->conversionPipeline);

		UpdateConversionDescriptorSet(this->device, this->conversionDescriptorSet, this->colorImageViews[targetFramebufferIndex], this->colorSampler, paletteTexture->imageView, paletteTexture->sampler);
		this->commandBuffer.bindDescriptorSets(graphicsPipelineBindPoint, uiPipelineLayout, ConversionDescriptorSetLayoutIndex, this->conversionDescriptorSet, vk::ArrayProxy<const uint32_t>());

		// Fullscreen quad for scene view.
		constexpr float conversionRectX = 0.0f;
		constexpr float conversionRectY = 0.0f;
		const float conversionRectWidth = static_cast<float>(this->sceneViewExtent.width);
		const float conversionRectHeight = static_cast<float>(this->sceneViewExtent.height);
		const float conversionVertexShaderPushConstants[] =
		{
			conversionRectX,
			conversionRectY,
			conversionRectWidth,
			conversionRectHeight,
			static_cast<float>(this->sceneViewExtent.width),
			static_cast<float>(this->sceneViewExtent.height)
		};

		this->commandBuffer.pushConstants<float>(uiPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, conversionVertexShaderPushConstants);

		const int conversionVertexCount = uiVertexPositionBuffer.vertexPosition.vertexCount;
		constexpr int conversionInstanceCount = 1;
		this->commandBuffer.draw(conversionVertexCount, conversionInstanceCount, 0, 0);
	}

	if (uiCommandList.entryCount > 0)
	{
		vk::Viewport uiViewport;
		uiViewport.width = static_cast<float>(this->swapchainExtent.width);
		uiViewport.height = static_cast<float>(this->swapchainExtent.height);
		uiViewport.minDepth = 0.0f;
		uiViewport.maxDepth = 1.0f;

		vk::Rect2D uiViewportScissor;
		uiViewportScissor.extent = this->swapchainExtent;

		this->commandBuffer.setViewport(0, uiViewport);
		this->commandBuffer.setScissor(0, uiViewportScissor);

		const VulkanPipeline &uiPipeline = this->graphicsPipelines.get(UiPipelineKeyIndex);
		this->commandBuffer.bindPipeline(graphicsPipelineBindPoint, uiPipeline.pipeline);

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

				this->commandBuffer.bindDescriptorSets(graphicsPipelineBindPoint, uiPipelineLayout, UiMaterialDescriptorSetLayoutIndex, *textureDescriptorSet, vk::ArrayProxy<const uint32_t>());

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

				const int uiVertexCount = uiVertexPositionBuffer.vertexPosition.vertexCount;
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
