#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "RenderCamera.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "SdlUiRenderer.h"
#include "SoftwareRenderer.h"
#include "VulkanRenderer.h"
#include "../Assets/TextureManager.h"
#include "../Math/MathUtils.h"
#include "../Math/Rect.h"
#include "../UI/CursorAlignment.h"
#include "../UI/GuiUtils.h"
#include "../UI/RenderSpace.h"
#include "../UI/Surface.h"
#include "../UI/UiCommandBuffer.h"
#include "../UI/UiDrawCall.h"
#include "../Utilities/Color.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	constexpr bool UseVulkan = true;
	VulkanRenderer g_vulkanRenderer;

	RenderCamera g_physicsDebugCamera; // Cached every frame for Jolt Physics debug renderer.
	constexpr double PHYSICS_DEBUG_MAX_DISTANCE = 4.0;
	constexpr double PHYSICS_DEBUG_MAX_DISTANCE_SQR = PHYSICS_DEBUG_MAX_DISTANCE * PHYSICS_DEBUG_MAX_DISTANCE;

	const char *GetSdlRenderScaleQuality()
	{
		return "nearest";
	}

	// Helper method for making a renderer context.
	SDL_Renderer *CreateSdlRendererForWindow(SDL_Window *window)
	{
		DebugAssert(!UseVulkan);

		// Automatically choose the best driver.
		constexpr int bestDriver = -1;

		SDL_Renderer *rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_ACCELERATED);
		if (rendererContext == nullptr)
		{
			DebugLogErrorFormat("Couldn't create SDL_Renderer with default driver (%s).", SDL_GetError());
			return nullptr;
		}

		SDL_RendererInfo rendererInfo;
		if (SDL_GetRendererInfo(rendererContext, &rendererInfo) < 0)
		{
			DebugLogErrorFormat("Couldn't get SDL_RendererInfo (%s).", SDL_GetError());
			return nullptr;
		}

		DebugLogFormat("Created renderer \"%s\" (flags: 0x%X).", rendererInfo.name, rendererInfo.flags);

		// Set pixel interpolation hint.
		const SDL_bool status = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, GetSdlRenderScaleQuality());
		if (status != SDL_TRUE)
		{
			DebugLogWarningFormat("Couldn't set SDL rendering interpolation hint (%s).", SDL_GetError());
		}

		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);

		auto isValidWindowSize = [](int width, int height)
		{
			return (width > 0) && (height > 0);
		};

		// Set the size of the render texture to be the size of the whole screen (it automatically scales otherwise).
		// If this fails, the OS might not support hardware accelerated renderers for some reason (such as with Linux),
		// so retry with software.
		if (!isValidWindowSize(windowWidth, windowHeight))
		{
			DebugLogWarningFormat("Failed to init accelerated SDL_Renderer, trying software fallback (%s).", SDL_GetError());
			SDL_DestroyRenderer(rendererContext);

			rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_SOFTWARE);
			if (rendererContext == nullptr)
			{
				DebugLogErrorFormat("Couldn't create software fallback SDL_Renderer (%s).", SDL_GetError());
				return nullptr;
			}

			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			if (!isValidWindowSize(windowWidth, windowHeight))
			{
				DebugLogErrorFormat("Couldn't get software fallback SDL_Window dimensions (%s).", SDL_GetError());
				return nullptr;
			}
		}

		// Set the device-independent resolution for rendering (i.e., the "behind-the-scenes" resolution).
		SDL_RenderSetLogicalSize(rendererContext, windowWidth, windowHeight);

		return rendererContext;
	}

	Int2 MakeInternalRendererDimensions(const Int2 &dimensions, double resolutionScale)
	{
		const double scaledWidthReal = static_cast<double>(dimensions.x) * resolutionScale;
		const double scaledHeightReal = static_cast<double>(dimensions.y) * resolutionScale;

		// Avoid off-by-one like 1079p.
		const int roundedWidth = static_cast<int>(std::round(scaledWidthReal));
		const int roundedHeight = static_cast<int>(std::round(scaledHeightReal));

		// Keep as a multiple of a power of 2 for SIMD-friendliness. Don't worry about skewing aspect ratio at low resolution.
		// The camera retains the projection so the result is taller/wider pixels.
		constexpr int alignment = RendererUtils::RESOLUTION_ALIGNMENT;
		constexpr int alignmentMask = ~(alignment - 1);
		const int alignedWidth = std::max(roundedWidth & alignmentMask, alignment);
		const int alignedHeight = std::max(roundedHeight & alignmentMask, alignment);

		return Int2(alignedWidth, alignedHeight);
	}
}

