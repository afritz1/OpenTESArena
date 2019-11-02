#include "EntityData.h"

EntityData::EntityData(int flatIndex)
{
	this->flatIndex = flatIndex;
}

int EntityData::getFlatIndex() const
{
	return this->flatIndex;
}

EntityAnimationData &EntityData::getAnimationData()
{
	return this->animationData;
}

const EntityAnimationData &EntityData::getAnimationData() const
{
	return this->animationData;
}
