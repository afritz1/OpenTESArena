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

	// Gets various profiler information about internal renderer state.
	Renderer3DProfilerData getProfilerData() const override;

	Surface getScreenshot() const override;

	int getBytesPerFloat() const override;

	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent) override;
	void freeVertexPositionBuffer(VertexPositionBufferID id) override;
	LockedBuffer lockVertexPositionBuffer(VertexPositionBufferID id) override;
	void unlockVertexPositionBuffer(VertexPositionBufferID id) override;

	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent) override;
	void freeVertexAttributeBuffer(VertexAttributeBufferID id) override;
	LockedBuffer lockVertexAttributeBuffer(VertexAttributeBufferID id) override;
	void unlockVertexAttributeBuffer(VertexAttributeBufferID id) override;

	IndexBufferID createIndexBuffer(int indexCount, int bytesPerIndex) override;
	void freeIndexBuffer(IndexBufferID id) override;
	LockedBuffer lockIndexBuffer(IndexBufferID id) override;
	void unlockIndexBuffer(IndexBufferID id) override;

	UniformBufferID createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement) override;
	void freeUniformBuffer(UniformBufferID id) override;
	LockedBuffer lockUniformBuffer(UniformBufferID id) override;
	LockedBuffer lockUniformBufferIndex(UniformBufferID id, int index) override;
	void unlockUniformBuffer(UniformBufferID id) override;
	void unlockUniformBufferIndex(UniformBufferID id, int index) override;

	RenderLightID createLight() override;
	void freeLight(RenderLightID id) override;
	bool populateLight(RenderLightID id, const Double3 &point, double startRadius, double endRadius) override;

	ObjectTextureID createObjectTexture(int width, int height, int bytesPerTexel) override;
	void freeObjectTexture(ObjectTextureID id) override;
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;
	LockedTexture lockObjectTexture(ObjectTextureID id) override;
	void unlockObjectTexture(ObjectTextureID id) override;

	UiTextureID createUiTexture(int width, int height) override;
	void freeUiTexture(UiTextureID id) override;
	std::optional<Int2> tryGetUiTextureDims(UiTextureID id) const override;
	LockedTexture lockUiTexture(UiTextureID id) override;
	void unlockUiTexture(UiTextureID id) override;

	// Renders a frame to the target window. Currently this is blocking and should be safe to present
	// the frame upon returning.
	void submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
		const RenderCamera &camera, const RenderFrameSettings &frameSettings) override;
};

#endif
