#ifndef RENDERER_SYSTEM_3D_H
#define RENDERER_SYSTEM_3D_H

#include <cstdint>

#include "RenderGeometryUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"
#include "../Math/MathUtils.h"
#include "../Math/Vector3.h"
#include "../Utilities/Palette.h"

#include "components/utilities/BufferView.h"

// Abstract base class for 3D renderer.

class Random;
class TextureBuilder;

struct RenderCamera;
struct RenderDrawCall;
struct RenderFrameSettings;
struct RenderInitSettings;

class RendererSystem3D
{
public:
	// Profiling info gathered from internal renderer state.
	struct ProfilerData
	{
		int width, height;
		int threadCount;
		int drawCallCount;
		int sceneTriangleCount, visTriangleCount;
		int textureCount;
		int64_t textureByteCount;
		int totalLightCount;

		ProfilerData(int width, int height, int threadCount, int drawCallCount, int sceneTriangleCount,
			int visTriangleCount, int textureCount, int64_t textureByteCount, int totalLightCount);
	};

	virtual ~RendererSystem3D();

	virtual void init(const RenderInitSettings &settings) = 0;
	virtual void shutdown() = 0;

	virtual bool isInited() const = 0;

	virtual void resize(int width, int height) = 0;

	// Geometry management functions.
	virtual bool tryCreateVertexBuffer(int vertexCount, int componentsPerVertex, VertexBufferID *outID) = 0;
	virtual bool tryCreateAttributeBuffer(int vertexCount, int componentsPerVertex, AttributeBufferID *outID) = 0;
	virtual bool tryCreateIndexBuffer(int indexCount, IndexBufferID *outID) = 0;
	virtual void populateVertexBuffer(VertexBufferID id, const BufferView<const double> &vertices) = 0;
	virtual void populateAttributeBuffer(AttributeBufferID id, const BufferView<const double> &attributes) = 0;
	virtual void populateIndexBuffer(IndexBufferID id, const BufferView<const int32_t> &indices) = 0;
	virtual void freeVertexBuffer(VertexBufferID id) = 0;
	virtual void freeAttributeBuffer(AttributeBufferID id) = 0;
	virtual void freeIndexBuffer(IndexBufferID id) = 0;

	// Texture management functions.
	virtual bool tryCreateObjectTexture(int width, int height, int bytesPerTexel, ObjectTextureID *outID) = 0;
	virtual bool tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID) = 0;
	virtual LockedTexture lockObjectTexture(ObjectTextureID id) = 0;
	virtual void unlockObjectTexture(ObjectTextureID id) = 0;
	virtual void freeObjectTexture(ObjectTextureID id) = 0;

	// Shading management functions.
	virtual bool tryCreateLight(RenderLightID *outID) = 0;
	virtual void setLightPosition(RenderLightID id, const Double3 &worldPoint) = 0;
	virtual void setLightRadius(RenderLightID id, double startRadius, double endRadius) = 0;
	virtual void freeLight(RenderLightID id) = 0;

	// Returns the texture's dimensions, if it exists.
	virtual std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const = 0;

	// Gets various profiler information about internal renderer state.
	virtual ProfilerData getProfilerData() const = 0;
	
	// Begins rendering a frame. Currently this is a blocking call and it should be safe to present the frame
	// upon returning from this.
	virtual void submitFrame(const RenderCamera &camera, const BufferView<const RenderDrawCall> &drawCalls,
		const RenderFrameSettings &settings, uint32_t *outputBuffer) = 0;

	// Presents the finished frame to the screen. This may just be a copy to the screen frame buffer that
	// is then taken care of by the top-level rendering manager, since UI must be drawn afterwards.
	virtual void present() = 0;
};

#endif
