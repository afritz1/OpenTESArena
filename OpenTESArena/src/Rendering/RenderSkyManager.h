#ifndef RENDER_SKY_MANAGER_H
#define RENDER_SKY_MANAGER_H

#include "RenderDrawCall.h"
#include "RenderGeometryUtils.h"
#include "RenderTextureUtils.h"
#include "../Assets/TextureAsset.h"

class Renderer;
class SkyInfoDefinition;
class SkyInstance;

class RenderSkyManager
{
private:
	enum class LoadedSkyObjectTextureType
	{
		PaletteIndex,
		TextureAsset
	};

	struct LoadedSkyObjectTexture
	{
		LoadedSkyObjectTextureType type;
		uint8_t paletteIndex;
		TextureAsset textureAsset;
		ScopedObjectTextureRef objectTextureRef;

		LoadedSkyObjectTexture();

		void initPaletteIndex(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef);
		void initTextureAsset(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef);
	};

	VertexBufferID bgVertexBufferID;
	AttributeBufferID bgNormalBufferID;
	AttributeBufferID bgTexCoordBufferID;
	IndexBufferID bgIndexBufferID;
	ObjectTextureID bgObjectTextureID;
	RenderDrawCall bgDrawCall;

	// All sky objects share simple vertex + attribute + index buffers.
	VertexBufferID objectVertexBufferID;
	AttributeBufferID objectNormalBufferID;
	AttributeBufferID objectTexCoordBufferID;
	IndexBufferID objectIndexBufferID;
	std::vector<LoadedSkyObjectTexture> objectTextures;
	std::vector<RenderDrawCall> objectDrawCalls; // Order matters: stars, sun, planets, clouds, mountains.

	ObjectTextureID getSkyObjectTextureID(const TextureAsset &textureAsset) const;

	void freeBgBuffers(Renderer &renderer);
	void freeObjectBuffers(Renderer &renderer);
public:
	RenderSkyManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	RenderDrawCall getBgDrawCall() const;
	BufferView<const RenderDrawCall> getObjectDrawCalls() const;

	void loadScene(const SkyInstance &skyInst, const SkyInfoDefinition &skyInfoDef, TextureManager &textureManager, Renderer &renderer);
	void update(const CoordDouble3 &cameraCoord);
	void unloadScene(Renderer &renderer);
};

#endif