RenderDisplayMode::RenderDisplayMode(int width, int height, int refreshRate)
{
	this->width = width;
	this->height = height;
	this->refreshRate = refreshRate;
}

RendererProfilerData::RendererProfilerData()
{
	this->width = -1;
	this->height = -1;
	this->pixelCount = -1;
	this->threadCount = -1;
	this->drawCallCount = -1;
	this->presentedTriangleCount = -1;
	this->objectTextureCount = -1;
	this->objectTextureByteCount = -1;
	this->totalLightCount = -1;
	this->totalCoverageTests = -1;
	this->totalDepthTests = -1;
	this->totalColorWrites = -1;
	this->renderTime = 0.0;
	this->presentTime = 0.0;
}

void RendererProfilerData::init(int width, int height, int threadCount, int drawCallCount, int presentedTriangleCount,
	int objectTextureCount, int64_t objectTextureByteCount, int totalLightCount, int64_t totalCoverageTests, int64_t totalDepthTests,
	int64_t totalColorWrites, double renderTime, double presentTime)
{
	this->width = width;
	this->height = height;
	this->pixelCount = width * height;
	this->threadCount = threadCount;
	this->drawCallCount = drawCallCount;
	this->presentedTriangleCount = presentedTriangleCount;
	this->objectTextureCount = objectTextureCount;
	this->objectTextureByteCount = objectTextureByteCount;
	this->totalLightCount = totalLightCount;
	this->totalCoverageTests = totalCoverageTests;
	this->totalDepthTests = totalDepthTests;
	this->totalColorWrites = totalColorWrites;
	this->renderTime = renderTime;
	this->presentTime = presentTime;
}

Renderer::Renderer()
{
	this->window = nullptr;
	this->renderer = nullptr;
	this->nativeTexture = nullptr;
	this->gameWorldTexture = nullptr;
}

Renderer::~Renderer()
{
	DebugLog("Closing.");

	if (this->renderer2D)
	{
		this->renderer2D->shutdown();
	}

	if (this->renderer3D)
	{
		this->renderer3D->shutdown();
	}

	if constexpr (UseVulkan)
	{
		g_vulkanRenderer.shutdown();
	}

	// This also destroys the frame buffer textures.
	SDL_DestroyRenderer(this->renderer);

	this->nativeTexture = nullptr;
	this->gameWorldTexture = nullptr;
	this->renderer = nullptr;
	this->window = nullptr;
}

