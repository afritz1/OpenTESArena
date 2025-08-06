#include <chrono>

#include "SDL_hints.h"
#include "SDL_render.h"

#include "RenderCamera.h"
#include "RenderCommand.h"
#include "RenderInitSettings.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "Sdl2DSoft3DRenderBackend.h"
#include "Window.h"
#include "../Math/Rect.h"
#include "../UI/GuiUtils.h"
#include "../UI/Surface.h"
#include "../UI/UiCommand.h"
#include "../UI/UiDrawCall.h"

#include "components/debug/Debug.h"

namespace
{
	RenderCamera g_physicsDebugCamera; // Cached every frame for Jolt Physics debug renderer.

	constexpr const char RenderScaleQualityHint[] = "nearest";

	SDL_Renderer *CreateSdlRendererForWindow(SDL_Window *window)
	{
		// Automatically choose the best driver. Generally Direct3D on Windows, OpenGL on Linux, Metal on macOS.
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

		const SDL_bool status = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, RenderScaleQualityHint);
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
}

Sdl2DSoft3DRenderBackend::Sdl2DSoft3DRenderBackend()
{
	this->renderer = nullptr;
	this->nativeTexture = nullptr;
	this->gameWorldTexture = nullptr;
}

Sdl2DSoft3DRenderBackend::~Sdl2DSoft3DRenderBackend()
{
	DebugAssert(this->window == nullptr);
	DebugAssert(this->renderer == nullptr);
	DebugAssert(this->nativeTexture == nullptr);
	DebugAssert(this->gameWorldTexture == nullptr);
}

bool Sdl2DSoft3DRenderBackend::init(const RenderInitSettings &initSettings)
{
	this->window = initSettings.window;

	this->renderer = CreateSdlRendererForWindow(this->window->window);
	if (this->renderer == nullptr)
	{
		DebugLogErrorFormat("Couldn't create SDL_Renderer (%s).", SDL_GetError());
		return false;
	}

	const Int2 windowDims = this->window->getDimensions();
	this->nativeTexture = SDL_CreateTexture(this->renderer, RendererUtils::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, windowDims.x, windowDims.y);
	if (this->nativeTexture == nullptr)
	{
		DebugLogErrorFormat("Couldn't create SDL_Texture frame buffer (%s).", SDL_GetError());
		return false;
	}

	this->gameWorldTexture = SDL_CreateTexture(this->renderer, RendererUtils::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, initSettings.internalWidth, initSettings.internalHeight);
	if (this->gameWorldTexture == nullptr)
	{
		DebugLogErrorFormat("Couldn't create game world texture with dimensions %dx%d (%s).", initSettings.internalWidth, initSettings.internalHeight, SDL_GetError());
	}

	this->renderer2D.init(this->window->window);
	this->renderer3D.init(initSettings);

	return true;
}

void Sdl2DSoft3DRenderBackend::shutdown()
{
	this->renderer2D.shutdown();
	this->renderer3D.shutdown();

	if (this->renderer != nullptr)
	{
		// This also destroys the frame buffer textures.
		SDL_DestroyRenderer(this->renderer);

		this->gameWorldTexture = nullptr;
		this->nativeTexture = nullptr;
		this->renderer = nullptr;
	}
	
	this->window = nullptr;
}

void Sdl2DSoft3DRenderBackend::resize(int windowWidth, int windowHeight, int internalWidth, int internalHeight)
{
	SDL_RenderSetLogicalSize(this->renderer, windowWidth, windowHeight);

	if (this->nativeTexture != nullptr)
	{
		SDL_DestroyTexture(this->nativeTexture);
	}

	this->nativeTexture = SDL_CreateTexture(this->renderer, RendererUtils::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, windowWidth, windowHeight);
	if (this->nativeTexture == nullptr)
	{
		DebugLogErrorFormat("Couldn't recreate native frame buffer for window resize to %dx%d (%s).", windowWidth, windowHeight, SDL_GetError());
	}

	if (this->gameWorldTexture != nullptr)
	{
		SDL_DestroyTexture(this->gameWorldTexture);
	}

	this->gameWorldTexture = SDL_CreateTexture(this->renderer, RendererUtils::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, internalWidth, internalHeight);
	if (this->gameWorldTexture == nullptr)
	{
		DebugLogErrorFormat("Couldn't recreate game world texture for internal resize to %dx%d (%s).", internalWidth, internalHeight, SDL_GetError());
	}

	this->renderer3D.resize(internalWidth, internalHeight);
}

