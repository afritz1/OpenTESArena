#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "SDL.h"

#include "ArenaRenderUtils.h"
#include "Renderer.h"
#include "RenderInitSettings.h"
#include "SdlUiRenderer.h"
#include "SoftwareRenderer.h"
#include "../Entities/EntityAnimationInstance.h"
#include "../Interface/CursorAlignment.h"
#include "../Interface/Surface.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Rect.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Utilities/Platform.h"
#include "../World/VoxelGrid.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

Renderer::DisplayMode::DisplayMode(int width, int height, int refreshRate)
{
	this->width = width;
	this->height = height;
	this->refreshRate = refreshRate;
}

Renderer::ProfilerData::ProfilerData()
{
	this->width = -1;
	this->height = -1;
	this->threadCount = -1;
	this->potentiallyVisFlatCount = -1;
	this->visFlatCount = -1;
	this->visLightCount = -1;
	this->frameTime = 0.0;
}

void Renderer::ProfilerData::init(int width, int height, int threadCount, int potentiallyVisFlatCount,
	int visFlatCount, int visLightCount, double frameTime)
{
	this->width = width;
	this->height = height;
	this->threadCount = threadCount;
	this->potentiallyVisFlatCount = potentiallyVisFlatCount;
	this->visFlatCount = visFlatCount;
	this->visLightCount = visLightCount;
	this->frameTime = frameTime;
}

void Renderer::TextureInstance::init(TextureBuilderID textureBuilderID, PaletteID paletteID, Texture &&texture)
{
	this->textureBuilderID = textureBuilderID;
	this->paletteID = paletteID;
	this->texture = std::move(texture);
}

const char *Renderer::DEFAULT_RENDER_SCALE_QUALITY = "nearest";
const char *Renderer::DEFAULT_TITLE = "OpenTESArena";
const int Renderer::DEFAULT_BPP = 32;
const uint32_t Renderer::DEFAULT_PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;

Renderer::Renderer()
{
	DebugAssert(this->nativeTexture.get() == nullptr);
	DebugAssert(this->gameWorldTexture.get() == nullptr);
	this->window = nullptr;
	this->renderer = nullptr;
	this->letterboxMode = 0;
	this->fullGameWindow = false;
}

Renderer::~Renderer()
{
	DebugLog("Closing.");

	SDL_DestroyWindow(this->window);

	// This also destroys the frame buffer textures.
	SDL_DestroyRenderer(this->renderer);
}

SDL_Renderer *Renderer::createRenderer(SDL_Window *window)
{
	// Automatically choose the best driver.
	constexpr int bestDriver = -1;

	SDL_Renderer *rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_ACCELERATED);
	if (rendererContext == nullptr)
	{
		DebugLogError("Couldn't create SDL_Renderer with driver \"" + std::to_string(bestDriver) + "\".");
		return nullptr;
	}

	SDL_RendererInfo rendererInfo;
	if (SDL_GetRendererInfo(rendererContext, &rendererInfo) < 0)
	{
		DebugLogError("Couldn't get SDL_RendererInfo (error: " + std::string(SDL_GetError()) + ").");
		return nullptr;
	}

	const std::string rendererInfoFlags = String::toHexString(rendererInfo.flags);
	DebugLog("Created renderer \"" + std::string(rendererInfo.name) + "\" (flags: 0x" + rendererInfoFlags + ").");

	// Set pixel interpolation hint.
	const SDL_bool status = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, Renderer::DEFAULT_RENDER_SCALE_QUALITY);
	if (status != SDL_TRUE)
	{
		DebugLogWarning("Couldn't set SDL rendering interpolation hint.");
	}

	// Set the size of the render texture to be the size of the whole screen
	// (it automatically scales otherwise).
	const SDL_Surface *nativeSurface = SDL_GetWindowSurface(window);

	// If this fails, we might not support hardware accelerated renderers for some reason
	// (such as with Linux), so we retry with software.
	if (nativeSurface == nullptr)
	{
		DebugLogWarning("Failed to init accelerated SDL_Renderer, trying software fallback.");
		SDL_DestroyRenderer(rendererContext);

		rendererContext = SDL_CreateRenderer(window, bestDriver, SDL_RENDERER_SOFTWARE);
		if (rendererContext == nullptr)
		{
			DebugLogError("Couldn't create software fallback SDL_Renderer.");
			return nullptr;
		}

		nativeSurface = SDL_GetWindowSurface(window);
		if (nativeSurface == nullptr)
		{
			DebugLogError("Couldn't get software fallback SDL_Window surface.");
			return nullptr;
		}
	}

	// Set the device-independent resolution for rendering (i.e., the "behind-the-scenes" resolution).
	SDL_RenderSetLogicalSize(rendererContext, nativeSurface->w, nativeSurface->h);

	return rendererContext;
}

int Renderer::makeRendererDimension(int value, double resolutionScale)
{
	// Make sure renderer dimensions are at least 1x1, and round to make sure an
	// imprecise resolution scale doesn't result in off-by-one resolutions (like 1079p).
	return std::max(static_cast<int>(
		std::round(static_cast<double>(value) * resolutionScale)), 1);
}