bool Renderer::init(const Window *window, const RenderResolutionScaleFunc &resolutionScaleFunc, RendererSystemType2D systemType2D, RendererSystemType3D systemType3D,
	int renderThreadsMode, DitheringMode ditheringMode, const std::string &dataFolderPath)
{
	DebugLog("Initializing.");

	this->window = window;
	this->resolutionScaleFunc = resolutionScaleFunc;

	// Use window dimensions, just in case it's fullscreen and the given width and height are ignored.
	const Int2 windowDimensions = window->getDimensions();
	const Int2 viewDims = window->getViewDimensions();
	const double resolutionScale = resolutionScaleFunc();
	const Int2 internalRenderDims = MakeInternalRendererDimensions(viewDims, resolutionScale);

	if constexpr (!UseVulkan)
	{
		// Initialize SDL renderer context.
		this->renderer = CreateSdlRendererForWindow(window->window);
		if (this->renderer == nullptr)
		{
			DebugLogErrorFormat("Couldn't create SDL_Renderer (%s).", SDL_GetError());
			return false;
		}

		// Initialize native frame buffer.
		this->nativeTexture = SDL_CreateTexture(this->renderer, Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, windowDimensions.x, windowDimensions.y);
		if (this->nativeTexture == nullptr)
		{
			DebugLogErrorFormat("Couldn't create SDL_Texture frame buffer (%s).", SDL_GetError());
			return false;
		}

		// Initialize game world destination frame buffer.
		this->gameWorldTexture = SDL_CreateTexture(this->renderer, Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, internalRenderDims.x, internalRenderDims.y);
		if (this->gameWorldTexture == nullptr)
		{
			DebugLogErrorFormat("Couldn't create game world texture with dimensions %dx%d (%s).", internalRenderDims.x, internalRenderDims.y, SDL_GetError());
		}
	}

	// Initialize 2D renderer.
	this->renderer2D = [systemType2D]() -> std::unique_ptr<RendererSystem2D>
	{
		if (systemType2D == RendererSystemType2D::SDL2)
		{
			return std::make_unique<SdlUiRenderer>();
		}
		else
		{
			DebugLogErrorFormat("Unrecognized 2D renderer system type \"%d\".", systemType2D);
			return nullptr;
		}
	}();

	if (!this->renderer2D->init(window->window))
	{
		DebugCrash("Couldn't init 2D renderer.");
	}

	// Initialize 3D renderer.
	this->renderer3D = [systemType3D]() -> std::unique_ptr<RendererSystem3D>
	{
		if (systemType3D == RendererSystemType3D::SoftwareClassic)
		{
			return std::make_unique<SoftwareRenderer>();
		}
		else if (systemType3D == RendererSystemType3D::Vulkan)
		{
			return std::make_unique<VulkanRenderer>();
		}
		else
		{
			DebugLogErrorFormat("Unrecognized 3D renderer system type \"%d\".", systemType3D);
			return nullptr;
		}
	}();

	RenderInitSettings initSettings;
	initSettings.init(window->window, dataFolderPath, internalRenderDims.x, internalRenderDims.y, renderThreadsMode, ditheringMode);
	if (!this->renderer3D->init(initSettings))
	{
		DebugLogError("Couldn't init RendererSystem3D.");
		this->renderer3D->shutdown();
		return false;
	}

	return true;
}

