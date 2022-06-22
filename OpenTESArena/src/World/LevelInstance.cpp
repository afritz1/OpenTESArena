#include <algorithm>

#include "ArenaWeatherUtils.h"
#include "LevelInstance.h"
#include "MapDefinition.h"
#include "MapType.h"
#include "WeatherDefinition.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Entities/CitizenUtils.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"

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

ChunkManager &LevelInstance::getChunkManager()
{
	return this->chunkManager;
}

const ChunkManager &LevelInstance::getChunkManager() const
{
	return this->chunkManager;
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

// @temp for renderer.loadScene(); remove once we know where to properly use loadScene() and with what parameters.
#include "../Rendering/RenderCamera.h"
#include "../World/SkyInstance.h"
#include "../Entities/EntityDefinitionLibrary.h"
bool LevelInstance::trySetActive(const WeatherDefinition &weatherDef, bool nightLightsAreActive,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
	TextureManager &textureManager, Renderer &renderer)
{
	// Clear stored object texture refs, freeing them from the renderer.
	this->paletteTextureRef.destroy();
	this->lightTableTextureRef.destroy();

	renderer.unloadScene();

	DebugLogWarning("renderer.loadScene() is taking temp arguments.");
	renderer.loadScene(RenderCamera(), *this, SkyInstance(), 0, mapDefinition, citizenGenInfo,
		0.0, 0.0, 0.0, false, true, EntityDefinitionLibrary(), textureManager);

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

void LevelInstance::update(double dt, Game &game, const CoordDouble3 &playerCoord,
	const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	const EntityGeneration::EntityGenInfo &entityGenInfo,
	const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, int chunkDistance,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	TextureManager &textureManager, AudioManager &audioManager)
{
	const ChunkInt2 &centerChunk = playerCoord.chunk;
	this->chunkManager.update(dt, centerChunk, playerCoord, activeLevelIndex, mapDefinition, entityGenInfo,
		citizenGenInfo, this->ceilingScale, chunkDistance, entityDefLibrary, binaryAssetLibrary, textureManager,
		audioManager, this->entityManager);

	this->entityManager.tick(game, dt);
}

void LevelInstance::cleanUp()
{
	this->chunkManager.cleanUp();
}