std::optional<int> Renderer::tryGetTextureInstanceIndex(TextureBuilderID textureBuilderID, PaletteID paletteID) const
{
	const auto iter = std::find_if(this->textureInstances.begin(), this->textureInstances.end(),
		[textureBuilderID, paletteID](const TextureInstance &textureInst)
	{
		return (textureInst.textureBuilderID == textureBuilderID) && (textureInst.paletteID == paletteID);
	});

	if (iter != this->textureInstances.end())
	{
		return static_cast<int>(std::distance(this->textureInstances.begin(), iter));
	}
	else
	{
		return std::nullopt;
	}
}

void Renderer::addTextureInstance(TextureBuilderID textureBuilderID, PaletteID paletteID,
	const TextureManager &textureManager)
{
	// Texture should not already exist.
	DebugAssert(this->tryGetTextureInstanceIndex(textureBuilderID, paletteID) == std::nullopt);

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);
	const int width = textureBuilder.getWidth();
	const int height = textureBuilder.getHeight();
	Texture texture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (texture.get() == nullptr)
	{
		DebugCrash("Couldn't create texture (texture builder ID: " + std::to_string(textureBuilderID) +
			", palette ID: " + std::to_string(paletteID) + ", dimensions: " + std::to_string(width) + "x" +
			std::to_string(height) + ").");
	}

	// Prepare destination texture for writing.
	uint32_t *dstTexels;
	int pitch;
	if (SDL_LockTexture(texture.get(), nullptr, reinterpret_cast<void**>(&dstTexels), &pitch) != 0)
	{
		DebugCrash("Couldn't lock SDL texture (dimensions: " + std::to_string(width) +
			"x" + std::to_string(height) + ").");
	}

	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		// Convert 8-bit to 32-bit.
		const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
		const Buffer2D<uint8_t> &srcTexels = srcTexture.texels;
		const Palette &palette = textureManager.getPaletteHandle(paletteID);
		std::transform(srcTexels.get(), srcTexels.end(), dstTexels,
			[&palette](const uint8_t srcTexel)
		{
			return palette[srcTexel].toARGB();
		});
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		// Copy from 32-bit.
		const TextureBuilder::TrueColorTexture &srcTexture = textureBuilder.getTrueColor();
		const Buffer2D<uint32_t> &srcTexels = srcTexture.texels;
		std::copy(srcTexels.get(), srcTexels.end(), dstTexels);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(textureBuilderType)));
	}

	SDL_UnlockTexture(texture.get());

	// Set alpha transparency on.
	if (SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND) != 0)
	{
		DebugLogError("Couldn't set SDL texture alpha blending.");
	}

	TextureInstance textureInst;
	textureInst.init(textureBuilderID, paletteID, std::move(texture));
	this->textureInstances.emplace_back(std::move(textureInst));
}

const Texture *Renderer::getOrAddTextureInstance(TextureBuilderID textureBuilderID, PaletteID paletteID,
	const TextureManager &textureManager)
{
	std::optional<int> textureInstIndex = this->tryGetTextureInstanceIndex(textureBuilderID, paletteID);
	if (!textureInstIndex.has_value())
	{
		this->addTextureInstance(textureBuilderID, paletteID, textureManager);
		textureInstIndex = this->tryGetTextureInstanceIndex(textureBuilderID, paletteID);
	}

	if (textureInstIndex.has_value())
	{
		const TextureInstance &textureInst = this->textureInstances[*textureInstIndex];
		return &textureInst.texture;
	}
	else
	{
		return nullptr;
	}
}

double Renderer::getLetterboxAspect() const
{
	if (this->letterboxMode == 0)
	{
		// 16:10.
		return 16.0 / 10.0;
	}
	else if (this->letterboxMode == 1)
	{
		// 4:3.
		return 4.0 / 3.0;
	}
	else if (this->letterboxMode == 2)
	{
		// Stretch to fill.
		const Int2 windowDims = this->getWindowDimensions();
		return static_cast<double>(windowDims.x) / static_cast<double>(windowDims.y);
	}
	else
	{
		DebugUnhandledReturnMsg(double, std::to_string(this->letterboxMode));
	}
}

Int2 Renderer::getWindowDimensions() const
{
	const SDL_Surface *nativeSurface = SDL_GetWindowSurface(this->window);
	return Int2(nativeSurface->w, nativeSurface->h);
}

const std::vector<Renderer::DisplayMode> &Renderer::getDisplayModes() const
{
	return this->displayModes;
}

double Renderer::getDpiScale() const
{
	const double platformDpi = Platform::getDefaultDPI();	
	const int displayIndex = SDL_GetWindowDisplayIndex(this->window);

	float hdpi;
	if (SDL_GetDisplayDPI(displayIndex, nullptr, &hdpi, nullptr) == 0)
	{
		return static_cast<double>(hdpi) / platformDpi;
	}
	else
	{
		DebugLogWarning("Couldn't get DPI of display \"" + std::to_string(displayIndex) + "\".");
		return 1.0;
	}
}

