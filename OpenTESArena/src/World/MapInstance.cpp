#include "MapInstance.h"

void MapInstance::init(int levelCount, int skyCount)
{
	this->levels.init(levelCount);
	this->skies.init(skyCount);
}

int MapInstance::getLevelCount() const
{
	return this->levels.getCount();
}

LevelInstance &MapInstance::getLevel(int index)
{
	return this->levels.get(index);
}

int MapInstance::getSkyCount() const
{
	return this->skies.getCount();
}

SkyInstance &MapInstance::getSky(int index)
{
	return this->skies.get(index);
}
