#include "MapInstance.h"

void MapInstance::init(int levelCount)
{
	this->levels.init(levelCount);
}

LevelInstance &MapInstance::getLevel(int index)
{
	return this->levels.get(index);
}
