#ifndef RENDERER_INTERFACE_H
#define RENDERER_INTERFACE_H

#include <optional>

#include "RenderTextureUtils.h"

class RenderCamera;
class RenderDefinitionGroup;
class RenderFrameSettings;
class RenderInitSettings;
class RenderInstanceGroup;
class TextureBuilder;

class RendererInterface
{
public:
	virtual void init(const RenderInitSettings &settings) = 0;
	virtual void shutdown() = 0;
	virtual void resize(int width, int height) = 0;
	virtual std::optional<VoxelTextureID> tryCreateVoxelTexture(const TextureBuilder &textureBuilder) = 0;
	virtual std::optional<EntityTextureID> tryCreateEntityTexture(const TextureBuilder &textureBuilder) = 0;
	virtual std::optional<SkyTextureID> tryCreateSkyTexture(const TextureBuilder &textureBuilder) = 0;
	virtual std::optional<UiTextureID> tryCreateUiTexture(const TextureBuilder &textureBuilder) = 0;
	virtual void freeVoxelTexture(VoxelTextureID id) = 0;
	virtual void freeEntityTexture(EntityTextureID id) = 0;
	virtual void freeSkyTexture(SkyTextureID id) = 0;
	virtual void freeUiTexture(UiTextureID id) = 0;
	virtual void submitFrame(const RenderDefinitionGroup &defGroup, const RenderInstanceGroup &instGroup,
		const RenderCamera &camera, const RenderFrameSettings &settings) = 0;
	virtual void present() = 0;
};

#endif
