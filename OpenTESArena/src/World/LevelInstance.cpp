#include "LevelInstance.h"
#include "MapDefinition.h"
#include "MapType.h"
#include "WeatherUtils.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

LevelInstance::LevelInstance()
{
	this->ceilingScale = 0.0;
}

void LevelInstance::init(double ceilingScale)
{
	this->ceilingScale = ceilingScale;

	// @todo: remove fixed-size grid dependency in entity manager.
	DebugNotImplemented();
	//this->entityManager.init(-1, -1);
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

double LevelInstance::getCeilingScale() const
{
	return this->ceilingScale;
}

bool LevelInstance::trySetActive(WeatherType weatherType, bool nightLightsAreActive, TextureManager &textureManager,
	Renderer &renderer)
{
	DebugNotImplemented();

	// Clear renderer textures, distant sky, and entities.
	renderer.clearTexturesAndEntityRenderIDs();
	renderer.clearDistantSky();
	this->entityManager.clear();

	// @todo: set renderer sky palette
	// @todo: set interior sky color if is interior.
	//Buffer<uint32_t> skyPalette = WeatherUtils::makeExteriorSkyPalette(weatherType, textureManager);

	// Set renderer fog distance.
	const double fogDistance = WeatherUtils::getFogDistanceFromWeather(weatherType);
	renderer.setFogDistance(fogDistance);

	// @todo: set renderer night lights active via ArenaPaletteName::Default as the night light palette
	DebugNotImplemented();

	// @todo: see LevelData::setActive() for reference
	// - load voxel textures
	// - load chasm textures
	// - instantiate entities and load entity textures

	return true;
}

void LevelInstance::update(double dt, const ChunkInt2 &centerChunk, int activeLevelIndex,
	const MapDefinition &mapDefinition, int chunkDistance)
{
	this->chunkManager.update(dt, centerChunk, activeLevelIndex, mapDefinition, chunkDistance,
		this->entityManager);
}
