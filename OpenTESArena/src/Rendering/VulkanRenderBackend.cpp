#include <algorithm>
#include <fstream>
#include <limits>

#include "SDL_vulkan.h"

#include "RenderInitSettings.h"
#include "VulkanRenderBackend.h"
#include "Window.h"
#include "../UI/Surface.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"
#include "components/utilities/File.h"

namespace
{
	struct Vertex
	{
		Float2 position;
		Float3 color;
	};

	constexpr uint32_t RequiredApiVersion = VK_API_VERSION_1_0;

	constexpr const char *ValidationLayers[] =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	constexpr uint32_t INVALID_UINT32 = std::numeric_limits<uint32_t>::max();
	constexpr uint64_t TIMEOUT_UNLIMITED = std::numeric_limits<uint64_t>::max();

	Buffer<const char*> GetInstanceExtensions(SDL_Window *window)
	{
		uint32_t count;
		if (SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr) != SDL_TRUE)
		{
			return Buffer<const char*>();
		}

		Buffer<const char*> extensions(count);
		if (SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.begin()) != SDL_TRUE)
		{
			return Buffer<const char*>();
		}

		return extensions;
	}

	bool IsPhysicalDeviceSuitable(const vk::PhysicalDevice &physicalDevice)
	{
		const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

		const vk::PhysicalDeviceType deviceType = properties.deviceType;
		if (deviceType != vk::PhysicalDeviceType::eDiscreteGpu && deviceType != vk::PhysicalDeviceType::eIntegratedGpu)
		{
			return false;
		}

		const uint32_t deviceApiVersion = properties.apiVersion;
		if (deviceApiVersion < RequiredApiVersion)
		{
			return false;
		}

		return true;
	}

	// maybe the options menu values could be 0: best, 1: index 0 of physical devices, 2: index 1 of physical devices...
	vk::PhysicalDevice GetBestPhysicalDevice(Span<const vk::PhysicalDevice> physicalDevices)
	{
		DebugAssert(physicalDevices.getCount() > 0);

		for (const vk::PhysicalDevice physicalDevice : physicalDevices)
		{
			if (IsPhysicalDeviceSuitable(physicalDevice))
			{
				return physicalDevice;
			}
		}

		DebugUnhandledReturn(vk::PhysicalDevice);
	}

	Buffer<const char*> GetDeviceExtensions()
	{
		Buffer<const char*> extensions(1);
		extensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		return extensions;
	}

	uint32_t FindBufferMemoryTypeIndex(const vk::MemoryRequirements &memoryRequirements, vk::MemoryPropertyFlags flags, const vk::PhysicalDevice &physicalDevice)
	{
		const vk::PhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			const bool memoryRequirementsHasTypeBit = (memoryRequirements.memoryTypeBits & (1 << i)) != 0;
			const bool memoryTypeHasPropertyFlags = (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags;

			if (memoryRequirementsHasTypeBit && memoryTypeHasPropertyFlags)
			{
				return i;
			}
		}

		return INVALID_UINT32;
	}
}

void VulkanObjectTextureAllocator::init(vk::Device device)
{
	this->device = device;
}

ObjectTextureID VulkanObjectTextureAllocator::create(int width, int height, int bytesPerTexel)
{
	DebugNotImplemented();
	return ObjectTextureID();
}

ObjectTextureID VulkanObjectTextureAllocator::create(const TextureBuilder &textureBuilder)
{
	DebugNotImplemented();
	return ObjectTextureID();
}

void VulkanObjectTextureAllocator::free(ObjectTextureID textureID)
{
	DebugNotImplemented();
}

LockedTexture VulkanObjectTextureAllocator::lock(ObjectTextureID textureID)
{
	DebugNotImplemented();
	return LockedTexture();
}

void VulkanObjectTextureAllocator::unlock(ObjectTextureID textureID)
{
	DebugNotImplemented();
}

void VulkanUiTextureAllocator::init(vk::Device device)
{
	this->device = device;
}

UiTextureID VulkanUiTextureAllocator::create(int width, int height)
{
	DebugNotImplemented();
	return UiTextureID();
}

UiTextureID VulkanUiTextureAllocator::create(Span2D<const uint32_t> texels)
{
	DebugNotImplemented();
	return UiTextureID();
}

UiTextureID VulkanUiTextureAllocator::create(Span2D<const uint8_t> texels, const Palette &palette)
{
	DebugNotImplemented();
	return UiTextureID();
}