int Renderer::getViewHeight() const
{
	const int screenHeight = this->getWindowDimensions().y;

	// Ratio of the view height and window height in 320x200.
	const double viewWindowRatio = static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT - 53) /
		static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT);

	// Actual view height to use.
	const int viewHeight = this->fullGameWindow ? screenHeight :
		static_cast<int>(std::ceil(screenHeight * viewWindowRatio));

	return viewHeight;
}

SDL_Rect Renderer::getLetterboxDimensions() const
{
	const Int2 windowDims = this->getWindowDimensions();
	const double nativeAspect = static_cast<double>(windowDims.x) /
		static_cast<double>(windowDims.y);
	const double letterboxAspect = this->getLetterboxAspect();

	// Compare the two aspects to decide what the letterbox dimensions are.
	if (std::abs(nativeAspect - letterboxAspect) < Constants::Epsilon)
	{
		// Equal aspects. The letterbox is equal to the screen size.
		SDL_Rect rect;
		rect.x = 0;
		rect.y = 0;
		rect.w = windowDims.x;
		rect.h = windowDims.y;
		return rect;
	}
	else if (nativeAspect > letterboxAspect)
	{
		// Native window is wider = empty left and right.
		const int subWidth = static_cast<int>(std::ceil(
			static_cast<double>(windowDims.y) * letterboxAspect));
		SDL_Rect rect;
		rect.x = (windowDims.x - subWidth) / 2;
		rect.y = 0;
		rect.w = subWidth;
		rect.h = windowDims.y;
		return rect;
	}
	else
	{
		// Native window is taller = empty top and bottom.
		const int subHeight = static_cast<int>(std::ceil(
			static_cast<double>(windowDims.x) / letterboxAspect));
		SDL_Rect rect;
		rect.x = 0;
		rect.y = (windowDims.y - subHeight) / 2;
		rect.w = windowDims.x;
		rect.h = subHeight;
		return rect;
	}
}

Surface Renderer::getScreenshot() const
{
	const Int2 dimensions = this->getWindowDimensions();
	Surface screenshot = Surface::createWithFormat(dimensions.x, dimensions.y,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);

	const int status = SDL_RenderReadPixels(this->renderer, nullptr,
		screenshot.get()->format->format, screenshot.get()->pixels, screenshot.get()->pitch);

	if (status != 0)
	{
		DebugCrash("Couldn't take screenshot, " + std::string(SDL_GetError()));
	}

	return screenshot;
}

const Renderer::ProfilerData &Renderer::getProfilerData() const
{
	return this->profilerData;
}

bool Renderer::getEntityRayIntersection(const EntityManager::EntityVisibilityData &visData,
	const Double3 &entityForward, const Double3 &entityRight, const Double3 &entityUp,
	double entityWidth, double entityHeight, const Double3 &rayPoint, const Double3 &rayDirection,
	bool pixelPerfect, const Palette &palette, Double3 *outHitPoint) const
{
	DebugAssert(this->renderer3D->isInited());
	const Entity &entity = *visData.entity;

	// Do a ray test to see if the ray intersects.
	const NewDouble3 absoluteFlatPosition = VoxelUtils::coordToNewPoint(visData.flatPosition);
	if (MathUtils::rayPlaneIntersection(rayPoint, rayDirection, absoluteFlatPosition,
		entityForward, outHitPoint))
	{
		const Double3 diff = (*outHitPoint) - absoluteFlatPosition;

		// Get the texture coordinates. It's okay if they are outside the entity.
		const Double2 uv(
			0.5 - (diff.dot(entityRight) / entityWidth),
			1.0 - (diff.dot(entityUp) / entityHeight));

		// See if the ray successfully hit a point on the entity, and that point is considered
		// selectable (i.e. it's not transparent).
		bool isSelected;
		const bool withinEntity = this->renderer3D->tryGetEntitySelectionData(uv,
			entity.getRenderID(), visData.stateIndex, visData.angleIndex, visData.keyframeIndex,
			pixelPerfect, palette, &isSelected);

		return withinEntity && isSelected;
	}
	else
	{
		// Did not intersect the entity's plane.
		return false;
	}
}

Double3 Renderer::screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
	double fovY, double aspect) const
{
	return this->renderer3D->screenPointToRay(xPercent, yPercent, cameraDirection, fovY, aspect);
}

Int2 Renderer::nativeToOriginal(const Int2 &nativePoint) const
{
	// From native point to letterbox point.
	const Int2 windowDimensions = this->getWindowDimensions();
	const SDL_Rect letterbox = this->getLetterboxDimensions();

	const Int2 letterboxPoint(
		nativePoint.x - letterbox.x,
		nativePoint.y - letterbox.y);

	// Then from letterbox point to original point.
	const double letterboxXPercent = static_cast<double>(letterboxPoint.x) /
		static_cast<double>(letterbox.w);
	const double letterboxYPercent = static_cast<double>(letterboxPoint.y) /
		static_cast<double>(letterbox.h);

	const double originalWidthReal = static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
	const double originalHeightReal = static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT);

	const Int2 originalPoint(
		static_cast<int>(originalWidthReal * letterboxXPercent),
		static_cast<int>(originalHeightReal * letterboxYPercent));

	return originalPoint;
}

