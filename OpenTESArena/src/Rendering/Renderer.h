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
struct RenderTransform;
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
		int renderThreadsMode, DitheringMode ditheringMode, bool enableValidationLayers, const std::string &dataFolderPath);

	// Gets a screenshot of the current window.
	Surface getScreenshot() const;

	// Gets profiler data (timings, renderer properties, etc.).
	const RendererProfilerData &getProfilerData() const;

	// Resizes the renderer dimensions.
	void resize(int width, int height);

	// Handles resetting render target textures when switching in and out of exclusive fullscreen.
	void handleRenderTargetsReset();

	// Buffer management functions.
	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex);
	void freeVertexPositionBuffer(VertexPositionBufferID id);
	bool populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions);

	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex);
	void freeVertexAttributeBuffer(VertexAttributeBufferID id);
	bool populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes);

	IndexBufferID createIndexBuffer(int indexCount);
	void freeIndexBuffer(IndexBufferID id);
	bool populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices);

	UniformBufferID createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement);
	UniformBufferID createUniformBufferVector3s(int elementCount);
	UniformBufferID createUniformBufferRenderTransforms(int elementCount);
	void freeUniformBuffer(UniformBufferID id);
	bool populateUniformBuffer(UniformBufferID id, Span<const std::byte> bytes);
	bool populateUniformBufferVector3s(UniformBufferID id, Span<const Double3> values);
	bool populateUniformBufferRenderTransforms(UniformBufferID id, Span<const RenderTransform> transforms);
	bool populateUniformBufferIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformBytes);
	bool populateUniformBufferIndexRenderTransform(UniformBufferID id, int uniformIndex, const RenderTransform &transform);

	RenderLightID createLight();
	void freeLight(RenderLightID id);
	bool populateLight(RenderLightID id, const Double3 &point, double startRadius, double endRadius);

	// Texture management functions.
	ObjectTextureID createObjectTexture(int width, int height, int bytesPerTexel);
	void freeObjectTexture(ObjectTextureID id);
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const;
	LockedTexture lockObjectTexture(ObjectTextureID id);
	void unlockObjectTexture(ObjectTextureID id);
	bool populateObjectTexture(ObjectTextureID id, Span<const std::byte> texels);
	bool populateObjectTexture8Bit(ObjectTextureID id, Span<const uint8_t> texels);

	UiTextureID createUiTexture(int width, int height);
	void freeUiTexture(UiTextureID id);
	std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const;
	LockedTexture lockUiTexture(UiTextureID id);
	void unlockUiTexture(UiTextureID id);
	bool populateUiTexture(UiTextureID id, Span<const std::byte> texels, const Palette *palette = nullptr);
	bool populateUiTextureNoPalette(UiTextureID id, Span2D<const uint32_t> texels);

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
