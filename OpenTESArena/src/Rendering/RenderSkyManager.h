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
	struct LoadedGeneralSkyObjectTextureEntry
	{
		TextureAsset textureAsset;
		ScopedObjectTextureRef objectTextureRef;

		void init(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef);
	};

	struct LoadedSmallStarTextureEntry
	{
		uint8_t paletteIndex;
		ScopedObjectTextureRef objectTextureRef;

		void init(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef);
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
	std::vector<LoadedGeneralSkyObjectTextureEntry> generalSkyObjectTextures;
	std::vector<LoadedSmallStarTextureEntry> smallStarTextures;
	std::vector<RenderDrawCall> objectDrawCalls; // Order matters: stars, sun, planets, clouds, mountains.

	ObjectTextureID getGeneralSkyObjectTextureID(const TextureAsset &textureAsset) const;
	ObjectTextureID getSmallStarTextureID(uint8_t paletteIndex) const;

	void freeBgBuffers(Renderer &renderer);
	void freeObjectBuffers(Renderer &renderer);
public:
	RenderSkyManager();

	void init(Renderer &renderer);
	void shutdown(Renderer &renderer);

	RenderDrawCall getBgDrawCall() const;
	BufferView<const RenderDrawCall> getObjectDrawCalls() const;

	void loadScene(const SkyInfoDefinition &skyInfoDef, TextureManager &textureManager, Renderer &renderer);
	void update(const SkyInstance &skyInst, const CoordDouble3 &cameraCoord, const Renderer &renderer);
	void unloadScene(Renderer &renderer);
};

#endif