Surface Renderer::getScreenshot() const
{
	const Int2 dimensions = this->window->getDimensions();
	Surface screenshot = Surface::createWithFormat(dimensions.x, dimensions.y, Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
	const int status = SDL_RenderReadPixels(this->renderer, nullptr, screenshot.get()->format->format, screenshot.get()->pixels, screenshot.get()->pitch);

	if (status != 0)
	{
		DebugCrashFormat("Couldn't take screenshot (%s).", SDL_GetError());
	}

	return screenshot;
}

const RendererProfilerData &Renderer::getProfilerData() const
{
	return this->profilerData;
}

void Renderer::resize(int width, int height)
{
	// The window's dimensions are resized automatically by SDL. The renderer's are not.
	const Int2 windowDims = this->window->getDimensions();
	DebugAssertMsg(windowDims.x == width, "Mismatched resize widths.");
	DebugAssertMsg(windowDims.y == height, "Mismatched resize heights.");

	SDL_RenderSetLogicalSize(this->renderer, width, height);

	// Reinitialize native frame buffer.
	if (this->nativeTexture != nullptr)
	{
		SDL_DestroyTexture(this->nativeTexture);
	}

	this->nativeTexture = SDL_CreateTexture(this->renderer, Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, width, height);
	if (this->nativeTexture == nullptr)
	{
		DebugLogErrorFormat("Couldn't recreate native frame buffer for resize to %dx%d (%s).", width, height, SDL_GetError());
	}

	// Rebuild the 3D renderer if initialized.
	if (this->renderer3D->isInited())
	{
		const Int2 viewDims = this->window->getViewDimensions();
		const double resolutionScale = this->resolutionScaleFunc();
		const Int2 internalRenderDims = MakeInternalRendererDimensions(viewDims, resolutionScale);

		// Reinitialize the game world frame buffer.
		if (this->gameWorldTexture != nullptr)
		{
			SDL_DestroyTexture(this->gameWorldTexture);
		}

		this->gameWorldTexture = SDL_CreateTexture(this->renderer, Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, internalRenderDims.x, internalRenderDims.y);
		if (this->gameWorldTexture == nullptr)
		{
			DebugLogErrorFormat("Couldn't recreate game world texture for resize to %dx%d (%s).", internalRenderDims.x, internalRenderDims.y, SDL_GetError());
		}

		this->renderer3D->resize(internalRenderDims.x, internalRenderDims.y);
	}
}

void Renderer::handleRenderTargetsReset()
{
	if (this->window == nullptr)
	{
		DebugLogError("Missing SDL_Window for render targets reset.");
		return;
	}

	if (this->renderer == nullptr)
	{
		DebugLogError("Missing SDL_Renderer for render targets reset.");
		return;
	}

	if (this->nativeTexture != nullptr)
	{
		SDL_DestroyTexture(this->nativeTexture);
	}

	const Int2 windowDims = this->window->getDimensions();
	this->nativeTexture = SDL_CreateTexture(this->renderer, Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, windowDims.x, windowDims.y);
	if (this->nativeTexture == nullptr)
	{
		DebugLogErrorFormat("Couldn't recreate native frame buffer for render targets reset to %dx%d (%s).", windowDims.x, windowDims.y, SDL_GetError());
	}

	if (this->renderer3D->isInited())
	{
		const Int2 viewDims = this->window->getViewDimensions();
		const double resolutionScale = this->resolutionScaleFunc();
		const Int2 internalRenderDims = MakeInternalRendererDimensions(viewDims, resolutionScale);

		if (this->gameWorldTexture != nullptr)
		{
			SDL_DestroyTexture(this->gameWorldTexture);
		}

		this->gameWorldTexture = SDL_CreateTexture(this->renderer, Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, internalRenderDims.x, internalRenderDims.y);
		if (this->gameWorldTexture == nullptr)
		{
			DebugLogErrorFormat("Couldn't recreate game world texture for render targets reset to %dx%d (%s).", internalRenderDims.x, internalRenderDims.y, SDL_GetError());
		}

		this->renderer3D->resize(internalRenderDims.x, internalRenderDims.y);
	}
}

void Renderer::setClipRect(const SDL_Rect *rect)
{
	if (rect != nullptr)
	{
		// @temp: assume in classic space
		const Rect nativeRect = this->window->originalToNative(Rect(rect->x, rect->y, rect->w, rect->h));
		const SDL_Rect nativeRectSdl = nativeRect.getSdlRect();
		SDL_RenderSetClipRect(this->renderer, &nativeRectSdl);
	}
	else
	{
		SDL_RenderSetClipRect(this->renderer, nullptr);
	}
}

VertexPositionBufferID Renderer::createVertexPositionBuffer(int vertexCount, int componentsPerVertex)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createVertexPositionBuffer(vertexCount, componentsPerVertex);
}

VertexAttributeBufferID Renderer::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createVertexAttributeBuffer(vertexCount, componentsPerVertex);
}

IndexBufferID Renderer::createIndexBuffer(int indexCount)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createIndexBuffer(indexCount);
}

void Renderer::populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateVertexPositionBuffer(id, positions);
}

void Renderer::populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateVertexAttributeBuffer(id, attributes);
}

void Renderer::populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateIndexBuffer(id, indices);
}

void Renderer::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeVertexPositionBuffer(id);
}

void Renderer::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeVertexAttributeBuffer(id);
}

void Renderer::freeIndexBuffer(IndexBufferID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeIndexBuffer(id);
}

