#include "SceneManager.h"

#include "components/debug/Debug.h"

SceneManager::SceneManager()
{
	this->mapDefIndex = -1;
	this->activeLevelIndex = -1;
	this->activeSkyIndex = -1;
	//this->activeWeatherIndex = -1;
	this->ceilingScale = 0.0;
}

void SceneManager::init(int mapDefIndex)
{
	DebugAssert(mapDefIndex >= 0);
	this->mapDefIndex = mapDefIndex;
}

void SceneManager::setLevelActive(int index)
{
	// @todo: if given not the active level, then clears active chunk managers, and populates all the chunk managers.
	DebugNotImplemented();

	// @todo: does it need the camera's position?
}

void SceneManager::cleanUp()
{
	this->voxelChunkManager.cleanUp();
	this->entityChunkManager.cleanUp();
}
