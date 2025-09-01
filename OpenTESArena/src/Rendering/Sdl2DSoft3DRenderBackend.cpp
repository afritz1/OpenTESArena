#include "SDL_hints.h"
#include "SDL_render.h"

#include "RenderBuffer.h"
#include "RenderCamera.h"
#include "RenderCommand.h"
#include "RenderFrameSettings.h"
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
	this->window = nullptr;
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

void Sdl2DSoft3DRenderBackend::resize(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight)
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

void Sdl2DSoft3DRenderBackend::handleRenderTargetsReset(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight)
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

RendererProfilerData2D Sdl2DSoft3DRenderBackend::getProfilerData2D() const
{
	return this->renderer2D.getProfilerData();
}

RendererProfilerData3D Sdl2DSoft3DRenderBackend::getProfilerData3D() const
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

int Sdl2DSoft3DRenderBackend::getBytesPerFloat() const
{
	return this->renderer3D.getBytesPerFloat();
}

VertexPositionBufferID Sdl2DSoft3DRenderBackend::createVertexPositionBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent)
{
	return this->renderer3D.createVertexPositionBuffer(vertexCount, componentsPerVertex, bytesPerComponent);
}

void Sdl2DSoft3DRenderBackend::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	this->renderer3D.freeVertexPositionBuffer(id);
}

LockedBuffer Sdl2DSoft3DRenderBackend::lockVertexPositionBuffer(VertexPositionBufferID id)
{
	return this->renderer3D.lockVertexPositionBuffer(id);
}

void Sdl2DSoft3DRenderBackend::unlockVertexPositionBuffer(VertexPositionBufferID id)
{
	this->renderer3D.unlockVertexPositionBuffer(id);
}

VertexAttributeBufferID Sdl2DSoft3DRenderBackend::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent)
{
	return this->renderer3D.createVertexAttributeBuffer(vertexCount, componentsPerVertex, bytesPerComponent);
}

void Sdl2DSoft3DRenderBackend::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	this->renderer3D.freeVertexAttributeBuffer(id);
}

LockedBuffer Sdl2DSoft3DRenderBackend::lockVertexAttributeBuffer(VertexAttributeBufferID id)
{
	return this->renderer3D.lockVertexAttributeBuffer(id);
}

void Sdl2DSoft3DRenderBackend::unlockVertexAttributeBuffer(VertexAttributeBufferID id)
{
	this->renderer3D.unlockVertexAttributeBuffer(id);
}

IndexBufferID Sdl2DSoft3DRenderBackend::createIndexBuffer(int indexCount, int bytesPerIndex)
{
	return this->renderer3D.createIndexBuffer(indexCount, bytesPerIndex);
}

void Sdl2DSoft3DRenderBackend::freeIndexBuffer(IndexBufferID id)
{
	this->renderer3D.freeIndexBuffer(id);
}

LockedBuffer Sdl2DSoft3DRenderBackend::lockIndexBuffer(IndexBufferID id)
{
	return this->renderer3D.lockIndexBuffer(id);
}

void Sdl2DSoft3DRenderBackend::unlockIndexBuffer(IndexBufferID id)
{
	this->renderer3D.unlockIndexBuffer(id);
}

UniformBufferID Sdl2DSoft3DRenderBackend::createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement)
{
	return this->renderer3D.createUniformBuffer(elementCount, bytesPerElement, alignmentOfElement);
}

void Sdl2DSoft3DRenderBackend::freeUniformBuffer(UniformBufferID id)
{
	return this->renderer3D.freeUniformBuffer(id);
}

LockedBuffer Sdl2DSoft3DRenderBackend::lockUniformBuffer(UniformBufferID id)
{
	return this->renderer3D.lockUniformBuffer(id);
}

LockedBuffer Sdl2DSoft3DRenderBackend::lockUniformBufferIndex(UniformBufferID id, int index)
{
	return this->renderer3D.lockUniformBufferIndex(id, index);
}

void Sdl2DSoft3DRenderBackend::unlockUniformBuffer(UniformBufferID id)
{
	return this->renderer3D.unlockUniformBuffer(id);
}

void Sdl2DSoft3DRenderBackend::unlockUniformBufferIndex(UniformBufferID id, int index)
{
	return this->renderer3D.unlockUniformBufferIndex(id, index);
}

