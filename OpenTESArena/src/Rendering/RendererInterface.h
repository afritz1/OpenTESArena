#ifndef RENDERER_INTERFACE_H
#define RENDERER_INTERFACE_H

#include "RenderCamera.h"
#include "RenderDefinitionGroup.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "RenderInstanceGroup.h"
#include "RenderTextureUtils.h"

class RendererInterface
{
public:
	virtual void init(const RenderInitSettings &settings) = 0;
	virtual void shutdown() = 0;
	virtual void resize(int width, int height) = 0;
	virtual VoxelTextureID createVoxelTexture(int width, int height) = 0; // @todo: texture builder
	virtual SpriteTextureID createSpriteTexture(int width, int height) = 0; // @todo: texture builder
	virtual void freeVoxelTexture(VoxelTextureID textureID) = 0;
	virtual void freeSpriteTexture(SpriteTextureID textureID) = 0;
	virtual void submitFrame(const RenderDefinitionGroup &defGroup, const RenderInstanceGroup &instGroup,
		const RenderCamera &camera, const RenderFrameSettings &settings) = 0;
	virtual void present() = 0;
};

#endif
