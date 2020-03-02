#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <memory>
#include <vector>

#include "SoftwareRenderer.h"
#include "Texture.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../World/LevelData.h"

// Acts as a wrapper for SDL_Renderer operations as well as 3D rendering operations.

// The format for all textures is ARGB8888.

class Color;
class DistantSky;
class EntityManager;
class Palette;
class Rect;
class Surface;
class VoxelGrid;

enum class CursorAlignment;

struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;
struct SDL_Window;

class Renderer
{
public:
	struct DisplayMode
	{
		int width, height, refreshRate;

		DisplayMode(int width, int height, int refreshRate);
	};

	enum class WindowMode
	{
		Window,
		BorderlessFull
	};

	// Profiler information from the most recently rendered frame.
	struct ProfilerData
	{
		// Internal renderer resolution.
		int width, height;

		// Visible flats and lights.
		int visFlatCount, visLightCount;

		double frameTime;

		ProfilerData();
	};
private:
	static const char *DEFAULT_RENDER_SCALE_QUALITY;
	static const char *DEFAULT_TITLE;

	std::vector<DisplayMode> displayModes;
	SDL_Window *window;
	SDL_Renderer *renderer;
	Texture nativeTexture, gameWorldTexture; // Frame buffers.
	SoftwareRenderer softwareRenderer; // Game world renderer.
	ProfilerData profilerData;
	int letterboxMode; // Determines aspect ratio of the original UI (16:10, 4:3, etc.).
	bool fullGameWindow; // Determines height of 3D frame buffer.

	// Helper method for making a renderer context.
	static SDL_Renderer *createRenderer(SDL_Window *window);

	// Generates a renderer dimension while avoiding pitfalls of numeric imprecision.
	static int makeRendererDimension(int value, double resolutionScale);
public:
	// Only defined so members are initialized for Game ctor exception handling.
	Renderer();
	~Renderer();

	// Original screen dimensions.
	static const int ORIGINAL_WIDTH;
	static const int ORIGINAL_HEIGHT;

	// Default bits per pixel.
	static const int DEFAULT_BPP;

	// The default pixel format for all software surfaces, ARGB8888.
	static const uint32_t DEFAULT_PIXELFORMAT;

	// Gets the letterbox aspect associated with the current letterbox mode.
	double getLetterboxAspect() const;

	// Gets the width and height of the active window.
	Int2 getWindowDimensions() const;

	// Gets a list of supported fullscreen display modes.
	const std::vector<DisplayMode> &getDisplayModes() const;

	// Gets the active window's pixels-per-inch scale divided by platform DPI.
	double getDpiScale() const;

	// The "view height" is the height in pixels for the visible game world. This 
	// depends on whether the whole screen is rendered or just the portion above 
	// the interface. The game interface is 53 pixels tall in 320x200.
	int getViewHeight() const;

	// This is for the "letterbox" part of the screen, scaled to fit the window 
	// using the given letterbox aspect.
	SDL_Rect getLetterboxDimensions() const;

	// Gets a screenshot of the current window.
	Surface getScreenshot() const;

	// Gets profiler data (timings, renderer properties, etc.).
	const ProfilerData &getProfilerData() const;

	// Tests whether an entity is intersected by the given ray. Intended for ray cast selection.
	// 'pixelPerfect' determines whether the entity's texture is involved in the calculation.
	// Returns whether the entity was able to be tested and was hit by the ray. This is a renderer
	// function because the exact method of testing may depend on the 3D representation of the entity.
	bool getEntityRayIntersection(const EntityManager::EntityVisibilityData &visData,
		int flatIndex, const Double3 &entityForward, const Double3 &entityRight,
		const Double3 &entityUp, double entityWidth, double entityHeight, const Double3 &rayPoint,
		const Double3 &rayDirection, bool pixelPerfect, Double3 *outHitPoint) const;