void Sdl2DSoft3DRenderBackend::handleRenderTargetsReset(int windowWidth, int windowHeight, int internalWidth, int internalHeight)
{
	if (this->renderer == nullptr)
	{
		DebugLogError("Missing SDL_Renderer for render targets reset.");
		return;
	}

	if (this->nativeTexture != nullptr)
	{
		SDL_DestroyTexture(this->nativeTexture);
	}

	this->nativeTexture = SDL_CreateTexture(this->renderer, RendererUtils::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_TARGET, windowWidth, windowHeight);
	if (this->nativeTexture == nullptr)
	{
		DebugLogErrorFormat("Couldn't recreate native frame buffer for render targets reset to %dx%d (%s).", windowWidth, windowHeight, SDL_GetError());
	}

	if (this->gameWorldTexture != nullptr)
	{
		SDL_DestroyTexture(this->gameWorldTexture);
	}

	this->gameWorldTexture = SDL_CreateTexture(this->renderer, RendererUtils::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, internalWidth, internalHeight);
	if (this->gameWorldTexture == nullptr)
	{
		DebugLogErrorFormat("Couldn't recreate game world texture for render targets reset to %dx%d (%s).", internalWidth, internalHeight, SDL_GetError());
	}

	this->renderer3D.resize(internalWidth, internalHeight);
}

VertexPositionBufferID Sdl2DSoft3DRenderBackend::createVertexPositionBuffer(int vertexCount, int componentsPerVertex)
{
	return this->renderer3D.createVertexPositionBuffer(vertexCount, componentsPerVertex);
}

VertexAttributeBufferID Sdl2DSoft3DRenderBackend::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex)
{
	return this->renderer3D.createVertexAttributeBuffer(vertexCount, componentsPerVertex);
}

IndexBufferID Sdl2DSoft3DRenderBackend::createIndexBuffer(int indexCount)
{
	return this->renderer3D.createIndexBuffer(indexCount);
}

void Sdl2DSoft3DRenderBackend::populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions)
{
	this->renderer3D.populateVertexPositionBuffer(id, positions);
}

void Sdl2DSoft3DRenderBackend::populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes)
{
	this->renderer3D.populateVertexAttributeBuffer(id, attributes);
}

void Sdl2DSoft3DRenderBackend::populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices)
{
	this->renderer3D.populateIndexBuffer(id, indices);
}

void Sdl2DSoft3DRenderBackend::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	this->renderer3D.freeVertexPositionBuffer(id);
}

void Sdl2DSoft3DRenderBackend::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	this->renderer3D.freeVertexAttributeBuffer(id);
}

void Sdl2DSoft3DRenderBackend::freeIndexBuffer(IndexBufferID id)
{
	this->renderer3D.freeIndexBuffer(id);
}

ObjectTextureAllocator *Sdl2DSoft3DRenderBackend::getObjectTextureAllocator()
{
	return this->renderer3D.getTextureAllocator();
}

UiTextureAllocator *Sdl2DSoft3DRenderBackend::getUiTextureAllocator()
{
	return this->renderer2D.getTextureAllocator();
}

std::optional<Int2> Sdl2DSoft3DRenderBackend::tryGetObjectTextureDims(ObjectTextureID id) const
{
	return this->renderer3D.tryGetTextureDims(id);
}

std::optional<Int2> Sdl2DSoft3DRenderBackend::tryGetUiTextureDims(UiTextureID id) const
{
	return this->renderer2D.tryGetTextureDims(id);
}

UniformBufferID Sdl2DSoft3DRenderBackend::createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement)
{
	return this->renderer3D.createUniformBuffer(elementCount, sizeOfElement, alignmentOfElement);
}

void Sdl2DSoft3DRenderBackend::populateUniformBuffer(UniformBufferID id, Span<const std::byte> data)
{
	return this->renderer3D.populateUniformBuffer(id, data);
}

void Sdl2DSoft3DRenderBackend::populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData)
{
	return this->renderer3D.populateUniformAtIndex(id, uniformIndex, uniformData);
}

void Sdl2DSoft3DRenderBackend::freeUniformBuffer(UniformBufferID id)
{
	return this->renderer3D.freeUniformBuffer(id);
}

RenderLightID Sdl2DSoft3DRenderBackend::createLight()
{
	return this->renderer3D.createLight();
}

void Sdl2DSoft3DRenderBackend::setLightPosition(RenderLightID id, const Double3 &worldPoint)
{
	return this->renderer3D.setLightPosition(id, worldPoint);
}

void Sdl2DSoft3DRenderBackend::setLightRadius(RenderLightID id, double startRadius, double endRadius)
{
	return this->renderer3D.setLightRadius(id, startRadius, endRadius);
}

