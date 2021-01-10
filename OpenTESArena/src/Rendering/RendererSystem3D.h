#ifndef RENDERER_SYSTEM_3D_H
#define RENDERER_SYSTEM_3D_H

#include <cstdint>
#include <optional>

#include "RenderTextureUtils.h"
#include "../Entities/EntityUtils.h" // @todo: remove dependency on this
#include "../Math/MathUtils.h"
#include "../Math/Vector3.h"
#include "../Media/Palette.h"
#include "../World/LevelData.h"
#include "../World/VoxelDefinition.h"

// Abstract base class for 3D renderer.

// @todo: clean up this API since it currently contains leftovers from the previous design.

class DistantSky;
class EntityAnimationDefinition;
class EntityAnimationInstance;
class EntityDefinitionLibrary;
class EntityManager;
class RenderCamera;
class RenderDefinitionGroup;
class RenderFrameSettings;
class RenderInitSettings;
class RenderInstanceGroup;
class TextureBuilder;
class TextureManager;
class TextureInstanceManager;
class VoxelGrid;

class RendererSystem3D
{
public:
	// Profiling info gathered from internal renderer state.
	struct ProfilerData
	{
		int width, height;
		int potentiallyVisFlatCount, visFlatCount, visLightCount;

		ProfilerData(int width, int height, int potentiallyVisFlatCount, int visFlatCount, int visLightCount);
	};

	virtual void init(const RenderInitSettings &settings) = 0;
	virtual void shutdown() = 0;

	virtual bool isInited() const = 0;

	// Texture handle allocation functions for each texture type.
	virtual std::optional<VoxelTextureID> tryCreateVoxelTexture(const TextureBuilder &textureBuilder) = 0;
	virtual std::optional<EntityTextureID> tryCreateEntityTexture(const TextureBuilder &textureBuilder) = 0;
	virtual std::optional<SkyTextureID> tryCreateSkyTexture(const TextureBuilder &textureBuilder) = 0;

	// Texture handle freeing functions for each texture type.
	virtual void freeVoxelTexture(VoxelTextureID id) = 0;
	virtual void freeEntityTexture(EntityTextureID id) = 0;
	virtual void freeSkyTexture(SkyTextureID id) = 0;

	virtual void resize(int width, int height) = 0;

	// Tries to write out selection data for the given entity. Returns whether selection data was
	// successfully written.
	virtual bool tryGetEntitySelectionData(const Double2 &uv, EntityRenderID entityRenderID,
		int animStateID, int animAngleID, int animKeyframeID, bool pixelPerfect, bool *outIsSelected) const = 0;

	// Converts a screen point into a ray in the game world.
	virtual Double3 screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
		Degrees fovY, double aspect) const = 0;

	// Gets various profiler information about internal renderer state.
	virtual ProfilerData getProfilerData() const = 0;

	// Legacy functions (remove these eventually).
	virtual void setRenderThreadsMode(int mode) = 0;
	virtual void setFogDistance(double fogDistance) = 0;
	virtual void setVoxelTexture(int id, const uint8_t *srcTexels, const Palette &palette) = 0;
	virtual EntityRenderID makeEntityRenderID() = 0;
	virtual void setFlatTextures(EntityRenderID entityRenderID, const EntityAnimationDefinition &animDef,
		const EntityAnimationInstance &animInst, bool isPuddle, const Palette &palette,
		TextureManager &textureManager, const TextureInstanceManager &textureInstManager) = 0;
	virtual void addChasmTexture(VoxelDefinition::ChasmData::Type chasmType, const uint8_t *colors,
		int width, int height, const Palette &palette) = 0;
	virtual void setDistantSky(const DistantSky &distantSky, const Palette &palette,
		TextureManager &textureManager) = 0;
	virtual void setSkyPalette(const uint32_t *colors, int count) = 0;
	virtual void setNightLightsActive(bool active, const Palette &palette) = 0;
	virtual void clearTexturesAndEntityRenderIDs() = 0;
	virtual void clearDistantSky() = 0;
	virtual void render(const Double3 &eye, const Double3 &forward, double fovY, double ambient,
		double daytimePercent, double chasmAnimPercent, double latitude, bool nightLightsAreActive,
		bool isExterior, bool playerHasLight, int chunkDistance, double ceilingHeight,
		const std::vector<LevelData::DoorState> &openDoors,
		const std::vector<LevelData::FadeState> &fadingVoxels,
		const LevelData::ChasmStates &chasmStates, const VoxelGrid &voxelGrid,
		const EntityManager &entityManager, const EntityDefinitionLibrary &entityDefLibrary,
		uint32_t *colorBuffer) = 0;
	
	// Begins rendering a frame. Currently this is a blocking call and it should be safe to present the frame
	// upon returning from this.
	virtual void submitFrame(const RenderDefinitionGroup &defGroup, const RenderInstanceGroup &instGroup,
		const RenderCamera &camera, const RenderFrameSettings &settings) = 0;

	// Presents the finished frame to the screen. This may just be a copy to the screen frame buffer that
	// is then taken care of by the top-level rendering manager, since UI must be drawn afterwards.
	virtual void present() = 0;
};

#endif