Rect Renderer::nativeToOriginal(const Rect &nativeRect) const
{
	const Int2 newTopLeft = this->nativeToOriginal(nativeRect.getTopLeft());
	const Int2 newBottomRight = this->nativeToOriginal(nativeRect.getBottomRight());
	return Rect(
		newTopLeft.x,
		newTopLeft.y,
		newBottomRight.x - newTopLeft.x,
		newBottomRight.y - newTopLeft.y);
}

Int2 Renderer::originalToNative(const Int2 &originalPoint) const
{
	// From original point to letterbox point.
	const double originalXPercent = static_cast<double>(originalPoint.x) /
		static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
	const double originalYPercent = static_cast<double>(originalPoint.y) /
		static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT);

	const SDL_Rect letterbox = this->getLetterboxDimensions();

	const double letterboxWidthReal = static_cast<double>(letterbox.w);
	const double letterboxHeightReal = static_cast<double>(letterbox.h);

	// Convert to letterbox point. Round to avoid off-by-one errors.
	const Int2 letterboxPoint(
		static_cast<int>(std::round(letterboxWidthReal * originalXPercent)),
		static_cast<int>(std::round(letterboxHeightReal * originalYPercent)));

	// Then from letterbox point to native point.
	const Int2 nativePoint(
		letterboxPoint.x + letterbox.x,
		letterboxPoint.y + letterbox.y);

	return nativePoint;
}

Rect Renderer::originalToNative(const Rect &originalRect) const
{
	const Int2 newTopLeft = this->originalToNative(originalRect.getTopLeft());
	const Int2 newBottomRight = this->originalToNative(originalRect.getBottomRight());
	return Rect(
		newTopLeft.x,
		newTopLeft.y,
		newBottomRight.x - newTopLeft.x,
		newBottomRight.y - newTopLeft.y);
}

bool Renderer::letterboxContains(const Int2 &nativePoint) const
{
	const SDL_Rect letterbox = this->getLetterboxDimensions();
	const Rect rectangle(letterbox.x, letterbox.y,
		letterbox.w, letterbox.h);
	return rectangle.contains(nativePoint);
}

Texture Renderer::createTexture(uint32_t format, int access, int w, int h)
{
	SDL_Texture *tex = SDL_CreateTexture(this->renderer, format, access, w, h);
	if (tex == nullptr)
	{
		DebugLogError("Could not create SDL_Texture.");
	}

	Texture texture;
	texture.init(tex);
	return texture;
}

Texture Renderer::createTextureFromSurface(const Surface &surface)
{
	SDL_Texture *tex = SDL_CreateTextureFromSurface(this->renderer, surface.get());
	if (tex == nullptr)
	{
		DebugLogError("Could not create SDL_Texture from surface.");
	}

	Texture texture;
	texture.init(tex);
	return texture;
}

