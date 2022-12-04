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
		if (!renderer.tryCreateObjectTexture(static_cast<int>(defaultPalette.size()), 1, true, &paletteTextureID))
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

		DebugAssert(lockedPaletteTexture.isTrueColor);
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

EntityManager &LevelInstance::getEntityManager()
{
	return this->entityManager;
}

const EntityManager &LevelInstance::getEntityManager() const
{
	return this->entityManager;
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
	const CoordDouble3 &playerCoord, const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	int chunkDistance, double chasmAnimPercent, RenderChunkManager &renderChunkManager, TextureManager &textureManager,
	AudioManager &audioManager, Renderer &renderer)
{
	const ChunkInt2 &centerChunkPos = playerCoord.chunk;
	this->voxelChunkManager.update(dt, newChunkPositions, freedChunkPositions, playerCoord, activeLevelIndex,
		mapDefinition, this->ceilingScale, audioManager);

	for (int i = 0; i < freedChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = freedChunkPositions.get(i);
		renderChunkManager.unloadVoxelChunk(chunkPos, renderer);
	}

	for (int i = 0; i < newChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = newChunkPositions.get(i);
		VoxelChunk &voxelChunk = this->voxelChunkManager.getChunkAtPosition(chunkPos);
		renderChunkManager.loadVoxelChunk(voxelChunk, this->ceilingScale, textureManager, renderer);
		renderChunkManager.rebuildVoxelChunkDrawCalls(voxelChunk, this->ceilingScale, chasmAnimPercent, true, false);
	}

	for (int i = 0; i < activeChunkPositions.getCount(); i++)
	{
		const ChunkInt2 &chunkPos = activeChunkPositions.get(i);
		const VoxelChunk &voxelChunk = this->voxelChunkManager.getChunkAtPosition(chunkPos);
		renderChunkManager.rebuildVoxelChunkDrawCalls(voxelChunk, this->ceilingScale, chasmAnimPercent, false, true);
	}

	// @todo: only rebuild if needed; currently we assume that all scenes in the game have some kind of animating chasms/etc., which is inefficient
	//if ((freedChunkCount > 0) || (newChunkCount > 0))
	{
		renderChunkManager.rebuildVoxelDrawCallsList();
	}

	//this->entityManager.tick(game, dt); // @todo: simulate entities after voxel chunks are working well
}

void LevelInstance::cleanUp()
{
	this->voxelChunkManager.cleanUp();
}