ObjectTextureID Sdl2DSoft3DRenderBackend::createObjectTexture(int width, int height, int bytesPerTexel)
{
	return this->renderer3D.createTexture(width, height, bytesPerTexel);
}

void Sdl2DSoft3DRenderBackend::freeObjectTexture(ObjectTextureID id)
{
	this->renderer3D.freeTexture(id);
}

std::optional<Int2> Sdl2DSoft3DRenderBackend::tryGetObjectTextureDims(ObjectTextureID id) const
{
	return this->renderer3D.tryGetTextureDims(id);
}

LockedTexture Sdl2DSoft3DRenderBackend::lockObjectTexture(ObjectTextureID id)
{
	return this->renderer3D.lockTexture(id);
}

void Sdl2DSoft3DRenderBackend::unlockObjectTexture(ObjectTextureID id)
{
	this->renderer3D.unlockTexture(id);
}

UiTextureID Sdl2DSoft3DRenderBackend::createUiTexture(int width, int height)
{
	return this->renderer2D.createTexture(width, height);
}

void Sdl2DSoft3DRenderBackend::freeUiTexture(UiTextureID id)
{
	this->renderer2D.freeTexture(id);
}

std::optional<Int2> Sdl2DSoft3DRenderBackend::tryGetUiTextureDims(UiTextureID id) const
{
	return this->renderer2D.tryGetTextureDims(id);
}

LockedTexture Sdl2DSoft3DRenderBackend::lockUiTexture(UiTextureID id)
{
	return this->renderer2D.lockTexture(id);
}

void Sdl2DSoft3DRenderBackend::unlockUiTexture(UiTextureID id)
{
	this->renderer2D.unlockTexture(id);
}

RenderMaterialID Sdl2DSoft3DRenderBackend::createMaterial(RenderMaterialKey key)
{
	return this->renderer3D.createMaterial(key);
}

void Sdl2DSoft3DRenderBackend::freeMaterial(RenderMaterialID id)
{
	this->renderer3D.freeMaterial(id);
}

void Sdl2DSoft3DRenderBackend::setMaterialParameterMeshLightingPercent(RenderMaterialID id, double value)
{
	this->renderer3D.setMaterialParameterMeshLightingPercent(id, value);
}

void Sdl2DSoft3DRenderBackend::setMaterialParameterPixelShaderParam(RenderMaterialID id, double value)
{
	this->renderer3D.setMaterialParameterPixelShaderParam(id, value);
}

void Sdl2DSoft3DRenderBackend::submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
	const RenderCamera &camera, const RenderFrameSettings &frameSettings)
{
	g_physicsDebugCamera = camera;

	SDL_SetRenderTarget(this->renderer, this->nativeTexture);

	const Color clearColor = frameSettings.clearColor;
	SDL_SetRenderDrawColor(this->renderer, clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	SDL_RenderClear(this->renderer);

	// Render the game world (no UI).
	if (renderCommandList.entryCount > 0)
	{
		uint32_t *outputBuffer;
		int gameWorldPitch;
		int status = SDL_LockTexture(this->gameWorldTexture, nullptr, reinterpret_cast<void**>(&outputBuffer), &gameWorldPitch);
		if (status != 0)
		{
			DebugLogErrorFormat("Couldn't lock game world texture for scene rendering (%s).", SDL_GetError());
			return;
		}

		this->renderer3D.submitFrame(renderCommandList, camera, frameSettings, outputBuffer);
		SDL_UnlockTexture(this->gameWorldTexture);

		const Int2 viewDims = this->window->getSceneViewDimensions();

		SDL_Rect gameWorldDrawRect;
		gameWorldDrawRect.x = 0;
		gameWorldDrawRect.y = 0;
		gameWorldDrawRect.w = viewDims.x;
		gameWorldDrawRect.h = viewDims.y;
		SDL_RenderCopy(this->renderer, this->gameWorldTexture, nullptr, &gameWorldDrawRect);
	}

	for (int entryIndex = 0; entryIndex < uiCommandList.entryCount; entryIndex++)
	{
		Span<const RenderElement2D> uiRenderElements = uiCommandList.entries[entryIndex];
		this->renderer2D.draw(uiRenderElements);
	}

	SDL_SetRenderTarget(this->renderer, nullptr);
	SDL_RenderCopy(this->renderer, this->nativeTexture, nullptr, nullptr);
	SDL_RenderPresent(this->renderer);
}