ObjectTextureID Renderer::createObjectTexture(int width, int height, int bytesPerTexel)
{
	DebugAssert(this->renderer3D->isInited());
	ObjectTextureAllocator *allocator = this->renderer3D->getTextureAllocator();
	return allocator->create(width, height, bytesPerTexel);
}

ObjectTextureID Renderer::createObjectTexture(const TextureBuilder &textureBuilder)
{
	DebugAssert(this->renderer3D->isInited());
	ObjectTextureAllocator *allocator = this->renderer3D->getTextureAllocator();
	return allocator->create(textureBuilder);
}

UiTextureID Renderer::createUiTexture(int width, int height)
{
	UiTextureAllocator *allocator = this->renderer2D->getTextureAllocator();
	return allocator->create(width, height);
}

UiTextureID Renderer::createUiTexture(Span2D<const uint32_t> texels)
{
	UiTextureAllocator *allocator = this->renderer2D->getTextureAllocator();
	return allocator->create(texels);
}

UiTextureID Renderer::createUiTexture(Span2D<const uint8_t> texels, const Palette &palette)
{
	UiTextureAllocator *allocator = this->renderer2D->getTextureAllocator();
	return allocator->create(texels, palette);
}

UiTextureID Renderer::createUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager)
{
	UiTextureAllocator *allocator = this->renderer2D->getTextureAllocator();
	return allocator->create(textureBuilderID, paletteID, textureManager);
}

std::optional<Int2> Renderer::tryGetObjectTextureDims(ObjectTextureID id) const
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->tryGetObjectTextureDims(id);
}

std::optional<Int2> Renderer::tryGetUiTextureDims(UiTextureID id) const
{
	return this->renderer2D->tryGetTextureDims(id);
}

LockedTexture Renderer::lockObjectTexture(ObjectTextureID id)
{
	DebugAssert(this->renderer3D->isInited());
	ObjectTextureAllocator *allocator = this->renderer3D->getTextureAllocator();
	return allocator->lock(id);
}

LockedTexture Renderer::lockUiTexture(UiTextureID textureID)
{
	UiTextureAllocator *allocator = this->renderer2D->getTextureAllocator();
	return allocator->lock(textureID);
}

void Renderer::unlockObjectTexture(ObjectTextureID id)
{
	DebugAssert(this->renderer3D->isInited());
	ObjectTextureAllocator *allocator = this->renderer3D->getTextureAllocator();
	allocator->unlock(id);
}

void Renderer::unlockUiTexture(UiTextureID textureID)
{
	UiTextureAllocator *allocator = this->renderer2D->getTextureAllocator();
	allocator->unlock(textureID);
}

void Renderer::freeObjectTexture(ObjectTextureID id)
{
	DebugAssert(this->renderer3D->isInited());
	ObjectTextureAllocator *allocator = this->renderer3D->getTextureAllocator();
	allocator->free(id);
}

void Renderer::freeUiTexture(UiTextureID id)
{
	UiTextureAllocator *allocator = this->renderer2D->getTextureAllocator();
	allocator->free(id);
}

UniformBufferID Renderer::createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement)
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createUniformBuffer(elementCount, sizeOfElement, alignmentOfElement);
}

void Renderer::populateUniformBuffer(UniformBufferID id, Span<const std::byte> data)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateUniformBuffer(id, data);
}

void Renderer::populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->populateUniformAtIndex(id, uniformIndex, uniformData);
}

void Renderer::freeUniformBuffer(UniformBufferID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeUniformBuffer(id);
}

RenderLightID Renderer::createLight()
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->createLight();
}

void Renderer::setLightPosition(RenderLightID id, const Double3 &worldPoint)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setLightPosition(id, worldPoint);
}

void Renderer::setLightRadius(RenderLightID id, double startRadius, double endRadius)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setLightRadius(id, startRadius, endRadius);
}

void Renderer::freeLight(RenderLightID id)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->freeLight(id);
}

void Renderer::clear(const Color &color)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderClear(this->renderer);
}

void Renderer::clear()
{
	this->clear(Colors::Black);
}