UiTextureID VulkanUiTextureAllocator::create(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager)
{
	DebugNotImplemented();
	return UiTextureID();
}

void VulkanUiTextureAllocator::free(UiTextureID textureID)
{
	DebugNotImplemented();
}

LockedTexture VulkanUiTextureAllocator::lock(UiTextureID textureID)
{
	DebugNotImplemented();
	return LockedTexture();
}

void VulkanUiTextureAllocator::unlock(UiTextureID textureID)
{
	DebugNotImplemented();
}

bool VulkanRenderBackend::init(const RenderInitSettings &initSettings)
{
	const Window *window = initSettings.window;
	const std::string &dataFolderPath = initSettings.dataFolderPath;

	const Buffer<const char*> instanceExtensions = GetInstanceExtensions(window->window);
	if (instanceExtensions.getCount() == 0)
	{
		DebugLogError("Couldn't get Vulkan instance extensions.");
		return false;
	}

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "OpenTESArena";
	appInfo.applicationVersion = 0;
	appInfo.apiVersion = RequiredApiVersion;

	vk::InstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(std::size(ValidationLayers));
	instanceCreateInfo.ppEnabledLayerNames = ValidationLayers;
	instanceCreateInfo.enabledExtensionCount = instanceExtensions.getCount();
	instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.begin();

	vk::ResultValue<vk::Instance> instanceCreateResult = vk::createInstance(instanceCreateInfo);
	if (instanceCreateResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create vk::Instance (%d).", instanceCreateResult.result);
		return false;
	}

	this->instance = std::move(instanceCreateResult.value);

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
	const std::vector<vk::QueueFamilyProperties> queueFamilyPropertiesList = this->physicalDevice.getQueueFamilyProperties();
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
		vk::ResultValue<uint32_t> surfaceSupportResult = this->physicalDevice.getSurfaceSupportKHR(i, this->surface);
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

	const uint32_t queueFamilyIndices[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

	constexpr float deviceQueuePriority = 1.0f;
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
	deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = &deviceQueuePriority;

	const Buffer<const char*> deviceExtensions = GetDeviceExtensions();

	DebugAssertMsgFormat(graphicsQueueFamilyIndex == presentQueueFamilyIndex, "Queue family indices are different for graphics (%d) and present (%d), not supported yet.", graphicsQueueFamilyIndex, presentQueueFamilyIndex);
	vk::DeviceCreateInfo deviceCreateInfo; // @todo this needs to be 1) graphics and 2) present if their family indices are different
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = deviceExtensions.getCount();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.begin();

	vk::ResultValue<vk::Device> deviceCreateResult = this->physicalDevice.createDevice(deviceCreateInfo);
	if (deviceCreateResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create vk::Device (%d).", deviceCreateResult.result);
		return false;
	}

	this->device = std::move(deviceCreateResult.value);
	this->graphicsQueue = this->device.getQueue(graphicsQueueFamilyIndex, 0);
	this->presentQueue = this->device.getQueue(presentQueueFamilyIndex, 0);
	this->objectTextureAllocator.init(this->device);
	this->uiTextureAllocator.init(this->device);

	vk::ResultValue<vk::SurfaceCapabilitiesKHR> surfaceCapabilitiesResult = this->physicalDevice.getSurfaceCapabilitiesKHR(this->surface);
	if (surfaceCapabilitiesResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't query physical device getSurfaceCapabilitiesKHR() (%d).", surfaceCapabilitiesResult.result);
		return false;
	}

	const vk::SurfaceCapabilitiesKHR surfaceCapabilities = std::move(surfaceCapabilitiesResult.value);

	vk::ResultValue<std::vector<vk::SurfaceFormatKHR>> surfaceFormatsResult = this->physicalDevice.getSurfaceFormatsKHR(this->surface);
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
		if ((currentSurfaceFormat.format == vk::Format::eR8G8B8A8Srgb) &&
			(currentSurfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear))
		{
			surfaceFormat = currentSurfaceFormat;
			break;
		}
	}

	vk::ResultValue<std::vector<vk::PresentModeKHR>> presentModesResult = this->physicalDevice.getSurfacePresentModesKHR(this->surface);
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
		if (currentPresentMode == vk::PresentModeKHR::eImmediate) // No vsync
		{
			presentMode = currentPresentMode;
			break;
		}
	}

	this->swapchainExtent = surfaceCapabilities.currentExtent;
	if (this->swapchainExtent.width == INVALID_UINT32)
	{
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window->window, &windowWidth, &windowHeight);
		this->swapchainExtent.width = windowWidth;
		this->swapchainExtent.height = windowHeight;
	}

	uint32_t swapchainSurfaceImageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0)
	{
		swapchainSurfaceImageCount = std::min(swapchainSurfaceImageCount, surfaceCapabilities.maxImageCount);
	}

	vk::SwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.surface = this->surface;
	swapchainCreateInfo.minImageCount = swapchainSurfaceImageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = this->swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
	{
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

	vk::ResultValue<vk::SwapchainKHR> swapchainCreateResult = this->device.createSwapchainKHR(swapchainCreateInfo);
	if (swapchainCreateResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create device swapchain (%d).", swapchainCreateResult.result);
		return false;
	}

	this->swapchain = std::move(swapchainCreateResult.value);

	vk::ResultValue<std::vector<vk::Image>> swapchainImagesResult = this->device.getSwapchainImagesKHR(this->swapchain);
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

	this->swapchainImageViews.init(swapchainImages.size());
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

		vk::ResultValue<vk::ImageView> imageViewCreateResult = this->device.createImageView(imageViewCreateInfo);
		if (imageViewCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create image view index %d (%d).", i, imageViewCreateResult.result);
			return false;
		}

		this->swapchainImageViews[i] = std::move(imageViewCreateResult.value);
	}

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

	vk::ResultValue<vk::RenderPass> renderPassCreateResult = this->device.createRenderPass(renderPassCreateInfo);
	if (renderPassCreateResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create device render pass (%d).", renderPassCreateResult.result);
		return false;
	}

	this->renderPass = std::move(renderPassCreateResult.value);

	this->swapchainFramebuffers.init(this->swapchainImageViews.getCount());
	for (int i = 0; i < this->swapchainImageViews.getCount(); i++)
	{
		vk::FramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.renderPass = this->renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &this->swapchainImageViews[i];
		framebufferCreateInfo.width = this->swapchainExtent.width;
		framebufferCreateInfo.height = this->swapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		vk::ResultValue<vk::Framebuffer> framebufferCreateResult = this->device.createFramebuffer(framebufferCreateInfo);
		if (framebufferCreateResult.result != vk::Result::eSuccess)
		{
			DebugLogErrorFormat("Couldn't create device framebuffer index %d (%d).", i, framebufferCreateResult.result);
			return false;
		}

		this->swapchainFramebuffers[i] = std::move(framebufferCreateResult.value);
	}

	vk::CommandPoolCreateInfo commandPoolCreateInfo;
	commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	vk::ResultValue<vk::CommandPool> commandPoolCreateResult = this->device.createCommandPool(commandPoolCreateInfo);
	if (commandPoolCreateResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create vk::CommandPool (%d).", commandPoolCreateResult.result);
		return false;
	}

	this->commandPool = std::move(commandPoolCreateResult.value);

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
	commandBufferAllocateInfo.commandPool = this->commandPool;
	commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
	commandBufferAllocateInfo.commandBufferCount = 1;

	vk::ResultValue<std::vector<vk::CommandBuffer>> commandBufferAllocateResult = this->device.allocateCommandBuffers(commandBufferAllocateInfo);
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

	this->commandBuffer = commandBuffers[0];

	const std::string shadersFolderPath = dataFolderPath + "shaders/";
	const std::string vertexShaderBytesFilename = shadersFolderPath + "testVertex.spv";
	const std::string fragmentShaderBytesFilename = shadersFolderPath + "testFragment.spv";
	const Buffer<std::byte> vertexShaderBytes = File::readAllBytes(vertexShaderBytesFilename.c_str());
	const Buffer<std::byte> fragmentShaderBytes = File::readAllBytes(fragmentShaderBytesFilename.c_str());

	vk::ShaderModuleCreateInfo vertexShaderModuleCreateInfo;
	vertexShaderModuleCreateInfo.codeSize = vertexShaderBytes.getCount();
	vertexShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderBytes.begin());

	vk::ResultValue<vk::ShaderModule> vertexShaderModuleResult = this->device.createShaderModule(vertexShaderModuleCreateInfo);
	if (vertexShaderModuleResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create vertex shader module from \"%s\" (%d).", vertexShaderBytesFilename.c_str(), vertexShaderModuleResult.result);
		return false;
	}

	this->vertexShaderModule = std::move(vertexShaderModuleResult.value);

	vk::ShaderModuleCreateInfo fragmentShaderModuleCreateInfo;
	fragmentShaderModuleCreateInfo.codeSize = fragmentShaderBytes.getCount();
	fragmentShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentShaderBytes.begin());

	vk::ResultValue<vk::ShaderModule> fragmentShaderModuleResult = this->device.createShaderModule(fragmentShaderModuleCreateInfo);
	if (fragmentShaderModuleResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create fragment shader module from \"%s\" (%d).", fragmentShaderBytesFilename.c_str(), fragmentShaderModuleResult.result);
		return false;
	}

	this->fragmentShaderModule = std::move(fragmentShaderModuleResult.value);

	vk::PipelineShaderStageCreateInfo vertexPipelineShaderStageCreateInfo;
	vertexPipelineShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertexPipelineShaderStageCreateInfo.module = this->vertexShaderModule;
	vertexPipelineShaderStageCreateInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragmentPipelineShaderStageCreateInfo;
	fragmentPipelineShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragmentPipelineShaderStageCreateInfo.module = this->fragmentShaderModule;
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
	viewport.width = static_cast<float>(this->swapchainExtent.width);
	viewport.height = static_cast<float>(this->swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D viewportScissor;
	viewportScissor.extent = this->swapchainExtent;

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

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	vk::ResultValue<vk::PipelineLayout> pipelineLayoutResult = this->device.createPipelineLayout(pipelineLayoutCreateInfo);
	if (pipelineLayoutResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create device vk::PipelineLayout (%d).", pipelineLayoutResult.result);
		return false;
	}

	this->pipelineLayout = std::move(pipelineLayoutResult.value);

	vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
	graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(std::size(pipelineShaderStageCreateInfos));
	graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos;
	graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.layout = this->pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = this->renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = nullptr;

	vk::PipelineCache pipelineCache;
	vk::ResultValue<vk::Pipeline> graphicsPipelineResult = this->device.createGraphicsPipeline(pipelineCache, graphicsPipelineCreateInfo);
	if (graphicsPipelineResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create device graphics pipeline (%d).", graphicsPipelineResult.result);
		return false;
	}

	this->graphicsPipeline = std::move(graphicsPipelineResult.value);

	constexpr Vertex vertices[] =
	{
		{ { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
	};

	vk::BufferCreateInfo vertexBufferCreateInfo;
	vertexBufferCreateInfo.size = sizeof(Vertex) * std::size(vertices);
	vertexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	vertexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	vertexBufferCreateInfo.queueFamilyIndexCount = 1;
	vertexBufferCreateInfo.pQueueFamilyIndices = &graphicsQueueFamilyIndex;

	vk::ResultValue<vk::Buffer> vertexBufferCreateResult = this->device.createBuffer(vertexBufferCreateInfo);
	if (vertexBufferCreateResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create device vertex buffer (%d).", vertexBufferCreateResult.result);
		return false;
	}

	this->vertexBuffer = std::move(vertexBufferCreateResult.value);

	const vk::MemoryRequirements vertexBufferMemoryRequirements = this->device.getBufferMemoryRequirements(this->vertexBuffer);
	const vk::MemoryPropertyFlags vertexBufferMemoryPropertyFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

	vk::MemoryAllocateInfo vertexBufferMemoryAllocateInfo;
	vertexBufferMemoryAllocateInfo.allocationSize = vertexBufferMemoryRequirements.size;
	vertexBufferMemoryAllocateInfo.memoryTypeIndex = FindBufferMemoryTypeIndex(vertexBufferMemoryRequirements, vertexBufferMemoryPropertyFlags, this->physicalDevice);
	if (vertexBufferMemoryAllocateInfo.memoryTypeIndex == INVALID_UINT32)
	{
		DebugLogErrorFormat("Couldn't find suitable vertex buffer memory type.");
		return false;
	}

	vk::ResultValue<vk::DeviceMemory> vertexBufferDeviceMemoryResult = this->device.allocateMemory(vertexBufferMemoryAllocateInfo);
	if (vertexBufferDeviceMemoryResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't allocate device vertex buffer memory (%d).", vertexBufferDeviceMemoryResult.result);
		return false;
	}

	this->vertexBufferDeviceMemory = std::move(vertexBufferDeviceMemoryResult.value);

	const vk::Result vertexBufferBindMemoryResult = this->device.bindBufferMemory(this->vertexBuffer, this->vertexBufferDeviceMemory, 0);
	if (vertexBufferBindMemoryResult != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't bind device vertex buffer memory (%d).", vertexBufferBindMemoryResult);
		return false;
	}

	vk::ResultValue<void*> vertexBufferMapMemoryResult = this->device.mapMemory(this->vertexBufferDeviceMemory, 0, vertexBufferCreateInfo.size);
	if (vertexBufferMapMemoryResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't map device vertex buffer memory (%d).", vertexBufferMapMemoryResult.result);
		return false;
	}

	void *vertexBufferHostMemory = std::move(vertexBufferMapMemoryResult.value);
	std::copy(std::begin(vertices), std::end(vertices), reinterpret_cast<Vertex*>(vertexBufferHostMemory));
	this->device.unmapMemory(this->vertexBufferDeviceMemory);

	vk::SemaphoreCreateInfo semaphoreCreateInfo;
	vk::ResultValue<vk::Semaphore> imageIsAvailableSemaphoreResult = this->device.createSemaphore(semaphoreCreateInfo);
	if (imageIsAvailableSemaphoreResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create image-is-available semaphore (%d).", imageIsAvailableSemaphoreResult.result);
		return false;
	}

	this->imageIsAvailableSemaphore = std::move(imageIsAvailableSemaphoreResult.value);

	vk::ResultValue<vk::Semaphore> renderIsFinishedSemaphoreResult = this->device.createSemaphore(semaphoreCreateInfo);
	if (renderIsFinishedSemaphoreResult.result != vk::Result::eSuccess)
	{
		DebugLogErrorFormat("Couldn't create render-is-finished semaphore (%d).", renderIsFinishedSemaphoreResult.result);
		return false;
	}

	this->renderIsFinishedSemaphore = std::move(renderIsFinishedSemaphoreResult.value);

	return true;
}

void VulkanRenderBackend::shutdown()
{
	if (this->device)
	{
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
			this->device.destroyFramebuffer(framebuffer);
		}

		this->swapchainFramebuffers.clear();

		if (this->renderPass)
		{
			this->device.destroyRenderPass(this->renderPass);
			this->renderPass = nullptr;
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

		this->swapchainExtent = vk::Extent2D();

		this->presentQueue = nullptr;
		this->graphicsQueue = nullptr;

		this->device.destroy();
		this->device = nullptr;
	}

	if (this->instance)
	{
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
	DebugNotImplemented();
	return VertexPositionBufferID();
}

VertexAttributeBufferID VulkanRenderBackend::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex)
{
	DebugNotImplemented();
	return VertexAttributeBufferID();
}

IndexBufferID VulkanRenderBackend::createIndexBuffer(int indexCount)
{
	DebugNotImplemented();
	return IndexBufferID();
}

void VulkanRenderBackend::populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::freeIndexBuffer(IndexBufferID id)
{
	DebugNotImplemented();
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
	DebugNotImplemented();
	return UniformBufferID();
}

void VulkanRenderBackend::populateUniformBuffer(UniformBufferID id, Span<const std::byte> data)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::freeUniformBuffer(UniformBufferID id)
{
	DebugNotImplemented();
}

RenderLightID VulkanRenderBackend::createLight()
{
	DebugNotImplemented();
	return RenderLightID();
}

void VulkanRenderBackend::setLightPosition(RenderLightID id, const Double3 &worldPoint)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::setLightRadius(RenderLightID id, double startRadius, double endRadius)
{
	DebugNotImplemented();
}

void VulkanRenderBackend::freeLight(RenderLightID id)
{
	DebugNotImplemented();
}

std::optional<Int2> VulkanRenderBackend::tryGetObjectTextureDims(ObjectTextureID id) const
{
	DebugNotImplemented();
	return std::optional<Int2>();
}

std::optional<Int2> VulkanRenderBackend::tryGetUiTextureDims(UiTextureID id) const
{
	DebugNotImplemented();
	return std::optional<Int2>();
}

Renderer3DProfilerData VulkanRenderBackend::getProfilerData() const
{
	DebugNotImplemented();
	return Renderer3DProfilerData(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

Surface VulkanRenderBackend::getScreenshot() const
{
	DebugNotImplemented();
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