bool Renderer::init(int width, int height, WindowMode windowMode, int letterboxMode,
	RendererSystemType2D systemType2D, RendererSystemType3D systemType3D)
{
	DebugLog("Initializing.");

	if ((width <= 0) || (height <= 0))
	{
		DebugLogError("Invalid renderer dimensions \"" + std::to_string(width) + "x" + std::to_string(height) + "\"");
		return false;
	}

	this->letterboxMode = letterboxMode;

	// Initialize window. The SDL_Surface is obtained from this window.
	this->window = [width, height, windowMode]()
	{
		const char *title = Renderer::DEFAULT_TITLE;
		const int position = [windowMode]() -> int
		{
			switch (windowMode)
			{
			case WindowMode::Window:
				return SDL_WINDOWPOS_CENTERED;
			case WindowMode::BorderlessFull:
				return SDL_WINDOWPOS_UNDEFINED;
			default:
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(windowMode)));
			}
		}();

		const uint32_t flags = SDL_WINDOW_RESIZABLE |
			((windowMode == WindowMode::BorderlessFull) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
			SDL_WINDOW_ALLOW_HIGHDPI;

		// If fullscreen is true, then width and height are ignored. They are stored
		// behind the scenes for when the user changes to windowed mode, however.
		return SDL_CreateWindow(title, position, position, width, height, flags);
	}();

	if (this->window == nullptr)
	{
		DebugLogError("Couldn't create SDL_Window (dimensions: " + std::to_string(width) + "x" + std::to_string(height) +
			", window mode: " + std::to_string(static_cast<int>(windowMode)) + ").");
		return false;
	}

	// Initialize renderer context.
	this->renderer = Renderer::createRenderer(this->window);
	if (this->renderer == nullptr)
	{
		DebugLogError("Couldn't create SDL_Renderer.");
		return false;
	}

	// Initialize display modes list for the current window.
	const int displayIndex = SDL_GetWindowDisplayIndex(this->window);
	const int displayModeCount = SDL_GetNumDisplayModes(displayIndex);
	for (int i = 0; i < displayModeCount; i++)
	{
		// Convert SDL display mode to our display mode.
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(displayIndex, i, &mode) == 0)
		{
			// Filter away non-24-bit displays. Perhaps this could be handled better, but I don't
			// know how to do that for all possible displays out there.
			if (mode.format == SDL_PIXELFORMAT_RGB888)
			{
				this->displayModes.emplace_back(DisplayMode(mode.w, mode.h, mode.refresh_rate));
			}
		}
	}

	// Use window dimensions, just in case it's fullscreen and the given width and
	// height are ignored.
	const Int2 windowDimensions = this->getWindowDimensions();

	// Initialize native frame buffer.
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, windowDimensions.x, windowDimensions.y);
	if (this->nativeTexture.get() == nullptr)
	{
		DebugLogError("Couldn't create SDL_Texture frame buffer (error: " + std::string(SDL_GetError()) + ").");
		return false;
	}

	// Initialize 2D renderer resources.
	this->renderer2D = [systemType2D]() -> std::unique_ptr<RendererSystem2D>
	{
		if (systemType2D == RendererSystemType2D::SDL2)
		{
			return std::make_unique<SdlUiRenderer>();
		}
		else
		{
			DebugLogError("Unrecognized 2D renderer system type \"" +
				std::to_string(static_cast<int>(systemType2D)) + "\".");
			return nullptr;
		}
	}();

	// Initialize 3D renderer resources.
	this->renderer3D = [systemType3D]() -> std::unique_ptr<RendererSystem3D>
	{
		if (systemType3D == RendererSystemType3D::SoftwareClassic)
		{
			return std::make_unique<SoftwareRenderer>();
		}
		else
		{
			DebugLogError("Unrecognized 3D renderer system type \"" +
				std::to_string(static_cast<int>(systemType3D)) + "\".");
			return nullptr;
		}
	}();

	// Don't initialize the game world buffer until the 3D renderer is initialized.
	DebugAssert(this->gameWorldTexture.get() == nullptr);
	this->fullGameWindow = false;

	return true;
}

void Renderer::resize(int width, int height, double resolutionScale, bool fullGameWindow)
{
	// The window's dimensions are resized automatically by SDL. The renderer's are not.
	const Int2 windowDims = this->getWindowDimensions();
	DebugAssertMsg(windowDims.x == width, "Mismatched resize widths.");
	DebugAssertMsg(windowDims.y == height, "Mismatched resize heights.");

	SDL_RenderSetLogicalSize(this->renderer, width, height);

	// Reinitialize native frame buffer.
	this->nativeTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_TARGET, width, height);
	DebugAssertMsg(this->nativeTexture.get() != nullptr,
		"Couldn't recreate native frame buffer, " + std::string(SDL_GetError()));

	this->fullGameWindow = fullGameWindow;

	// Rebuild the 3D renderer if initialized.
	if (this->renderer3D->isInited())
	{
		// Height of the game world view in pixels. Determined by whether the game 
		// interface is visible or not.
		const int viewHeight = this->getViewHeight();

		const int renderWidth = Renderer::makeRendererDimension(width, resolutionScale);
		const int renderHeight = Renderer::makeRendererDimension(viewHeight, resolutionScale);

		// Reinitialize the game world frame buffer.
		this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
			SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
		DebugAssertMsg(this->gameWorldTexture.get() != nullptr,
			"Couldn't recreate game world texture, " + std::string(SDL_GetError()));

		this->renderer3D->resize(renderWidth, renderHeight);
	}
}

void Renderer::setLetterboxMode(int letterboxMode)
{
	this->letterboxMode = letterboxMode;
}

void Renderer::setWindowMode(WindowMode mode)
{
	const uint32_t flags = [mode]() -> uint32_t
	{
		// Use fake fullscreen for now.
		switch (mode)
		{
		case WindowMode::Window:
			return 0;
		case WindowMode::BorderlessFull:
			return SDL_WINDOW_FULLSCREEN_DESKTOP;
		default:
			DebugUnhandledReturnMsg(uint32_t, std::to_string(static_cast<int>(mode)));
		}
	}();

	SDL_SetWindowFullscreen(this->window, flags);

	// Reset the cursor to the center of the screen for consistency.
	const Int2 windowDims = this->getWindowDimensions();
	this->warpMouse(windowDims.x / 2, windowDims.y / 2);
}

void Renderer::setWindowIcon(const Surface &icon)
{
	SDL_SetWindowIcon(this->window, icon.get());
}

void Renderer::setWindowTitle(const char *title)
{
	SDL_SetWindowTitle(this->window, title);
}

void Renderer::warpMouse(int x, int y)
{
	SDL_WarpMouseInWindow(this->window, x, y);
}