void Renderer::clearOriginal(const Color &color)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	const SDL_Rect rect = this->window->getLetterboxDimensions();
	SDL_RenderFillRect(this->renderer, &rect);
}

void Renderer::clearOriginal()
{
	this->clearOriginal(Colors::Black);
}

void Renderer::drawPixel(const Color &color, int x, int y)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawPoint(this->renderer, x, y);
}

void Renderer::drawLine(const Color &color, int x1, int y1, int x2, int y2)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(this->renderer, x1, y1, x2, y2);
}

void Renderer::drawRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderDrawRect(this->renderer, &rect);
}

void Renderer::fillRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderFillRect(this->renderer, &rect);
}

void Renderer::fillOriginalRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	const Rect rect = this->window->originalToNative(Rect(x, y, w, h));
	const SDL_Rect rectSdl = rect.getSdlRect();
	SDL_RenderFillRect(this->renderer, &rectSdl);
}

void Renderer::DrawLine(JPH::RVec3Arg src, JPH::RVec3Arg dst, JPH::ColorArg color)
{
	const RenderCamera &camera = g_physicsDebugCamera;
	const Double3 worldPoint0(static_cast<double>(src.GetX()), static_cast<double>(src.GetY()), static_cast<double>(src.GetZ()));
	const Double3 worldPoint1(static_cast<double>(dst.GetX()), static_cast<double>(dst.GetY()), static_cast<double>(dst.GetZ()));
	const double distSqr0 = (camera.worldPoint - worldPoint0).lengthSquared();
	const double distSqr1 = (camera.worldPoint - worldPoint1).lengthSquared();
	if ((distSqr0 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr1 > PHYSICS_DEBUG_MAX_DISTANCE_SQR))
	{
		return;
	}

	const Double4 clipPoint0 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint0, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint1 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint1, 1.0), camera.viewProjMatrix);
	if ((clipPoint0.w <= 0.0) || (clipPoint1.w <= 0.0))
	{
		return;
	}

	const Int2 viewDims = this->window->getViewDimensions();
	const Double3 ndc0 = RendererUtils::clipSpaceToNDC(clipPoint0);
	const Double3 ndc1 = RendererUtils::clipSpaceToNDC(clipPoint1);
	const Double2 screenSpace0 = RendererUtils::ndcToScreenSpace(ndc0, viewDims.x, viewDims.y);
	const Double2 screenSpace1 = RendererUtils::ndcToScreenSpace(ndc1, viewDims.x, viewDims.y);
	const Int2 pixelSpace0(static_cast<int>(screenSpace0.x), static_cast<int>(screenSpace0.y));
	const Int2 pixelSpace1(static_cast<int>(screenSpace1.x), static_cast<int>(screenSpace1.y));

	const double distanceRatio = std::max(distSqr0, distSqr1) / PHYSICS_DEBUG_MAX_DISTANCE_SQR;
	const double intensityPercent = std::clamp(1.0 - (distanceRatio * distanceRatio * distanceRatio), 0.0, 1.0);
	const ColorReal multipliedColor = ColorReal::fromARGB(color.GetUInt32()) * intensityPercent;
	const Color presentedColor = Color::fromARGB(multipliedColor.toARGB());
	this->drawLine(presentedColor, pixelSpace0.x, pixelSpace0.y, pixelSpace1.x, pixelSpace1.y);
}

