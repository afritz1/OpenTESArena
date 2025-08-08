#include <algorithm>
#include <fstream>
#include <limits>

#include "SDL_vulkan.h"

#include "RenderInitSettings.h"
#include "VulkanRenderBackend.h"
#include "Window.h"
#include "../Assets/TextureBuilder.h"
#include "../Assets/TextureManager.h"
#include "../UI/Surface.h"

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

		int discreteGpuCount = 0;
		int integratedGpuCount = 0;
		int cpuCount = 0;
		int virtualGpuCount = 0;
		int otherCount = 0;

		int bestDiscreteGpuIndex = -1;
		int bestIntegratedGpuIndex = -1;
		int bestCpuIndex = -1;
		int bestVirtualGpuIndex = -1;
		int bestOtherIndex = -1;

		for (int i = 0; i < physicalDevices.getCount(); i++)
		{
			const vk::PhysicalDevice &physicalDevice = physicalDevices[i];
			const vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();
			const vk::PhysicalDeviceType physicalDeviceType = physicalDeviceProperties.deviceType;

			switch (physicalDeviceType)
			{
			case vk::PhysicalDeviceType::eDiscreteGpu:
				bestDiscreteGpuIndex = i;
				discreteGpuCount++;
				break;
			case vk::PhysicalDeviceType::eIntegratedGpu:
				bestIntegratedGpuIndex = i;
				integratedGpuCount++;
				break;
			case vk::PhysicalDeviceType::eCpu:
				bestCpuIndex = i;
				cpuCount++;
				break;
			case vk::PhysicalDeviceType::eVirtualGpu:
				bestVirtualGpuIndex = i;
				virtualGpuCount++;
				break;
			case vk::PhysicalDeviceType::eOther:
				bestOtherIndex = i;
				otherCount++;
				break;
			}
		}

		DebugLogFormat("Physical devices: %d discrete GPU(s), %d integrated GPU(s), %d CPU(s), %d virtual GPU(s), %d other(s).",
			discreteGpuCount, integratedGpuCount, cpuCount, virtualGpuCount, otherCount);

		const int bestPhysicalDeviceIndices[] =
		{
			bestDiscreteGpuIndex,
			bestIntegratedGpuIndex,
			bestVirtualGpuIndex,
			bestCpuIndex
			// Don't want 'other' for now
		};

		vk::PhysicalDevice selectedPhysicalDevice;
		for (const int physicalDeviceIndex : bestPhysicalDeviceIndices)
		{
			if (physicalDeviceIndex >= 0)
			{
				selectedPhysicalDevice = physicalDevices[physicalDeviceIndex];
				break;
			}
		}

		if (selectedPhysicalDevice)
		{
			const vk::PhysicalDeviceProperties selectedPhysicalDeviceProperties = selectedPhysicalDevice.getProperties();
			DebugLogFormat("Selected: %s", selectedPhysicalDeviceProperties.deviceName.data());
			return selectedPhysicalDevice;
		}

		DebugLogError("No valid physical device available.");
		return nullptr;
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
			vk::ImageViewCreateInfo imageViewCreateInfo;
			imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
			imageViewCreateInfo.format = surfaceFormat.format;
			imageViewCreateInfo.components = { vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity };
			imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.image = swapchainImages[i];

			vk::ResultValue<vk::ImageView> imageViewCreateResult = device.createImageView(imageViewCreateInfo);
			if (imageViewCreateResult.result != vk::Result::eSuccess)
			{
				DebugLogErrorFormat("Couldn't create image view index %d (%d).", i, imageViewCreateResult.result);
				return false;
			}

			(*outImageViews)[i] = std::move(imageViewCreateResult.value);
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
}