void Renderer::setClipRect(const SDL_Rect *rect)
{
	SDL_RenderSetClipRect(this->renderer, rect);
}

void Renderer::initializeWorldRendering(double resolutionScale, bool fullGameWindow,
	int renderThreadsMode)
{
	this->fullGameWindow = fullGameWindow;

	const int screenWidth = this->getWindowDimensions().x;

	// Height of the game world view in pixels, used in place of the screen height.
	// Its value is a function of whether the game interface is visible or not.
	const int viewHeight = this->getViewHeight();

	// Make sure render dimensions are at least 1x1.
	const int renderWidth = Renderer::makeRendererDimension(screenWidth, resolutionScale);
	const int renderHeight = Renderer::makeRendererDimension(viewHeight, resolutionScale);

	// Initialize a new game world frame buffer, removing any previous game world frame buffer.
	this->gameWorldTexture = this->createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STREAMING, renderWidth, renderHeight);
	DebugAssertMsg(this->gameWorldTexture.get() != nullptr,
		"Couldn't create game world texture, " + std::string(SDL_GetError()));

	// Initialize 3D rendering.
	RenderInitSettings initSettings;
	initSettings.init(renderWidth, renderHeight, renderThreadsMode);
	this->renderer3D->init(initSettings);
}

void Renderer::setRenderThreadsMode(int mode)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setRenderThreadsMode(mode);
}

bool Renderer::tryCreateVoxelTexture(const TextureAssetReference &textureAssetRef, TextureManager &textureManager)
{
	return this->renderer3D->tryCreateVoxelTexture(textureAssetRef, textureManager);
}

bool Renderer::tryCreateEntityTexture(const TextureAssetReference &textureAssetRef, TextureManager &textureManager)
{
	return this->renderer3D->tryCreateEntityTexture(textureAssetRef, textureManager);
}

bool Renderer::tryCreateSkyTexture(const TextureAssetReference &textureAssetRef, TextureManager &textureManager)
{
	return this->renderer3D->tryCreateSkyTexture(textureAssetRef, textureManager);
}

bool Renderer::tryCreateUiTexture(const TextureAssetReference &textureAssetRef, TextureManager &textureManager)
{
	return this->renderer2D->tryCreateUiTexture(textureAssetRef, textureManager);
}

void Renderer::freeVoxelTexture(const TextureAssetReference &textureAssetRef)
{
	this->renderer3D->freeVoxelTexture(textureAssetRef);
}

void Renderer::freeEntityTexture(const TextureAssetReference &textureAssetRef)
{
	this->renderer3D->freeEntityTexture(textureAssetRef);
}

void Renderer::freeSkyTexture(const TextureAssetReference &textureAssetRef)
{
	this->renderer3D->freeSkyTexture(textureAssetRef);
}

void Renderer::freeUiTexture(const TextureAssetReference &textureAssetRef)
{
	this->renderer2D->freeUiTexture(textureAssetRef);
}

void Renderer::setFogDistance(double fogDistance)
{
	this->renderer3D->setFogDistance(fogDistance);
}

EntityRenderID Renderer::makeEntityRenderID()
{
	DebugAssert(this->renderer3D->isInited());
	return this->renderer3D->makeEntityRenderID();
}

void Renderer::setFlatTextures(EntityRenderID entityRenderID, const EntityAnimationDefinition &animDef,
	const EntityAnimationInstance &animInst, bool isPuddle, TextureManager &textureManager)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setFlatTextures(entityRenderID, animDef, animInst, isPuddle, textureManager);
}

void Renderer::addChasmTexture(ArenaTypes::ChasmType chasmType, const uint8_t *colors,
	int width, int height, const Palette &palette)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->addChasmTexture(chasmType, colors, width, height, palette);
}

void Renderer::setDistantSky(const DistantSky &distantSky, const Palette &palette,
	TextureManager &textureManager)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setDistantSky(distantSky, palette, textureManager);
}

void Renderer::setSkyPalette(const uint32_t *colors, int count)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setSkyPalette(colors, count);
}

void Renderer::setNightLightsActive(bool active, const Palette &palette)
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->setNightLightsActive(active, palette);
}

void Renderer::clearTexturesAndEntityRenderIDs()
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->clearTexturesAndEntityRenderIDs();
}

void Renderer::clearDistantSky()
{
	DebugAssert(this->renderer3D->isInited());
	this->renderer3D->clearDistantSky();
}

void Renderer::clear(const Color &color)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderClear(this->renderer);
}

void Renderer::clear()
{
	this->clear(Color::Black);
}

void Renderer::clearOriginal(const Color &color)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	const SDL_Rect rect = this->getLetterboxDimensions();
	SDL_RenderFillRect(this->renderer, &rect);
}

void Renderer::clearOriginal()
{
	this->clearOriginal(Color::Black);
}

void Renderer::drawPixel(const Color &color, int x, int y)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawPoint(this->renderer, x, y);
}

