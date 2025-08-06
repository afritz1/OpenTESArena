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
#include "Window.h"
#include "../Assets/TextureUtils.h"

#include "components/utilities/Span.h"

class Surface;
class TextureManager;

enum class CursorAlignment;

struct Color;
struct Rect;
struct SDL_Rect;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;
struct TextureBuilder;
struct UiCommandBuffer;
struct Window;

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
	int64_t totalCoverageTests;
	int64_t totalDepthTests;
	int64_t totalColorWrites;

	double renderTime;
	double presentTime;

	RendererProfilerData();

	void init(int width, int height, int threadCount, int drawCallCount, int presentedTriangleCount, int objectTextureCount,
		int64_t objectTextureByteCount, int totalLightCount, int64_t totalCoverageTests, int64_t totalDepthTests, int64_t totalColorWrites,
		double renderTime, double presentTime);
};

using RenderResolutionScaleFunc = std::function<double()>;

// Manages the active window and 2D and 3D rendering operations.
class Renderer : public JPH::DebugRendererSimple
{
private:
	const Window *window;
	std::unique_ptr<RendererSystem2D> renderer2D;
	std::unique_ptr<RendererSystem3D> renderer3D;
	SDL_Renderer *renderer;
	SDL_Texture *nativeTexture, *gameWorldTexture; // Frame buffers.
	RendererProfilerData profilerData;
	RenderResolutionScaleFunc resolutionScaleFunc; // Gets an up-to-date resolution scale value from the game options.
public:
	// Default bits per pixel.
	static constexpr int DEFAULT_BPP = 32;

	// The default pixel format for all software surfaces, ARGB8888.
	static constexpr uint32_t DEFAULT_PIXELFORMAT = SDL_PIXELFORMAT_ARGB8888;

	// Only defined so members are initialized for Game ctor exception handling.
	Renderer();
	~Renderer();

	bool init(const Window *window, const RenderResolutionScaleFunc &resolutionScaleFunc, RendererSystemType2D systemType2D, RendererSystemType3D systemType3D,
		int renderThreadsMode, DitheringMode ditheringMode, const std::string &dataFolderPath);

	// Gets a screenshot of the current window.
	Surface getScreenshot() const;

	// Gets profiler data (timings, renderer properties, etc.).
	const RendererProfilerData &getProfilerData() const;

	// Resizes the renderer dimensions.
	void resize(int width, int height);

	// Handles resetting render target textures when switching in and out of exclusive fullscreen.
	void handleRenderTargetsReset();

	// Sets the clip rectangle of the renderer so that pixels outside the specified area
	// will not be rendered. If rect is null, then clipping is disabled.
	void setClipRect(const SDL_Rect *rect);

	// Geometry management functions.
	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex);
	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex);
	IndexBufferID createIndexBuffer(int indexCount);
	void populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions);
	void populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes);
	void populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices);
	void freeVertexPositionBuffer(VertexPositionBufferID id);
	void freeVertexAttributeBuffer(VertexAttributeBufferID id);
	void freeIndexBuffer(IndexBufferID id);

	// Texture handle allocation functions.
	ObjectTextureID createObjectTexture(int width, int height, int bytesPerTexel);
	ObjectTextureID createObjectTexture(const TextureBuilder &textureBuilder);
	UiTextureID createUiTexture(int width, int height);
	UiTextureID createUiTexture(Span2D<const uint32_t> texels);
	UiTextureID createUiTexture(Span2D<const uint8_t> texels, const Palette &palette);
	UiTextureID createUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager);

	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const;
	std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const;

	// Allows for updating all texels in the given texture. Must be unlocked to flush the changes.
	LockedTexture lockObjectTexture(ObjectTextureID id);
	LockedTexture lockUiTexture(UiTextureID id);
	void unlockObjectTexture(ObjectTextureID id);
	void unlockUiTexture(UiTextureID id);

	// Texture handle freeing functions.
	void freeObjectTexture(ObjectTextureID id);
	void freeUiTexture(UiTextureID id);

	// Shading management functions.
	UniformBufferID createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement);
	void populateUniformBuffer(UniformBufferID id, Span<const std::byte> data);

	template<typename T>
	void populateUniformBuffer(UniformBufferID id, const T &value)
	{
		Span<const std::byte> valueAsBytes(reinterpret_cast<const std::byte*>(&value), sizeof(value));
		this->populateUniformBuffer(id, valueAsBytes);
	}

	void populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData);

	template<typename T>
	void populateUniformAtIndex(UniformBufferID id, int uniformIndex, const T &value)
	{
		Span<const std::byte> valueAsBytes(reinterpret_cast<const std::byte*>(&value), sizeof(value));
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
	void submitSceneCommands(const RenderCamera &camera, const RenderCommandBuffer &commandBuffer, double ambientPercent,
		Span<const RenderLightID> visibleLightIDs, double screenSpaceAnimPercent, ObjectTextureID paletteTextureID,
		ObjectTextureID lightTableTextureID, ObjectTextureID skyBgTextureID, int renderThreadsMode, DitheringMode ditheringMode);

	// Draws UI onto the screen.
	void submitUiCommands(const UiCommandBuffer &commandBuffer);

	// Draw methods for the native and original frame buffers.
	void draw(SDL_Texture *texture, int x, int y, int w, int h);
	void draw(const RenderElement2D *renderElements, int count, RenderSpace renderSpace);

	// Causes all draw operations to render to a framebuffer that eventually is presented to the screen.
	void setRenderTargetToFrameBuffer();

	// Causes draw operations to render directly to the output window texture.
	void setRenderTargetToOutput();

	// Refreshes the displayed frame buffer.
	void present();
};

#endif
