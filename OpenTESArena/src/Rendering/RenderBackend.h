#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

#include <cstddef>
#include <cstdint>
#include <optional>

#include "RenderLightUtils.h"
#include "RenderMaterialUtils.h"
#include "RenderMeshUtils.h"
#include "RenderShaderUtils.h"
#include "RenderTextureUtils.h"
#include "../Math/Vector3.h"

#include "components/utilities/Span.h"

class Surface;

struct LockedBuffer;
struct RenderCamera;
struct RenderCommandList;
struct RenderFrameSettings;
struct RenderInitSettings;
struct UiCommandList;

// Profiling info gathered from internal renderer state.
struct RendererProfilerData2D
{
	int drawCallCount;
	int uiTextureCount;
	int64_t uiTextureByteCount;

	RendererProfilerData2D();
};

struct RendererProfilerData3D
{
	int width, height;
	int threadCount;
	int drawCallCount;
	int presentedTriangleCount;
	int objectTextureCount;
	int64_t objectTextureByteCount;
	int totalLightCount;
	int64_t totalCoverageTests;
	int64_t totalDepthTests;
	int64_t totalColorWrites;

	RendererProfilerData3D();
};

// Abstract base supporting several implementations (software + SDL_Texture, Vulkan, etc.).
class RenderBackend
{
public:
	virtual bool init(const RenderInitSettings &initSettings) = 0;
	virtual void shutdown() = 0;

	virtual void resize(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight) = 0;
	virtual void handleRenderTargetsReset(int windowWidth, int windowHeight, int sceneViewWidth, int sceneViewHeight, int internalWidth, int internalHeight) = 0;

	// Gets various profiler information about internal renderer state.
	virtual RendererProfilerData2D getProfilerData2D() const = 0;
	virtual RendererProfilerData3D getProfilerData3D() const = 0;

	virtual Surface getScreenshot() const = 0;

	virtual int getBytesPerFloat() const = 0;

	// Buffer management functions.
	virtual VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent) = 0;
	virtual void freeVertexPositionBuffer(VertexPositionBufferID id) = 0;
	virtual LockedBuffer lockVertexPositionBuffer(VertexPositionBufferID id) = 0;
	virtual void unlockVertexPositionBuffer(VertexPositionBufferID id) = 0;

	virtual VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent) = 0;
	virtual void freeVertexAttributeBuffer(VertexAttributeBufferID id) = 0;
	virtual LockedBuffer lockVertexAttributeBuffer(VertexAttributeBufferID id) = 0;
	virtual void unlockVertexAttributeBuffer(VertexAttributeBufferID id) = 0;

	virtual IndexBufferID createIndexBuffer(int indexCount, int bytesPerIndex) = 0;
	virtual void freeIndexBuffer(IndexBufferID id) = 0;
	virtual LockedBuffer lockIndexBuffer(IndexBufferID id) = 0;
	virtual void unlockIndexBuffer(IndexBufferID id) = 0;

	virtual UniformBufferID createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement) = 0;
	virtual void freeUniformBuffer(UniformBufferID id) = 0;
	virtual LockedBuffer lockUniformBuffer(UniformBufferID id) = 0;
	virtual LockedBuffer lockUniformBufferIndex(UniformBufferID id, int index) = 0;
	virtual void unlockUniformBuffer(UniformBufferID id) = 0;
	virtual void unlockUniformBufferIndex(UniformBufferID id, int index) = 0;

	virtual RenderLightID createLight() = 0;
	virtual void freeLight(RenderLightID id) = 0;
	virtual bool populateLight(RenderLightID id, const Double3 &point, double startRadius, double endRadius) = 0;

	// Texture management functions.
	virtual ObjectTextureID createObjectTexture(int width, int height, int bytesPerTexel) = 0;
	virtual void freeObjectTexture(ObjectTextureID id) = 0;
	virtual std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const = 0;
	virtual LockedTexture lockObjectTexture(ObjectTextureID id) = 0;
	virtual void unlockObjectTexture(ObjectTextureID id) = 0;

	virtual UiTextureID createUiTexture(int width, int height) = 0;
	virtual void freeUiTexture(UiTextureID id) = 0;
	virtual std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const = 0;
	virtual LockedTexture lockUiTexture(UiTextureID id) = 0;
	virtual void unlockUiTexture(UiTextureID id) = 0;

	// Material management functions.
	virtual RenderMaterialID createMaterial(RenderMaterialKey key) = 0;
	virtual void freeMaterial(RenderMaterialID id) = 0;
	virtual void setMaterialParameterMeshLightingPercent(RenderMaterialID id, double value) = 0;
	virtual void setMaterialParameterPixelShaderParam(RenderMaterialID id, double value) = 0;

	// Renders a frame to the target window. Currently this is blocking and should be safe to present the frame upon returning.
	virtual void submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
		const RenderCamera &camera, const RenderFrameSettings &frameSettings) = 0;
};

#endif