void Renderer::drawLine(const Color &color, int x1, int y1, int x2, int y2)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(this->renderer, x1, y1, x2, y2);
}

void Renderer::drawRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
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
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
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
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	const Rect rect = this->originalToNative(Rect(x, y, w, h));
	SDL_RenderFillRect(this->renderer, &rect.getRect());
}

void Renderer::renderWorld(const CoordDouble3 &eye, const Double3 &forward, double fovY, double ambient,
	double daytimePercent, double chasmAnimPercent, double latitude, bool nightLightsAreActive, bool isExterior,
	bool playerHasLight, int chunkDistance, double ceilingHeight, const LevelData &levelData,
	const EntityDefinitionLibrary &entityDefLibrary, const Palette &palette)
{
	// The 3D renderer must be initialized.
	DebugAssert(this->renderer3D->isInited());
	
	// Lock the game world texture and give the pixel pointer to the software renderer.
	// - Supposedly this is faster than SDL_UpdateTexture(). In any case, there's one
	//   less frame buffer to take care of.
	uint32_t *gameWorldPixels;
	int gameWorldPitch;
	int status = SDL_LockTexture(this->gameWorldTexture.get(), nullptr,
		reinterpret_cast<void**>(&gameWorldPixels), &gameWorldPitch);
	DebugAssertMsg(status == 0, "Couldn't lock game world texture, " + std::string(SDL_GetError()));

	// Render the game world to the game world frame buffer.
	const auto startTime = std::chrono::high_resolution_clock::now();
	this->renderer3D->render(eye, forward, fovY, ambient, daytimePercent, chasmAnimPercent, latitude,
		nightLightsAreActive, isExterior, playerHasLight, chunkDistance, ceilingHeight, levelData,
		entityDefLibrary, palette, gameWorldPixels);
	const auto endTime = std::chrono::high_resolution_clock::now();
	const double frameTime = static_cast<double>((endTime - startTime).count()) / static_cast<double>(std::nano::den);

	// Update profiler stats.
	const RendererSystem3D::ProfilerData swProfilerData = this->renderer3D->getProfilerData();
	this->profilerData.init(swProfilerData.width, swProfilerData.height, swProfilerData.threadCount,
		swProfilerData.potentiallyVisFlatCount, swProfilerData.visFlatCount, swProfilerData.visLightCount,
		frameTime);

	// Update the game world texture with the new ARGB8888 pixels.
	SDL_UnlockTexture(this->gameWorldTexture.get());

	// Now copy to the native frame buffer (stretching if needed).
	const int screenWidth = this->getWindowDimensions().x;
	const int viewHeight = this->getViewHeight();
	this->draw(this->gameWorldTexture, 0, 0, screenWidth, viewHeight);
}

void Renderer::drawCursor(TextureBuilderID textureBuilderID, PaletteID paletteID, CursorAlignment alignment,
	const Int2 &mousePosition, double scale, const TextureManager &textureManager)
{
	const Texture *cursor = this->getOrAddTextureInstance(textureBuilderID, paletteID, textureManager);
	if (cursor == nullptr)
	{
		DebugLogError("Couldn't draw cursor (texture builder ID: " + std::to_string(textureBuilderID) +
			", palette ID: " + std::to_string(paletteID) + ").");
		return;
	}

	const int scaledWidth = static_cast<int>(std::round(cursor->getWidth() * scale));
	const int scaledHeight = static_cast<int>(std::round(cursor->getHeight() * scale));

	// Get the magnitude to offset the cursor's coordinates by.
	const Int2 cursorOffset = [alignment, scaledWidth, scaledHeight]()
	{
		const int xOffset = [alignment, scaledWidth]()
		{
			if ((alignment == CursorAlignment::TopLeft) ||
				(alignment == CursorAlignment::Left) ||
				(alignment == CursorAlignment::BottomLeft))
			{
				return 0;
			}
			else if ((alignment == CursorAlignment::Top) ||
				(alignment == CursorAlignment::Middle) ||
				(alignment == CursorAlignment::Bottom))
			{
				return scaledWidth / 2;
			}
			else
			{
				return scaledWidth - 1;
			}
		}();

		const int yOffset = [alignment, scaledHeight]()
		{
			if ((alignment == CursorAlignment::TopLeft) ||
				(alignment == CursorAlignment::Top) ||
				(alignment == CursorAlignment::TopRight))
			{
				return 0;
			}
			else if ((alignment == CursorAlignment::Left) ||
				(alignment == CursorAlignment::Middle) ||
				(alignment == CursorAlignment::Right))
			{
				return scaledHeight / 2;
			}
			else
			{
				return scaledHeight - 1;
			}
		}();

		return Int2(xOffset, yOffset);
	}();

	const int cursorX = mousePosition.x - cursorOffset.x;
	const int cursorY = mousePosition.y - cursorOffset.y;
	this->draw(*cursor, cursorX, cursorY, scaledWidth, scaledHeight);
}

