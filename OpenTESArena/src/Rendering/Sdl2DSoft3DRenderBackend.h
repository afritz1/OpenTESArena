#ifndef SDL_2D_SOFT_3D_RENDER_BACKEND_H
#define SDL_2D_SOFT_3D_RENDER_BACKEND_H

#include "RenderBackend.h"
#include "SoftwareRenderer.h"
#include "SdlUiRenderer.h"

class Sdl2DSoft3DRenderBackend final : public RenderBackend
{
private:
	const Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *nativeTexture; // Window frame buffer equal to window dimensions.
	SDL_Texture *gameWorldTexture; // Internal rendering frame buffer, variable dimensions.
	SdlUiRenderer renderer2D;
	SoftwareRenderer renderer3D;
public:
	Sdl2DSoft3DRenderBackend();
	virtual ~Sdl2DSoft3DRenderBackend();

	bool init(const RenderInitSettings &initSettings) override;
	void shutdown() override;

	void resize(int windowWidth, int windowHeight, int internalWidth, int internalHeight) override;
	void handleRenderTargetsReset(int windowWidth, int windowHeight, int internalWidth, int internalHeight) override;

	// Geometry management functions.
	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex) override;
	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex) override;
	IndexBufferID createIndexBuffer(int indexCount) override;
	void populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions) override;
	void populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes) override;
	void populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices) override;
	void freeVertexPositionBuffer(VertexPositionBufferID id) override;
	void freeVertexAttributeBuffer(VertexAttributeBufferID id) override;
	void freeIndexBuffer(IndexBufferID id) override;

	// Texture management functions.
	ObjectTextureAllocator *getObjectTextureAllocator() override;
	UiTextureAllocator *getUiTextureAllocator() override;
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;
	std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const override;

	// Uniform management functions.
	UniformBufferID createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement) override;
	void populateUniformBuffer(UniformBufferID id, Span<const std::byte> data) override;
	void populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData) override;
	void freeUniformBuffer(UniformBufferID id) override;

	// Light management functions.
	RenderLightID createLight() override;
	void setLightPosition(RenderLightID id, const Double3 &worldPoint) override;
	void setLightRadius(RenderLightID id, double startRadius, double endRadius) override;
	void freeLight(RenderLightID id) override;

	// Gets various profiler information about internal renderer state.
	Renderer3DProfilerData getProfilerData() const override;

	Surface getScreenshot() const override;

	// Renders a frame to the target window. Currently this is blocking and should be safe to present
	// the frame upon returning.
	void submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
		const RenderCamera &camera, const RenderFrameSettings &frameSettings) override;
};

#endif
