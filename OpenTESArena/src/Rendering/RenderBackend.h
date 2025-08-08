#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

#include <cstddef>
#include <cstdint>
#include <optional>

#include "RenderLightUtils.h"
#include "RenderMeshUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"
#include "../Math/Vector3.h"

#include "components/utilities/Span.h"

class Surface;

struct ObjectTextureAllocator;
struct RenderCamera;
struct RenderCommandList;
struct RenderFrameSettings;
struct RenderInitSettings;
struct UiCommandList;
struct UiTextureAllocator;

// Profiling info gathered from internal renderer state.
struct Renderer3DProfilerData
{
	int width, height;
	int threadCount;
	int drawCallCount;
	int presentedTriangleCount;
	int textureCount;
	int64_t textureByteCount;
	int totalLightCount;
	int64_t totalCoverageTests;
	int64_t totalDepthTests;
	int64_t totalColorWrites;

	Renderer3DProfilerData(int width, int height, int threadCount, int drawCallCount, int presentedTriangleCount,
		int textureCount, int64_t textureByteCount, int totalLightCount, int64_t totalCoverageTests, int64_t totalDepthTests, int64_t totalColorWrites);
};

// Abstract base supporting several implementations (software + SDL_Texture, Vulkan, etc.).
class RenderBackend
{
public:
	virtual bool init(const RenderInitSettings &initSettings) = 0;
	virtual void shutdown() = 0;

	virtual void resize(int windowWidth, int windowHeight, int internalWidth, int internalHeight) = 0;
	virtual void handleRenderTargetsReset(int windowWidth, int windowHeight, int internalWidth, int internalHeight) = 0;

	// Geometry management functions.
	virtual VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex) = 0;
	virtual VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex) = 0;
	virtual IndexBufferID createIndexBuffer(int indexCount) = 0;
	virtual void populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions) = 0;
	virtual void populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes) = 0;
	virtual void populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices) = 0;
	virtual void freeVertexPositionBuffer(VertexPositionBufferID id) = 0;
	virtual void freeVertexAttributeBuffer(VertexAttributeBufferID id) = 0;
	virtual void freeIndexBuffer(IndexBufferID id) = 0;

	// Texture management functions.
	virtual ObjectTextureAllocator *getObjectTextureAllocator() = 0;
	virtual UiTextureAllocator *getUiTextureAllocator() = 0;
	virtual std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const = 0;
	virtual std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const = 0;

	// Uniform management functions.
	virtual UniformBufferID createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement) = 0;
	virtual void populateUniformBuffer(UniformBufferID id, Span<const std::byte> data) = 0;
	virtual void populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData) = 0;
	virtual void freeUniformBuffer(UniformBufferID id) = 0;

	// Light management functions.
	virtual RenderLightID createLight() = 0;
	virtual void setLightPosition(RenderLightID id, const Double3 &worldPoint) = 0;
	virtual void setLightRadius(RenderLightID id, double startRadius, double endRadius) = 0;
	virtual void freeLight(RenderLightID id) = 0;

	// Gets various profiler information about internal renderer state.
	virtual Renderer3DProfilerData getProfilerData() const = 0;

	virtual Surface getScreenshot() const = 0;

	// Renders a frame to the target window. Currently this is blocking and should be safe to present the frame upon returning.
	virtual void submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
		const RenderCamera &camera, const RenderFrameSettings &frameSettings) = 0;
};

#endif