void Renderer::DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow castShadow)
{
	const RenderCamera &camera = g_physicsDebugCamera;
	const Double3 worldPoint0(static_cast<double>(v1.GetX()), static_cast<double>(v1.GetY()), static_cast<double>(v1.GetZ()));
	const Double3 worldPoint1(static_cast<double>(v2.GetX()), static_cast<double>(v2.GetY()), static_cast<double>(v2.GetZ()));
	const Double3 worldPoint2(static_cast<double>(v3.GetX()), static_cast<double>(v3.GetY()), static_cast<double>(v3.GetZ()));
	const double distSqr0 = (camera.worldPoint - worldPoint0).lengthSquared();
	const double distSqr1 = (camera.worldPoint - worldPoint1).lengthSquared();
	const double distSqr2 = (camera.worldPoint - worldPoint2).lengthSquared();
	if ((distSqr0 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr1 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr2 > PHYSICS_DEBUG_MAX_DISTANCE_SQR))
	{
		return;
	}

	const Double4 clipPoint0 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint0, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint1 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint1, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint2 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint2, 1.0), camera.viewProjMatrix);
	if ((clipPoint0.w <= 0.0) || (clipPoint1.w <= 0.0) || (clipPoint2.w <= 0.0))
	{
		return;
	}

	const Int2 viewDims = this->window->getViewDimensions();
	const Double3 ndc0 = RendererUtils::clipSpaceToNDC(clipPoint0);
	const Double3 ndc1 = RendererUtils::clipSpaceToNDC(clipPoint1);
	const Double3 ndc2 = RendererUtils::clipSpaceToNDC(clipPoint2);
	const Double2 screenSpace0 = RendererUtils::ndcToScreenSpace(ndc0, viewDims.x, viewDims.y);
	const Double2 screenSpace1 = RendererUtils::ndcToScreenSpace(ndc1, viewDims.x, viewDims.y);
	const Double2 screenSpace2 = RendererUtils::ndcToScreenSpace(ndc2, viewDims.x, viewDims.y);
	const Double2 screenSpace01 = screenSpace1 - screenSpace0;
	const Double2 screenSpace12 = screenSpace2 - screenSpace1;
	const Double2 screenSpace20 = screenSpace0 - screenSpace2;
	const double screenSpace01Cross12 = screenSpace12.cross(screenSpace01);
	const double screenSpace12Cross20 = screenSpace20.cross(screenSpace12);
	const double screenSpace20Cross01 = screenSpace01.cross(screenSpace20);

	// Discard back-facing.
	const bool isFrontFacing = (screenSpace01Cross12 + screenSpace12Cross20 + screenSpace20Cross01) > 0.0;
	if (!isFrontFacing)
	{
		return;
	}

	this->DrawLine(v1, v2, color);
	this->DrawLine(v2, v3, color);
	this->DrawLine(v3, v1, color);
}

void Renderer::DrawText3D(JPH::RVec3Arg position, const std::string_view &str, JPH::ColorArg color, float height)
{
	// Do nothing.
}

void Renderer::submitSceneCommands(const RenderCamera &camera, const RenderCommandBuffer &commandBuffer, double ambientPercent,
	Span<const RenderLightID> visibleLightIDs, double screenSpaceAnimPercent, ObjectTextureID paletteTextureID,
	ObjectTextureID lightTableTextureID, ObjectTextureID skyBgTextureID, int renderThreadsMode, DitheringMode ditheringMode)
{
	DebugAssert(this->renderer3D->isInited());

	g_physicsDebugCamera = camera;

	int gameWorldTextureWidth, gameWorldTextureHeight;
	int status = SDL_QueryTexture(this->gameWorldTexture, nullptr, nullptr, &gameWorldTextureWidth, &gameWorldTextureHeight);
	if (status != 0)
	{
		DebugLogErrorFormat("Couldn't query game world texture dimensions for scene rendering (%s).", SDL_GetError());
		return;
	}

	RenderFrameSettings renderFrameSettings;
	renderFrameSettings.init(ambientPercent, visibleLightIDs, screenSpaceAnimPercent, paletteTextureID, lightTableTextureID, skyBgTextureID,
		gameWorldTextureWidth, gameWorldTextureHeight, renderThreadsMode, ditheringMode);

	uint32_t *outputBuffer;
	int gameWorldPitch;
	status = SDL_LockTexture(this->gameWorldTexture, nullptr, reinterpret_cast<void**>(&outputBuffer), &gameWorldPitch);
	if (status != 0)
	{
		DebugLogErrorFormat("Couldn't lock game world texture for scene rendering (%s).", SDL_GetError());
		return;
	}

	// Render the game world (no UI).
	const auto renderStartTime = std::chrono::high_resolution_clock::now();
	this->renderer3D->submitFrame(camera, renderFrameSettings, commandBuffer, outputBuffer);
	const auto renderEndTime = std::chrono::high_resolution_clock::now();
	const double renderTotalTime = static_cast<double>((renderEndTime - renderStartTime).count()) / static_cast<double>(std::nano::den);

	// Update the game world texture with the new pixels and copy to the native frame buffer (stretching if needed).
	const auto presentStartTime = std::chrono::high_resolution_clock::now();
	SDL_UnlockTexture(this->gameWorldTexture);

	const Int2 viewDims = this->window->getViewDimensions();
	this->draw(this->gameWorldTexture, 0, 0, viewDims.x, viewDims.y);
	const auto presentEndTime = std::chrono::high_resolution_clock::now();
	const double presentTotalTime = static_cast<double>((presentEndTime - presentStartTime).count()) / static_cast<double>(std::nano::den);

	// Update profiler stats.
	const Renderer3DProfilerData swProfilerData = this->renderer3D->getProfilerData();
	this->profilerData.init(swProfilerData.width, swProfilerData.height, swProfilerData.threadCount,
		swProfilerData.drawCallCount, swProfilerData.presentedTriangleCount, swProfilerData.textureCount,
		swProfilerData.textureByteCount, swProfilerData.totalLightCount, swProfilerData.totalCoverageTests,
		swProfilerData.totalDepthTests, swProfilerData.totalColorWrites, renderTotalTime, presentTotalTime);
}

