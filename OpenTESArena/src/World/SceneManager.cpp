#include "SceneManager.h"

#include "components/debug/Debug.h"

SceneManager::SceneManager()
{
	
}

void SceneManager::cleanUp()
{
	this->chunkManager.cleanUp();
	this->voxelChunkManager.cleanUp();
	this->entityChunkManager.cleanUp();
}
