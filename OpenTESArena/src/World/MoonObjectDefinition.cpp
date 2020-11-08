#include "MoonObjectDefinition.h"

void MoonObjectDefinition::init(const Double3 &baseDir, double bonusLatitude, int phaseCount,
	int phaseIndexDayOffset, ImageID imageID)
{
	this->baseDir = baseDir;
	this->bonusLatitude = bonusLatitude;
	this->phaseCount = phaseCount;
	this->phaseIndexDayOffset = phaseIndexDayOffset;
	this->imageID = imageID;
}

const Double3 &MoonObjectDefinition::getBaseDirection() const
{
	return this->baseDir;
}

double MoonObjectDefinition::getBonusLatitude() const
{
	return this->bonusLatitude;
}

int MoonObjectDefinition::getPhaseCount() const
{
	return this->phaseCount;
}

int MoonObjectDefinition::getPhaseIndexDayOffset() const
{
	return this->phaseIndexDayOffset;
}

ImageID MoonObjectDefinition::getImageID() const
{
	return this->imageID;
}
