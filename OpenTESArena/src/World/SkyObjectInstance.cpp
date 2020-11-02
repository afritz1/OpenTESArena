#include "SkyObjectDefinition.h"
#include "SkyObjectInstance.h"
#include "../Media/TextureManager.h"

void SkyObjectInstance::Land::init(Radians angleX)
{
	this->angleX = angleX;
}

void SkyObjectInstance::Air::init(Radians angleX, Radians angleY)
{
	this->angleX = angleX;
	this->angleY = angleY;
}

void SkyObjectInstance::Sun::init(double bonusLatitude)
{
	this->bonusLatitude = bonusLatitude;
}

void SkyObjectInstance::Moon::init(double baseDirX, double baseDirY, double baseDirZ, double bonusLatitude,
	double phasePercent, int phaseCount, int phaseIndexDayOffset)
{
	this->baseDirX = baseDirX;
	this->baseDirY = baseDirY;
	this->baseDirZ = baseDirZ;
	this->bonusLatitude = bonusLatitude;
	this->phasePercent = phasePercent;
	this->phaseCount = phaseCount;
	this->phaseIndexDayOffset = phaseIndexDayOffset;
}

void SkyObjectInstance::Star::init(Radians angleX, Radians angleY)
{
	this->angleX = angleX;
	this->angleY = angleY;
}

SkyObjectInstance::SkyObjectInstance()
{
	this->curAnimSeconds = 0.0;
	this->curImageID = TextureManager::NO_ID;
	this->defIndex = -1;
}

void SkyObjectInstance::init(int skyObjectDefIndex)
{
	this->defIndex = skyObjectDefIndex;
}

void SkyObjectInstance::initLand(Radians angleX, int skyObjectDefIndex)
{
	this->init(skyObjectDefIndex);
	this->land.init(angleX);
}

void SkyObjectInstance::initAir(Radians angleX, Radians angleY, int skyObjectDefIndex)
{
	this->init(skyObjectDefIndex);
	this->air.init(angleX, angleY);
}

void SkyObjectInstance::initSun(double bonusLatitude, int skyObjectDefIndex)
{
	this->init(skyObjectDefIndex);
	this->sun.init(bonusLatitude);
}

void SkyObjectInstance::initMoon(double baseDirX, double baseDirY, double baseDirZ, double bonusLatitude,
	double phasePercent, int phaseCount, int phaseIndexDayOffset, int skyObjectDefIndex)
{
	this->init(skyObjectDefIndex);
	this->moon.init(baseDirX, baseDirY, baseDirZ, bonusLatitude, phasePercent,
		phaseCount, phaseIndexDayOffset);
}

void SkyObjectInstance::initStar(Radians angleX, Radians angleY, int skyObjectDefIndex)
{
	this->init(skyObjectDefIndex);
	this->star.init(angleX, angleY);
}

const Double3 &SkyObjectInstance::getCalculatedDirection() const
{
	return this->calculatedDir;
}

ImageID SkyObjectInstance::getImageID() const
{
	return this->curImageID;
}

int SkyObjectInstance::getDefIndex() const
{
	return this->defIndex;
}

void SkyObjectInstance::update(double dt, const SkyObjectDefinition &skyObjectDef)
{
	// @todo: eventually move most(?) of SoftwareRenderer::updateVisibleDistantObjects() in here;
	// specifically the renderer-agnostic parts that affect objects' calculated 3D directions.

	// @todo: probably need to pass latitude/time w/ their rotations?

	// @todo: calculate actual direction based on type
	// @todo: make sure image ID is up to date
	DebugNotImplemented();
	const SkyObjectDefinition::Type defType = skyObjectDef.getType();
	if (defType == SkyObjectDefinition::Type::Land)
	{

	}
	else if (defType == SkyObjectDefinition::Type::Air)
	{

	}
	else if (defType == SkyObjectDefinition::Type::Sun)
	{

	}
	else if (defType == SkyObjectDefinition::Type::Moon)
	{

	}
	else if (defType == SkyObjectDefinition::Type::Star)
	{

	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(defType)));
	}
}