	// Converts a [0, 1] screen point to a ray through the world. The exact direction is
	// dependent on renderer details.
	Double3 screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
		double fovY, double aspect) const;

	// Transforms a native window (i.e., 1920x1080) point or rectangle to an original 
	// (320x200) point or rectangle. Points outside the letterbox will either be negative 
	// or outside the 320x200 limit when returned.
	Int2 nativeToOriginal(const Int2 &nativePoint) const;
	Rect nativeToOriginal(const Rect &nativeRect) const;

	// Does the opposite of nativeToOriginal().
	Int2 originalToNative(const Int2 &originalPoint) const;
	Rect originalToNative(const Rect &originalRect) const;

	// Returns true if the letterbox contains a native point.
	bool letterboxContains(const Int2 &nativePoint) const;

	// Wrapper methods for SDL_CreateTexture.
	Texture createTexture(uint32_t format, int access, int w, int h);
	Texture createTextureFromSurface(const Surface &surface);

	void init(int width, int height, WindowMode windowMode, int letterboxMode);

	// Resizes the renderer dimensions.
	void resize(int width, int height, double resolutionScale, bool fullGameWindow);

	// Sets the letterbox mode.
	void setLetterboxMode(int letterboxMode);

	// Sets whether the program is windowed, fullscreen, etc..
	void setWindowMode(WindowMode mode);

	// Sets the window icon to be the given surface.
	void setWindowIcon(const Surface &icon);

	// Sets the window title.
	void setWindowTitle(const char *title);

	// Teleports the mouse to a location in the window.
	void warpMouse(int x, int y);

	// Sets the clip rectangle of the renderer so that pixels outside the specified area
	// will not be rendered. If rect is null, then clipping is disabled.
	void setClipRect(const SDL_Rect *rect);

	// Initialize the renderer for the game world. The "fullGameWindow" argument 
	// determines whether to render a "fullscreen" 3D image or just the part above 
	// the game interface. If there is an existing renderer in memory, it will be 
	// overwritten with the new one.
	void initializeWorldRendering(double resolutionScale, bool fullGameWindow,
		int renderThreadsMode);

	// Sets which mode to use for software render threads (low, medium, high, etc.).
	void setRenderThreadsMode(int mode);

	// Helper methods for changing data in the 3D renderer. Some data, like the voxel
	// grid, are passed each frame by reference.
	// - Some 'add' methods take a unique ID and parameters to create a new object.
	// - 'update' methods take optional parameters for updating, ignoring null ones.
	// - 'remove' methods delete an object from renderer memory if it exists.
	void addLight(int id, const Double3 &point, const Double3 &color, double intensity);
	void updateLight(int id, const Double3 *point, const Double3 *color,
		const double *intensity);
	void setFogDistance(double fogDistance);
	void setVoxelTexture(int id, const uint8_t *srcTexels, const Palette &palette);
	void addFlatTexture(int flatIndex, EntityAnimationData::StateType stateType, int angleID,
		bool flipped, const uint8_t *srcTexels, int width, int height, const Palette &palette);
	void addChasmTexture(VoxelDefinition::ChasmData::Type chasmType, const uint8_t *colors,
		int width, int height, const Palette &palette);
	void setDistantSky(const DistantSky &distantSky, const Palette &palette);
	void setSkyPalette(const uint32_t *colors, int count);
	void setNightLightsActive(bool active);
	void removeLight(int id);
	void clearTextures();
	void clearDistantSky();

	// Fills the native frame buffer with the draw color, or default black/transparent.
	void clear(const Color &color);
	void clear();
	void clearOriginal(const Color &color);
	void clearOriginal();

	// Wrapper methods for some SDL draw functions.
	void drawPixel(const Color &color, int x, int y);
	void drawLine(const Color &color, int x1, int y1, int x2, int y2);
	void drawRect(const Color &color, int x, int y, int w, int h);

	// Wrapper methods for some SDL fill functions.
	void fillRect(const Color &color, int x, int y, int w, int h);
	void fillOriginalRect(const Color &color, int x, int y, int w, int h);

	// Runs the 3D renderer which draws the world onto the native frame buffer.
	// If the renderer is uninitialized, this causes a crash.
	void renderWorld(const Double3 &eye, const Double3 &forward, double fovY, double ambient,
		double daytimePercent, double chasmAnimPercent, double latitude, bool parallaxSky,
		bool nightLightsAreActive, bool isExterior, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels, const VoxelGrid &voxelGrid,
		const EntityManager &entityManager);

	// Draws the given cursor texture to the native frame buffer. The exact position 
	// of the cursor is modified by the cursor alignment.
	void drawCursor(const Texture &texture, CursorAlignment alignment, 
		const Int2 &mousePosition, double scale);

	// Draw methods for the native and original frame buffers.
	void draw(const Texture &texture, int x, int y, int w, int h);
	void draw(const Texture &texture, int x, int y);
	void draw(const Texture &texture);
	void drawClipped(const Texture &texture, const Rect &srcRect, const Rect &dstRect);
	void drawClipped(const Texture &texture, const Rect &srcRect, int x, int y);
	void drawOriginal(const Texture &texture, int x, int y, int w, int h);
	void drawOriginal(const Texture &texture, int x, int y);
	void drawOriginal(const Texture &texture);
	void drawOriginalClipped(const Texture &texture, const Rect &srcRect, const Rect &dstRect);
	void drawOriginalClipped(const Texture &texture, const Rect &srcRect, int x, int y);

	// Stretches a texture over the entire native frame buffer.
	void fill(const Texture &texture);

	// Refreshes the displayed frame buffer.
	void present();
};

#endif