// Vulkan buffers
namespace
{
	bool TryCreateBuffer(vk::Device device, int byteCount, vk::BufferUsageFlags usageFlags, bool isCpuVisible, uint32_t queueFamilyIndex,
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
		const vk::MemoryPropertyFlags bufferMemoryPropertyFlags = isCpuVisible ? (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) : vk::MemoryPropertyFlagBits::eDeviceLocal;

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

	void TransitionImageLayout(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer commandBuffer)
	{
		const bool isTransitionToInitialPopulate = (oldLayout == vk::ImageLayout::eUndefined) && (newLayout == vk::ImageLayout::eTransferDstOptimal);
		const bool isTransitionToShaderReadOnly = (oldLayout == vk::ImageLayout::eTransferDstOptimal) && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal);

		vk::ImageMemoryBarrier imageMemoryBarrier;
		imageMemoryBarrier.oldLayout = oldLayout;
		imageMemoryBarrier.newLayout = newLayout;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
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
		vk::MemoryBarrier memoryBarrier;
		vk::BufferMemoryBarrier bufferMemoryBarrier;
		commandBuffer.pipelineBarrier(srcPipelineStageFlags, dstPipelineStageFlags, dependencyFlags, memoryBarrier, bufferMemoryBarrier, imageMemoryBarrier);
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

// Vulkan pipelines
namespace
{
	bool TryCreatePipelineLayout(vk::Device device, vk::PipelineLayout *outPipelineLayout)
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
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

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
		pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
		pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
		pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

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

void VulkanVertexPositionBuffer::init(int vertexCount, int componentsPerVertex)
{
	// @todo vk::Buffer creation
}

void VulkanVertexAttributeBuffer::init(int vertexCount, int componentsPerVertex)
{
	// @todo vk::Buffer creation
}

void VulkanIndexBuffer::init(int indexCount)
{
	// @todo vk::Buffer creation
}

VulkanUniformBuffer::VulkanUniformBuffer()
{
	this->elementCount = 0;
	this->sizeOfElement = 0;
	this->alignmentOfElement = 0;
}

void VulkanUniformBuffer::init(int elementCount, size_t sizeOfElement, size_t alignmentOfElement)
{
	this->elementCount = elementCount;
	this->sizeOfElement = sizeOfElement;
	this->alignmentOfElement = alignmentOfElement;
	// @todo vk::Buffer creation
}

VulkanTexture::VulkanTexture()
{
	this->width = 0;
	this->height = 0;
	this->bytesPerTexel = 0;
}

void VulkanTexture::init(int width, int height, int bytesPerTexel)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(bytesPerTexel > 0);

	this->width = width;
	this->height = height;
	this->bytesPerTexel = bytesPerTexel;

	const int texelCount = width * height;
	const int byteCount = texelCount * bytesPerTexel;
	this->texels.init(byteCount);
	// @todo vk::Image creation
}

VulkanObjectTextureAllocator::VulkanObjectTextureAllocator()
{
	this->pool = nullptr;
}

void VulkanObjectTextureAllocator::init(VulkanObjectTexturePool *pool, vk::Device device)
{
	this->pool = pool;
	this->device = device;
}

ObjectTextureID VulkanObjectTextureAllocator::create(int width, int height, int bytesPerTexel)
{
	const ObjectTextureID textureID = this->pool->alloc();
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate object texture with dims %dx%d and %d bytes per texel.", width, height, bytesPerTexel);
		return -1;
	}

	VulkanTexture &texture = this->pool->get(textureID);
	texture.init(width, height, bytesPerTexel);

	// @todo vk::Image creation

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
		DebugLogErrorFormat("Couldn't allocate object texture from texture builder with dims %dx%d and %d bytes per texel.", width, height, bytesPerTexel);
		return -1;
	}

	VulkanTexture &texture = this->pool->get(textureID);

	if (bytesPerTexel == 1)
	{
		Span2D<const uint8_t> srcTexels = textureBuilder.getTexels8();
		uint8_t *dstTexels = reinterpret_cast<uint8_t*>(texture.texels.begin());
		std::copy(srcTexels.begin(), srcTexels.end(), dstTexels);
	}
	else if (bytesPerTexel == 4)
	{
		Span2D<const uint32_t> srcTexels = textureBuilder.getTexels32();
		uint32_t *dstTexels = reinterpret_cast<uint32_t*>(texture.texels.begin());
		std::copy(srcTexels.begin(), srcTexels.end(), dstTexels);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(bytesPerTexel));
	}

	// @todo vk::Image creation

	return textureID;
}

