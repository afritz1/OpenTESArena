#ifndef RENDERER_SYSTEM_3D_H
#define RENDERER_SYSTEM_3D_H

#include <cstdint>

#include "RenderLightUtils.h"
#include "RenderMeshUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"
#include "../Math/MathUtils.h"
#include "../Math/Vector3.h"
#include "../Utilities/Palette.h"

#include "components/utilities/Span.h"

class Random;

struct RenderCamera;
struct RenderCommandBuffer;
struct RenderDrawCall;
struct RenderFrameSettings;
struct RenderInitSettings;
struct TextureBuilder;

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

// Abstract base class for 3D renderer.
class RendererSystem3D
{
public:
	virtual ~RendererSystem3D();

	virtual void init(const RenderInitSettings &settings) = 0;
	virtual void shutdown() = 0;

	virtual bool isInited() const = 0;

	virtual void resize(int width, int height) = 0;

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
	virtual ObjectTextureID createObjectTexture(int width, int height, int bytesPerTexel) = 0;
	virtual ObjectTextureID createObjectTexture(const TextureBuilder &textureBuilder) = 0;
	virtual LockedTexture lockObjectTexture(ObjectTextureID id) = 0;
	virtual void unlockObjectTexture(ObjectTextureID id) = 0;
	virtual void freeObjectTexture(ObjectTextureID id) = 0;

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

	// Returns the texture's dimensions, if it exists.
	virtual std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const = 0;

	// Gets various profiler information about internal renderer state.
	virtual Renderer3DProfilerData getProfilerData() const = 0;
	
	// Begins rendering a frame. Currently this is a blocking call and it should be safe to present the frame
	// upon returning from this.
	virtual void submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings,
		const RenderCommandBuffer &commandBuffer, uint32_t *outputBuffer) = 0;

	// Presents the finished frame to the screen. This may just be a copy to the screen frame buffer that
	// is then taken care of by the top-level rendering manager, since UI must be drawn afterwards.
	virtual void present() = 0;
};

#endif