void Renderer::draw(const Texture &texture, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderCopy(this->renderer, texture.get(), nullptr, &rect);
}

void Renderer::draw(const Texture &texture, int x, int y)
{
	int width, height;
	SDL_QueryTexture(texture.get(), nullptr, nullptr, &width, &height);

	this->draw(texture, x, y, width, height);
}

void Renderer::draw(const Texture &texture)
{
	this->draw(texture, 0, 0);
}

void Renderer::drawClipped(const Texture &texture, const Rect &srcRect, const Rect &dstRect)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_RenderCopy(this->renderer, texture.get(), &srcRect.getRect(), &dstRect.getRect());
}

void Renderer::drawClipped(const Texture &texture, const Rect &srcRect, int x, int y)
{
	this->drawClipped(texture, srcRect, Rect(x, y, srcRect.getWidth(), srcRect.getHeight()));
}

void Renderer::drawOriginal(const Texture &texture, int x, int y, int w, int h)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	
	// The given coordinates and dimensions are in 320x200 space, so transform them
	// to native space.
	const Rect rect = this->originalToNative(Rect(x, y, w, h));

	SDL_RenderCopy(this->renderer, texture.get(), nullptr, &rect.getRect());
}

void Renderer::drawOriginal(const Texture &texture, int x, int y)
{
	int width, height;
	SDL_QueryTexture(texture.get(), nullptr, nullptr, &width, &height);

	this->drawOriginal(texture, x, y, width, height);
}

void Renderer::drawOriginal(const Texture &texture)
{
	this->drawOriginal(texture, 0, 0);
}

void Renderer::drawOriginal(TextureBuilderID textureBuilderID, PaletteID paletteID, int x, int y, int w, int h,
	const TextureManager &textureManager)
{
	const Texture *texture = this->getOrAddTextureInstance(textureBuilderID, paletteID, textureManager);
	if (texture == nullptr)
	{
		DebugLogError("Couldn't get texture (texture builder ID: " + std::to_string(textureBuilderID) +
			", palette ID: " + std::to_string(paletteID) + ").");
		return;
	}

	this->drawOriginal(*texture, x, y, w, h);
}

void Renderer::drawOriginal(TextureBuilderID textureBuilderID, PaletteID paletteID, int x, int y,
	const TextureManager &textureManager)
{
	const Texture *texture = this->getOrAddTextureInstance(textureBuilderID, paletteID, textureManager);
	if (texture == nullptr)
	{
		DebugLogError("Couldn't get texture (texture builder ID: " + std::to_string(textureBuilderID) +
			", palette ID: " + std::to_string(paletteID) + ").");
		return;
	}

	this->drawOriginal(*texture, x, y);
}

void Renderer::drawOriginal(TextureBuilderID textureBuilderID, PaletteID paletteID,
	const TextureManager &textureManager)
{
	constexpr int x = 0;
	constexpr int y = 0;
	this->drawOriginal(textureBuilderID, paletteID, x, y, textureManager);
}

void Renderer::drawOriginalClipped(const Texture &texture, const Rect &srcRect, const Rect &dstRect)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());

	// The destination coordinates and dimensions are in 320x200 space, so transform 
	// them to native space.
	const Rect rect = this->originalToNative(dstRect);

	SDL_RenderCopy(this->renderer, texture.get(), &srcRect.getRect(), &rect.getRect());
}

void Renderer::drawOriginalClipped(const Texture &texture, const Rect &srcRect, int x, int y)
{
	this->drawOriginalClipped(texture, srcRect, 
		Rect(x, y, srcRect.getWidth(), srcRect.getHeight()));
}

void Renderer::drawOriginalClipped(TextureBuilderID textureBuilderID, PaletteID paletteID, const Rect &srcRect,
	const Rect &dstRect, const TextureManager &textureManager)
{
	const Texture *texture = this->getOrAddTextureInstance(textureBuilderID, paletteID, textureManager);
	if (texture == nullptr)
	{
		DebugLogError("Couldn't get texture (texture builder ID: " + std::to_string(textureBuilderID) +
			", palette ID: " + std::to_string(paletteID) + ").");
		return;
	}

	this->drawOriginalClipped(*texture, srcRect, dstRect);
}

void Renderer::drawOriginalClipped(TextureBuilderID textureBuilderID, PaletteID paletteID, const Rect &srcRect,
	int x, int y, const TextureManager &textureManager)
{
	this->drawOriginalClipped(textureBuilderID, paletteID, srcRect,
		Rect(x, y, srcRect.getWidth(), srcRect.getHeight()), textureManager);
}

void Renderer::fill(const Texture &texture)
{
	SDL_SetRenderTarget(this->renderer, this->nativeTexture.get());
	SDL_RenderCopy(this->renderer, texture.get(), nullptr, nullptr);
}

void Renderer::present()
{
	SDL_SetRenderTarget(this->renderer, nullptr);
	SDL_RenderCopy(this->renderer, this->nativeTexture.get(), nullptr, nullptr);
	SDL_RenderPresent(this->renderer);
}
