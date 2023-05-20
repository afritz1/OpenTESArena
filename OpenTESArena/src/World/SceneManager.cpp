#include "SceneManager.h"

#include "components/debug/Debug.h"

SceneManager::SceneManager()
{
	this->ceilingScale = 0.0;
}

void SceneManager::cleanUp()
{
	this->voxelChunkManager.cleanUp();
	this->entityChunkManager.cleanUp();
}