void VulkanObjectTextureAllocator::free(ObjectTextureID textureID)
{
	VulkanTexture *texture = this->pool->tryGet(textureID);
	if (texture != nullptr)
	{
		// @todo vk::Image destroy
	}

	this->pool->free(textureID);
}

LockedTexture VulkanObjectTextureAllocator::lock(ObjectTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);

	// @todo: vk::Image locking/allocating/binding/mapping to host memory

	return LockedTexture(Span2D<std::byte>(static_cast<std::byte*>(texture.texels.begin()), texture.width, texture.height), texture.bytesPerTexel);
}

void VulkanObjectTextureAllocator::unlock(ObjectTextureID textureID)
{
	// @todo: vk::Image unmap etc
}

VulkanUiTextureAllocator::VulkanUiTextureAllocator()
{
	this->pool = nullptr;
}

void VulkanUiTextureAllocator::init(VulkanUiTexturePool *pool, vk::Device device)
{
	this->pool = pool;
	this->device = device;
}

UiTextureID VulkanUiTextureAllocator::create(int width, int height)
{
	const UiTextureID textureID = this->pool->alloc();
	if (textureID < 0)
	{
		DebugLogErrorFormat("Couldn't allocate UI texture with dims %dx%d.", width, height);
		return -1;
	}

	VulkanTexture &texture = this->pool->get(textureID);
	texture.init(width, height, 4);

	// @todo vk::Image creation

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

	// @todo vk::Image creation w/ texels

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

	// @todo vk::Image creation w/ texels and palette

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
		// @todo vk::Image destroy
	}

	this->pool->free(textureID);
}

LockedTexture VulkanUiTextureAllocator::lock(UiTextureID textureID)
{
	VulkanTexture &texture = this->pool->get(textureID);

	// @todo: vk::Image locking/allocating/binding/mapping to host memory

	return LockedTexture(Span2D<std::byte>(static_cast<std::byte*>(texture.texels.begin()), texture.width, texture.height), texture.bytesPerTexel);
}

