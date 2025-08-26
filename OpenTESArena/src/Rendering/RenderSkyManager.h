#ifndef RENDER_SKY_MANAGER_H
#define RENDER_SKY_MANAGER_H

#include "RenderDrawCall.h"
#include "RenderMaterialUtils.h"
#include "RenderMeshUtils.h"
#include "RenderTextureUtils.h"
#include "../Assets/TextureAsset.h"

class Renderer;
class SkyInfoDefinition;
class SkyInstance;
class SkyVisibilityManager;
class TextureManager;
class WeatherInstance;

enum class WeatherType;

struct ExeData;
struct RenderCommandList;

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

	// All the possible sky color textures to choose from, dependent on the active weather. These are used by
	// the renderer to look up palette colors.
	ScopedObjectTextureRef skyGradientAMTextureRef;
	ScopedObjectTextureRef skyGradientPMTextureRef;
	ScopedObjectTextureRef skyFogTextureRef;
	Buffer<ScopedObjectTextureRef> skyThunderstormTextureRefs; // One for each frame of flash animation.
	ScopedObjectTextureRef skyInteriorTextureRef; // Default black for interiors.

	RenderMaterialID skyGradientAMMaterialID;
	RenderMaterialID skyGradientPMMaterialID;
	RenderMaterialID skyFogMaterialID;
	Buffer<RenderMaterialID> skyThunderstormMaterialIDs;
	RenderMaterialID skyInteriorMaterialID;

	VertexPositionBufferID bgPositionBufferID;
	VertexAttributeBufferID bgNormalBufferID;
	VertexAttributeBufferID bgTexCoordBufferID;
	IndexBufferID bgIndexBufferID;
	UniformBufferID bgTransformBufferID;
	RenderDrawCall bgDrawCall;
	ObjectTextureID activeBgTextureID;

	std::vector<LoadedGeneralSkyObjectTextureEntry> generalSkyObjectTextures;
	std::vector<LoadedSmallStarTextureEntry> smallStarTextures;
	std::vector<RenderMaterial> objectMaterials;

	// All sky objects share simple vertex + attribute + index buffers.
	VertexPositionBufferID objectPositionBufferID;
	VertexAttributeBufferID objectNormalBufferID;
	VertexAttributeBufferID objectTexCoordBufferID;
	IndexBufferID objectIndexBufferID;
	UniformBufferID objectTransformBufferID;
	std::vector<RenderDrawCall> objectDrawCalls; // Order matters: stars, sun, planets, clouds, mountains.

	ObjectTextureID getGeneralSkyObjectTextureID(const TextureAsset &textureAsset) const;
	ObjectTextureID getSmallStarTextureID(uint8_t paletteIndex) const;

	void freeBgBuffers(Renderer &renderer);
	void freeObjectBuffers(Renderer &renderer);
public:
	RenderSkyManager();

	void init(const ExeData &exeData, TextureManager &textureManager, Renderer &renderer);
	void shutdown(Renderer &renderer);

	ObjectTextureID getBgTextureID() const;
	void populateCommandList(RenderCommandList &commandList) const;

	void loadScene(const SkyInstance &skyInst, const SkyInfoDefinition &skyInfoDef, TextureManager &textureManager, Renderer &renderer);
	void update(const SkyInstance &skyInst, const SkyVisibilityManager &skyVisManager, const WeatherInstance &weatherInst,
		const CoordDouble3 &cameraCoord, bool isInterior, double dayPercent, bool isFoggy, double distantAmbientPercent, Renderer &renderer);
	void unloadScene(Renderer &renderer);
};

#endif