void Sdl2DSoft3DRenderBackend::freeLight(RenderLightID id)
{
	return this->renderer3D.freeLight(id);
}

Renderer3DProfilerData Sdl2DSoft3DRenderBackend::getProfilerData() const
{
	return this->renderer3D.getProfilerData();
}

Surface Sdl2DSoft3DRenderBackend::getScreenshot() const
{
	const Int2 windowDims = this->window->getDimensions();
	Surface screenshot = Surface::createWithFormat(windowDims.x, windowDims.y, RendererUtils::DEFAULT_BPP, RendererUtils::DEFAULT_PIXELFORMAT);
	SDL_Surface *screenshotSurface = screenshot.get();

	const int status = SDL_RenderReadPixels(this->renderer, nullptr, screenshotSurface->format->format, screenshotSurface->pixels, screenshotSurface->pitch);
	if (status != 0)
	{
		DebugLogErrorFormat("Couldn't apply SDL_RenderReadPixels() to screenshot (%s).", SDL_GetError());
	}

	return screenshot;
}

void Sdl2DSoft3DRenderBackend::submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
	const RenderCamera &camera, const RenderFrameSettings &frameSettings)
{
	g_physicsDebugCamera = camera;
	
	SDL_SetRenderTarget(this->renderer, this->nativeTexture);

	constexpr Color clearColor = Colors::Black;
	SDL_SetRenderDrawColor(this->renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	SDL_RenderClear(this->renderer);

	int gameWorldTextureWidth, gameWorldTextureHeight;
	int status = SDL_QueryTexture(this->gameWorldTexture, nullptr, nullptr, &gameWorldTextureWidth, &gameWorldTextureHeight);
	if (status != 0)
	{
		DebugLogErrorFormat("Couldn't query game world texture dimensions for scene rendering (%s).", SDL_GetError());
		return;
	}

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
	if (renderCommandList.entryCount > 0)
	{
		this->renderer3D.submitFrame(camera, frameSettings, renderCommandList, outputBuffer);
	}
	
	const auto renderEndTime = std::chrono::high_resolution_clock::now();
	const double renderTotalTime = static_cast<double>((renderEndTime - renderStartTime).count()) / static_cast<double>(std::nano::den);

	// Update the game world texture with the new pixels and copy to the native frame buffer (stretching if needed).
	const auto presentStartTime = std::chrono::high_resolution_clock::now();
	SDL_UnlockTexture(this->gameWorldTexture);

	const Int2 viewDims = this->window->getViewDimensions();

	SDL_Rect gameWorldDrawRect;
	gameWorldDrawRect.x = 0;
	gameWorldDrawRect.y = 0;
	gameWorldDrawRect.w = viewDims.x;
	gameWorldDrawRect.h = viewDims.y;
	SDL_RenderCopy(this->renderer, this->gameWorldTexture, nullptr, &gameWorldDrawRect);

	const auto presentEndTime = std::chrono::high_resolution_clock::now();
	const double presentTotalTime = static_cast<double>((presentEndTime - presentStartTime).count()) / static_cast<double>(std::nano::den);

	const Int2 windowDims = this->window->getDimensions();
	const Rect letterboxRect = this->window->getLetterboxRect();

	// Sets the clip rectangle of the renderer so that pixels outside the specified area
	// will not be rendered. If rect is null, then clipping is disabled.
	auto setClipRect = [this](const SDL_Rect *rect)
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
	};

	for (int entryIndex = 0; entryIndex < uiCommandList.entryCount; entryIndex++)
	{
		Span<const UiDrawCall> uiDrawCalls = uiCommandList.entries[entryIndex];

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
				setClipRect(&clipRect);
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
			this->renderer2D.draw(&renderElement, 1, renderSpace, letterboxRect);

			if (optClipRect.has_value())
			{
				setClipRect(nullptr);
			}
		}
	}

	SDL_SetRenderTarget(this->renderer, nullptr);
	SDL_RenderCopy(this->renderer, this->nativeTexture, nullptr, nullptr);
	SDL_RenderPresent(this->renderer);

	// @todo include some more times in here for UI rendering and final present, then update in Renderer
	// Update profiler stats.
	/*const Renderer3DProfilerData swProfilerData = this->backend->getProfilerData();
	this->profilerData.init(swProfilerData.width, swProfilerData.height, swProfilerData.threadCount,
		swProfilerData.drawCallCount, swProfilerData.presentedTriangleCount, swProfilerData.textureCount,
		swProfilerData.textureByteCount, swProfilerData.totalLightCount, swProfilerData.totalCoverageTests,
		swProfilerData.totalDepthTests, swProfilerData.totalColorWrites, renderTotalTime, presentTotalTime);*/
}
