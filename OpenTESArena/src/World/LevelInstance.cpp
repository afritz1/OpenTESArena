#include <algorithm>

#include "LevelInstance.h"
#include "MapDefinition.h"
#include "MapType.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureManager.h"
#include "../Entities/CitizenUtils.h"
#include "../Game/Game.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../Weather/WeatherDefinition.h"

#include "components/debug/Debug.h"

namespace
{
	bool TryPopulatePaletteTexture(ScopedObjectTextureRef &paletteTextureRef, TextureManager &textureManager, Renderer &renderer)
	{
		const std::string &defaultPaletteFilename = ArenaPaletteName::Default;
		const std::optional<PaletteID> defaultPaletteID = textureManager.tryGetPaletteID(defaultPaletteFilename.c_str());
		if (!defaultPaletteID.has_value())
		{
			DebugLogError("Couldn't get default palette ID from \"" + defaultPaletteFilename + "\".");
			return false;
		}

		const Palette &defaultPalette = textureManager.getPaletteHandle(*defaultPaletteID);
		ObjectTextureID paletteTextureID;
		if (!renderer.tryCreateObjectTexture(static_cast<int>(defaultPalette.size()), 1, 4, &paletteTextureID))
		{
			DebugLogError("Couldn't create default palette texture \"" + defaultPaletteFilename + "\".");
			return false;
		}

		paletteTextureRef.init(paletteTextureID, renderer);
		LockedTexture lockedPaletteTexture = paletteTextureRef.lockTexels();
		if (!lockedPaletteTexture.isValid())
		{
			DebugLogError("Couldn't lock palette texture \"" + defaultPaletteFilename + "\" for writing.");
			return false;
		}

		DebugAssert(lockedPaletteTexture.bytesPerTexel == 4);
		uint32_t *paletteTexels = static_cast<uint32_t*>(lockedPaletteTexture.texels);
		std::transform(defaultPalette.begin(), defaultPalette.end(), paletteTexels,
			[](const Color &paletteColor)
		{
			return paletteColor.toARGB();
		});

		paletteTextureRef.unlockTexels();
		return true;
	}

	bool TryPopulateLightTableTexture(ScopedObjectTextureRef &lightTableTextureRef, TextureManager &textureManager, Renderer &renderer)
	{
		const std::string &lightTableFilename = ArenaTextureName::NormalLightTable;
		const std::optional<TextureBuilderID> lightTableTextureBuilderID = textureManager.tryGetTextureBuilderID(lightTableFilename.c_str());
		if (!lightTableTextureBuilderID.has_value())
		{
			DebugLogError("Couldn't get light table texture builder ID from \"" + lightTableFilename + "\".");
			return false;
		}

		const TextureBuilder &lightTableTextureBuilder = textureManager.getTextureBuilderHandle(*lightTableTextureBuilderID);
		ObjectTextureID lightTableTextureID;
		if (!renderer.tryCreateObjectTexture(lightTableTextureBuilder, &lightTableTextureID))
		{
			DebugLogError("Couldn't create light table texture \"" + lightTableFilename + "\".");
			return false;
		}

		lightTableTextureRef.init(lightTableTextureID, renderer);
		return true;
	}
}

LevelInstance::LevelInstance()
{
	this->ceilingScale = 0.0;
}

void LevelInstance::init(double ceilingScale)
{
	this->ceilingScale = ceilingScale;
}

VoxelChunkManager &LevelInstance::getVoxelChunkManager()
{
	return this->voxelChunkManager;
}

const VoxelChunkManager &LevelInstance::getVoxelChunkManager() const
{
	return this->voxelChunkManager;
}

CollisionChunkManager &LevelInstance::getCollisionChunkManager()
{
	return this->collisionChunkManager;
}

const CollisionChunkManager &LevelInstance::getCollisionChunkManager() const
{
	return this->collisionChunkManager;
}

EntityChunkManager &LevelInstance::getEntityChunkManager()
{
	return this->entityChunkManager;
}

const EntityChunkManager &LevelInstance::getEntityChunkManager() const
{
	return this->entityChunkManager;
}

ObjectTextureID LevelInstance::getPaletteTextureID() const
{
	return this->paletteTextureRef.get();
}

ObjectTextureID LevelInstance::getLightTableTextureID() const
{
	return this->lightTableTextureRef.get();
}

double LevelInstance::getCeilingScale() const
{
	return this->ceilingScale;
}