void Renderer::submitUiCommands(const UiCommandBuffer &commandBuffer)
{
	const Int2 windowDims = this->window->getDimensions();

	for (int entryIndex = 0; entryIndex < commandBuffer.entryCount; entryIndex++)
	{
		Span<const UiDrawCall> uiDrawCalls = commandBuffer.entries[entryIndex];

		for (const UiDrawCall &drawCall : uiDrawCalls)
		{
			if (!drawCall.activeFunc())
			{
				continue;
			}

			const std::optional<Rect> &optClipRect = drawCall.clipRect;
			if (optClipRect.has_value())
			{
				const SDL_Rect clipRect = optClipRect->getSdlRect();
				this->setClipRect(&clipRect);
			}

			const UiTextureID textureID = drawCall.textureFunc();
			const Int2 position = drawCall.positionFunc();
			const Int2 size = drawCall.sizeFunc();
			const PivotType pivotType = drawCall.pivotFunc();
			const RenderSpace renderSpace = drawCall.renderSpace;

			double xPercent, yPercent, wPercent, hPercent;
			GuiUtils::makeRenderElementPercents(position.x, position.y, size.x, size.y, windowDims.x, windowDims.y,
				renderSpace, pivotType, &xPercent, &yPercent, &wPercent, &hPercent);

			const RenderElement2D renderElement(textureID, xPercent, yPercent, wPercent, hPercent);
			this->draw(&renderElement, 1, renderSpace);

			if (optClipRect.has_value())
			{
				this->setClipRect(nullptr);
			}
		}
	}
}

void Renderer::draw(SDL_Texture *texture, int x, int y, int w, int h)
{
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderCopy(this->renderer, texture, nullptr, &rect);
}

void Renderer::draw(const RenderElement2D *renderElements, int count, RenderSpace renderSpace)
{
	const SDL_Rect letterboxRect = this->window->getLetterboxDimensions();
	this->renderer2D->draw(renderElements, count, renderSpace, Rect(letterboxRect.x, letterboxRect.y, letterboxRect.w, letterboxRect.h));
}

void Renderer::setRenderTargetToFrameBuffer()
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);
}

void Renderer::setRenderTargetToOutput()
{
	SDL_SetRenderTarget(this->renderer, nullptr);
}

void Renderer::present()
{
	this->setRenderTargetToOutput();
	SDL_RenderCopy(this->renderer, this->nativeTexture, nullptr, nullptr);
	SDL_RenderPresent(this->renderer);
}
