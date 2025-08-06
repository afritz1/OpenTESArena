#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "Jolt/Jolt.h"
#include "Jolt/Renderer/DebugRendererSimple.h"
#include "SDL.h"

#include "RenderMeshUtils.h"
#include "RenderLightUtils.h"
#include "RenderTextureUtils.h"
#include "Window.h"
#include "../Assets/TextureUtils.h"
#include "../Math/Vector3.h"
#include "../Utilities/Palette.h"

#include "components/utilities/Span.h"
#include "components/utilities/Span2D.h"

class RenderBackend;
class Surface;
class TextureManager;

enum class CursorAlignment;
enum class RenderBackendType;

struct Rect;
struct RenderCamera;
struct RenderCommandList;
struct RenderFrameSettings;
struct TextureBuilder;
struct UiCommandList;
struct Window;

struct RenderElement2D
{
	UiTextureID id;
	double x, y; // X and Y percents across the render space.
	double width, height; // Percents of render space dimensions.
	// @todo: optional shading/blending parameters? SDL_BlendMode? Alpha percent?

	RenderElement2D(UiTextureID id, double x, double y, double width, double height);
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
class Renderer //: public JPH::DebugRendererSimple
{
private:
	const Window *window;
	std::unique_ptr<RenderBackend> backend;
	RendererProfilerData profilerData;
	RenderResolutionScaleFunc resolutionScaleFunc; // Gets an up-to-date resolution scale value from the game options.
public:
	// Only defined so members are initialized for Game ctor exception handling.
	Renderer();
	~Renderer();

	bool init(const Window *window, RenderBackendType backendType, const RenderResolutionScaleFunc &resolutionScaleFunc,
		int renderThreadsMode, DitheringMode ditheringMode, const std::string &dataFolderPath);

	// Gets a screenshot of the current window.
	Surface getScreenshot() const;

	// Gets profiler data (timings, renderer properties, etc.).
	const RendererProfilerData &getProfilerData() const;

	// Resizes the renderer dimensions.
	void resize(int width, int height);

	// Handles resetting render target textures when switching in and out of exclusive fullscreen.
	void handleRenderTargetsReset();

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

	// Wrapper methods for some SDL draw functions.
	//void drawPixel(const Color &color, int x, int y);
	//void drawLine(const Color &color, int x1, int y1, int x2, int y2);
	//void drawRect(const Color &color, int x, int y, int w, int h);

	// Wrapper methods for some SDL fill functions.
	//void fillRect(const Color &color, int x, int y, int w, int h);

	// Jolt Physics debugging.
	//void DrawLine(JPH::RVec3Arg src, JPH::RVec3Arg dst, JPH::ColorArg color) override;
	//void DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow castShadow) override;
	//void DrawText3D(JPH::RVec3Arg position, const std::string_view &str, JPH::ColorArg color, float height) override;

	// Runs the 3D renderer which draws the world onto the native frame buffer then runs the 2D renderer for UI.
	void submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
		const RenderCamera &camera, const RenderFrameSettings &frameSettings);
};

#endif