void VulkanUiTextureAllocator::unlock(UiTextureID textureID)
{
	// @todo: vk::Image unmap etc
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
	this->objectTextureAllocator.init(&this->objectTexturePool, this->device);
	this->uiTextureAllocator.init(&this->uiTexturePool, this->device);

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

	if (!TryCreatePipelineLayout(this->device, &this->pipelineLayout))
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

	constexpr Vertex vertices[] =
	{
		{ { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
	};

	const uint32_t verticesByteCount = sizeof(Vertex) * static_cast<int>(std::size(vertices));
	if (!TryCreateBuffer(this->device, verticesByteCount, vk::BufferUsageFlagBits::eVertexBuffer, true, this->graphicsQueueFamilyIndex, this->physicalDevice,
		&this->vertexBuffer, &this->vertexBufferDeviceMemory))
	{
		DebugLogErrorFormat("Couldn't create vertex buffer with %d bytes.", verticesByteCount);
		return false;
	}

	if (!TryCopyToBufferHostVisible<Vertex>(this->device, vertices, this->vertexBufferDeviceMemory))
	{
		DebugLogError("Couldn't copy vertices to vertex buffer.");
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
		for (VulkanTexture &texture : this->uiTexturePool.values)
		{
			// @todo device destroy image
		}

		this->uiTexturePool.clear();

		for (VulkanTexture &texture : this->objectTexturePool.values)
		{
			// @todo device destroy image
		}

		this->objectTexturePool.clear();

		for (VulkanLight &light : this->lightPool.values)
		{
			// @todo device destroy something
		}

		this->lightPool.clear();

		for (VulkanUniformBuffer &buffer : this->uniformBufferPool.values)
		{
			// @todo device destroy buffer
		}

		this->uniformBufferPool.clear();

		for (VulkanIndexBuffer &buffer : this->indexBufferPool.values)
		{
			// @todo device destroy buffer
		}

		this->indexBufferPool.clear();

		for (VulkanVertexAttributeBuffer &buffer : this->vertexAttributeBufferPool.values)
		{
			// @todo device destroy buffer
		}

		this->vertexAttributeBufferPool.clear();

		for (VulkanVertexPositionBuffer &buffer : this->vertexPositionBufferPool.values)
		{
			// @todo device destroy buffer
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

		if (this->vertexBufferDeviceMemory)
		{
			this->device.freeMemory(this->vertexBufferDeviceMemory);
			this->vertexBufferDeviceMemory = nullptr;
		}

		if (this->vertexBuffer)
		{
			this->device.destroyBuffer(this->vertexBuffer);
			this->vertexBuffer = nullptr;
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

	VulkanVertexPositionBuffer &buffer = this->vertexPositionBufferPool.get(id);
	buffer.init(vertexCount, componentsPerVertex);

	// @todo vk::Buffer creation

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

	VulkanVertexAttributeBuffer &buffer = this->vertexAttributeBufferPool.get(id);
	buffer.init(vertexCount, componentsPerVertex);

	// @todo vk::Buffer creation

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

	VulkanIndexBuffer &buffer = this->indexBufferPool.get(id);
	buffer.init(indexCount);

	// @todo vk::Buffer creation

	return id;
}

void VulkanRenderBackend::populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions)
{
	VulkanVertexPositionBuffer &buffer = this->vertexPositionBufferPool.get(id);
	// @todo vk::Buffer copy (allocate/bind/map/etc.)
}

void VulkanRenderBackend::populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes)
{
	VulkanVertexAttributeBuffer &buffer = this->vertexAttributeBufferPool.get(id);
	// @todo vk::Buffer copy (allocate/bind/map/etc.)
}

void VulkanRenderBackend::populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices)
{
	VulkanIndexBuffer &buffer = this->indexBufferPool.get(id);
	// @todo vk::Buffer copy (allocate/bind/map/etc.)
}

void VulkanRenderBackend::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	VulkanVertexPositionBuffer *buffer = this->vertexPositionBufferPool.tryGet(id);
	if (buffer != nullptr)
	{
		// @todo vk::Buffer destroy
	}

	this->vertexPositionBufferPool.free(id);
}

void VulkanRenderBackend::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	VulkanVertexAttributeBuffer *buffer = this->vertexAttributeBufferPool.tryGet(id);
	if (buffer != nullptr)
	{
		// @todo vk::Buffer destroy
	}

	this->vertexAttributeBufferPool.free(id);
}

void VulkanRenderBackend::freeIndexBuffer(IndexBufferID id)
{
	VulkanIndexBuffer *buffer = this->indexBufferPool.tryGet(id);
	if (buffer != nullptr)
	{
		// @todo vk::Buffer destroy
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

	VulkanUniformBuffer &buffer = this->uniformBufferPool.get(id);
	buffer.init(elementCount, sizeOfElement, alignmentOfElement);

	// @todo vk::Buffer creation

	return id;
}

void VulkanRenderBackend::populateUniformBuffer(UniformBufferID id, Span<const std::byte> data)
{
	VulkanUniformBuffer &buffer = this->uniformBufferPool.get(id);
	// @todo vk::Buffer copy (allocate/bind/map/etc.)
}

void VulkanRenderBackend::populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData)
{
	VulkanUniformBuffer &buffer = this->uniformBufferPool.get(id);
	// @todo vk::Buffer copy (allocate/bind/map/etc.)
}

void VulkanRenderBackend::freeUniformBuffer(UniformBufferID id)
{
	VulkanUniformBuffer *buffer = this->uniformBufferPool.tryGet(id);
	if (buffer != nullptr)
	{
		// @todo vk::Buffer destroy
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
		// @todo vk destroy something
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

	constexpr vk::DeviceSize vertexBufferOffset = 0;
	this->commandBuffer.bindVertexBuffers(0, this->vertexBuffer, vertexBufferOffset);

	constexpr uint32_t vertexCount = 3;
	constexpr uint32_t instanceCount = 1;
	this->commandBuffer.draw(vertexCount, instanceCount, 0, 0);

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