bool LevelInstance::trySetActive(RenderChunkManager &renderChunkManager, TextureManager &textureManager, Renderer &renderer)
{
	// Clear stored object texture refs, freeing them from the renderer.
	this->paletteTextureRef.destroy();
	this->lightTableTextureRef.destroy();

	renderChunkManager.unloadScene(renderer);

	if (!TryPopulatePaletteTexture(this->paletteTextureRef, textureManager, renderer))
	{
		DebugLogError("Couldn't load palette texture.");
		return false;
	}

	if (!TryPopulateLightTableTexture(this->lightTableTextureRef, textureManager, renderer))
	{
		DebugLogError("Couldn't load light table texture.");
		return false;
	}

	return true;
}

void LevelInstance::update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
	const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
	const Player &player, const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	double chasmAnimPercent, Random &random, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, RenderChunkManager &renderChunkManager, TextureManager &textureManager,
	AudioManager &audioManager, Renderer &renderer)
{
	const CoordDouble3 &playerCoord = player.getPosition();
	const CoordDouble2 playerCoordXZ(playerCoord.chunk, VoxelDouble2(playerCoord.point.x, playerCoord.point.z));
	const VoxelDouble2 playerDirXZ = player.getGroundDirection();

	const MapSubDefinition &mapSubDef = mapDefinition.getSubDefinition();
	const MapType mapType = mapSubDef.type;
	const BufferView<const LevelDefinition> levelDefs = mapDefinition.getLevels();
	const BufferView<const int> levelInfoDefIndices = mapDefinition.getLevelInfoIndices();
	const BufferView<const LevelInfoDefinition> levelInfoDefs = mapDefinition.getLevelInfos();
	const LevelDefinition *activeLevelDef = nullptr;
	const LevelInfoDefinition *activeLevelInfoDef = nullptr;
	switch (mapType)
	{
	case MapType::Interior:
	{
		DebugAssert(activeLevelIndex.has_value());
		activeLevelDef = &levelDefs[*activeLevelIndex];

		const int levelInfoDefIndex = levelInfoDefIndices[*activeLevelIndex];
		activeLevelInfoDef = &levelInfoDefs[levelInfoDefIndex];
		break;
	}
	case MapType::City:
	{
		DebugAssert(activeLevelIndex.has_value() && (*activeLevelIndex == 0));
		activeLevelDef = &levelDefs[0];

		const int levelInfoDefIndex = levelInfoDefIndices[0];
		activeLevelInfoDef = &levelInfoDefs[levelInfoDefIndex];
		break;
	}
	case MapType::Wilderness:
		// The wilderness doesn't have an active level index since it picks from a bag of levels for populating chunks.
		DebugAssert(!activeLevelIndex.has_value() || (*activeLevelIndex == 0));
		break;
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(mapType)));
		break;
	}

	// Simulate game world.
	this->voxelChunkManager.update(dt, newChunkPositions, freedChunkPositions, playerCoord, activeLevelDef,
		activeLevelInfoDef, mapSubDef, levelDefs, levelInfoDefIndices, levelInfoDefs, this->ceilingScale, audioManager);
	this->entityChunkManager.update(dt, activeChunkPositions, newChunkPositions, freedChunkPositions, player,
		activeLevelDef, activeLevelInfoDef, mapSubDef, levelDefs, levelInfoDefIndices, levelInfoDefs, entityGenInfo,
		citizenGenInfo, this->ceilingScale, random, this->voxelChunkManager, entityDefLibrary, binaryAssetLibrary,
		audioManager, textureManager, renderer);
	this->collisionChunkManager.update(dt, activeChunkPositions, newChunkPositions, freedChunkPositions, this->voxelChunkManager);

	// Update rendering.
	renderChunkManager.updateActiveChunks(activeChunkPositions, newChunkPositions, freedChunkPositions, this->voxelChunkManager, renderer);
	renderChunkManager.updateVoxels(activeChunkPositions, newChunkPositions, this->ceilingScale, chasmAnimPercent,
		this->voxelChunkManager, textureManager, renderer);
	renderChunkManager.updateEntities(activeChunkPositions, newChunkPositions, playerCoordXZ, playerDirXZ, ceilingScale,
		this->voxelChunkManager, this->entityChunkManager, entityDefLibrary, textureManager, renderer);
}

void LevelInstance::cleanUp()
{
	this->voxelChunkManager.cleanUp();
	this->entityChunkManager.cleanUp();
}
