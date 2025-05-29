#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "Jolt/Jolt.h"
#include "Jolt/Renderer/DebugRendererSimple.h"
#include "SDL.h"

#include "RendererSystem2D.h"
#include "RendererSystem3D.h"
#include "RendererSystemType.h"
#include "RenderLightUtils.h"
#include "../Assets/TextureUtils.h"
#include "../UI/Texture.h"

#include "components/utilities/BufferView.h"

class Surface;
class TextureManager;

enum class CursorAlignment;

struct Color;
struct Rect;
struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;
struct SDL_Window;

struct RenderDisplayMode
{
	int width, height, refreshRate;

	RenderDisplayMode(int width, int height, int refreshRate);
};

enum class RenderWindowMode
{
	Window,
	BorderlessFullscreen,
	ExclusiveFullscreen
};

// Profiler information from the most recently rendered frame.
struct RendererProfilerData
{
	// Internal renderer resolution.
	int width, height;
	int pixelCount;

	int threadCount;
	int drawCallCount;

	// Geometry.
	int presentedTriangleCount; // After clipping, only screen-space triangles with onscreen area.

	// Textures.
	int objectTextureCount;
	int64_t objectTextureByteCount;

	// Lights.
	int totalLightCount;

	// Pixel writes/overdraw.
	int totalCoverageTests;
	int totalDepthTests;
	int totalColorWrites;

	double renderTime;
	double presentTime;

	RendererProfilerData();

	void init(int width, int height, int threadCount, int drawCallCount, int presentedTriangleCount, int objectTextureCount,
		int64_t objectTextureByteCount, int totalLightCount, int totalCoverageTests, int totalDepthTests, int totalColorWrites,
		double renderTime, double presentTime);
};

using RenderResolutionScaleFunc = std::function<double()>;

// Manages the active window and 2D and 3D rendering operations.
class Renderer : public JPH::DebugRendererSimple
{
private:
	std::unique_ptr<RendererSystem2D> renderer2D;
	std::unique_ptr<RendererSystem3D> renderer3D;
	std::vector<RenderDisplayMode> displayModes;
	SDL_Window *window;
	SDL_Renderer *renderer;
	Texture nativeTexture, gameWorldTexture; // Frame buffers.
	RendererProfilerData profilerData;
	RenderResolutionScaleFunc resolutionScaleFunc; // Gets an up-to-date resolution scale value from the game options.
	int letterboxMode; // Determines aspect ratio of the original UI (16:10, 4:3, etc.).
	bool fullGameWindow; // Determines height of 3D frame buffer.
public:
	// Only defined so members are initialized for Game ctor exception handling.
	Renderer();
	~Renderer();

	// Default bits per pixel.
	static constexpr int DEFAULT_BPP = 32;

	// The default pixel format for all software surfaces, ARGB8888.
	static constexpr uint32_t DEFAULT_PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;

	// Gets the letterbox aspect associated with the current letterbox mode.
	double getLetterboxAspect() const;

	// Gets the width and height of the active window.
	Int2 getWindowDimensions() const;

	// Gets the aspect ratio of the active window.
	double getWindowAspect() const;

	// Gets a list of supported fullscreen display modes.
	BufferView<const RenderDisplayMode> getDisplayModes() const;

	// Gets the active window's pixels-per-inch scale divided by platform DPI.
	double getDpiScale() const;

	// The "view height" is the height in pixels for the visible game world. This 
	// depends on whether the whole screen is rendered or just the portion above 
	// the interface. The game interface is 53 pixels tall in 320x200.
	Int2 getViewDimensions() const;
	double getViewAspect() const;

	// This is for the "letterbox" part of the screen, scaled to fit the window 
	// using the given letterbox aspect.
	SDL_Rect getLetterboxDimensions() const;

	// Gets a screenshot of the current window.
	Surface getScreenshot() const;

	// Gets profiler data (timings, renderer properties, etc.).
	const RendererProfilerData &getProfilerData() const;

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

	bool init(int width, int height, RenderWindowMode windowMode, int letterboxMode, bool fullGameWindow,
		const RenderResolutionScaleFunc &resolutionScaleFunc, RendererSystemType2D systemType2D,
		RendererSystemType3D systemType3D, int renderThreadsMode, DitheringMode ditheringMode);

	// Resizes the renderer dimensions.
	void resize(int width, int height, double resolutionScale, bool fullGameWindow);

	// Handles resetting render target textures when switching in and out of exclusive fullscreen.
	void handleRenderTargetsReset();

	// Sets the letterbox mode.
	void setLetterboxMode(int letterboxMode);

	// Sets whether the program is windowed, fullscreen, etc..
	void setWindowMode(RenderWindowMode mode);

	// Sets the window icon to be the given surface.
	void setWindowIcon(const Surface &icon);

	// Sets the window title.
	void setWindowTitle(const char *title);

	// Teleports the mouse to a location in the window.
	void warpMouse(int x, int y);

	// Sets the clip rectangle of the renderer so that pixels outside the specified area
	// will not be rendered. If rect is null, then clipping is disabled.
	void setClipRect(const SDL_Rect *rect);

	// Geometry management functions.
	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex);
	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex);
	IndexBufferID createIndexBuffer(int indexCount);
	void populateVertexPositionBuffer(VertexPositionBufferID id, BufferView<const double> positions);
	void populateVertexAttributeBuffer(VertexAttributeBufferID id, BufferView<const double> attributes);
	void populateIndexBuffer(IndexBufferID id, BufferView<const int32_t> indices);
	void freeVertexPositionBuffer(VertexPositionBufferID id);
	void freeVertexAttributeBuffer(VertexAttributeBufferID id);
	void freeIndexBuffer(IndexBufferID id);

	// Texture handle allocation functions.
	ObjectTextureID createObjectTexture(int width, int height, int bytesPerTexel);
	ObjectTextureID createObjectTexture(const TextureBuilder &textureBuilder);
	bool tryCreateUiTexture(int width, int height, UiTextureID *outID);
	bool tryCreateUiTexture(BufferView2D<const uint32_t> texels, UiTextureID *outID);
	bool tryCreateUiTexture(BufferView2D<const uint8_t> texels, const Palette &palette, UiTextureID *outID);
	bool tryCreateUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID,
		const TextureManager &textureManager, UiTextureID *outID);

	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const;
	std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const;

	// Allows for updating all texels in the given texture. Must be unlocked to flush the changes.
	LockedTexture lockObjectTexture(ObjectTextureID id);
	uint32_t *lockUiTexture(UiTextureID id);
	void unlockObjectTexture(ObjectTextureID id);
	void unlockUiTexture(UiTextureID id);

	// Texture handle freeing functions.
	void freeObjectTexture(ObjectTextureID id);
	void freeUiTexture(UiTextureID id);

	// Shading management functions.
	UniformBufferID createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement);
	void populateUniformBuffer(UniformBufferID id, BufferView<const std::byte> data);

	template<typename T>
	void populateUniformBuffer(UniformBufferID id, const T &value)
	{
		BufferView<const std::byte> valueAsBytes(reinterpret_cast<const std::byte*>(&value), sizeof(value));
		this->populateUniformBuffer(id, valueAsBytes);
	}

	void populateUniformAtIndex(UniformBufferID id, int uniformIndex, BufferView<const std::byte> uniformData);

	template<typename T>
	void populateUniformAtIndex(UniformBufferID id, int uniformIndex, const T &value)
	{
		BufferView<const std::byte> valueAsBytes(reinterpret_cast<const std::byte*>(&value), sizeof(value));
		this->populateUniformAtIndex(id, uniformIndex, valueAsBytes);
	}

	void freeUniformBuffer(UniformBufferID id);
	RenderLightID createLight();
	void setLightPosition(RenderLightID id, const Double3 &worldPoint);
	void setLightRadius(RenderLightID id, double startRadius, double endRadius);
	void freeLight(RenderLightID id);

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

	// Jolt Physics debugging.
	void DrawLine(JPH::RVec3Arg src, JPH::RVec3Arg dst, JPH::ColorArg color) override;
	void DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow castShadow) override;
	void DrawText3D(JPH::RVec3Arg position, const std::string_view &str, JPH::ColorArg color, float height) override;

	// Runs the 3D renderer which draws the world onto the native frame buffer.
	void submitFrame(const RenderCamera &camera, const RenderCommandBuffer &commandBuffer, double ambientPercent,
		double screenSpaceAnimPercent, ObjectTextureID paletteTextureID, ObjectTextureID lightTableTextureID,
		ObjectTextureID skyBgTextureID, int renderThreadsMode, DitheringMode ditheringMode);

	// Draw methods for the native and original frame buffers.
	void draw(const Texture &texture, int x, int y, int w, int h);
	void draw(const RendererSystem2D::RenderElement *renderElements, int count, RenderSpace renderSpace);

	// Refreshes the displayed frame buffer.
	void present();
};

#endif
