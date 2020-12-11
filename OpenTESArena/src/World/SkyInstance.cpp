#include <cmath>

#include "AirObjectDefinition.h"
#include "LandObjectDefinition.h"
#include "MoonObjectDefinition.h"
#include "SkyDefinition.h"
#include "SkyInfoDefinition.h"
#include "SkyInstance.h"
#include "StarObjectDefinition.h"
#include "SunObjectDefinition.h"

#include "components/debug/Debug.h"

SkyInstance::ObjectInstance::ObjectInstance(const Double3 &baseDirection, ImageID imageID, double width,
	double height)
	: baseDirection(baseDirection)
{
	this->imageID = imageID;
	this->width = width;
	this->height = height;
}

SkyInstance::AnimInstance::AnimInstance()
{
	this->objectIndex = -1;
	this->targetSeconds = 0.0;
	this->currentSeconds = 0.0;
}

void SkyInstance::AnimInstance::init(int objectIndex, const TextureUtils::ImageIdGroup &imageIDs,
	double targetSeconds)
{
	this->objectIndex = objectIndex;
	this->imageIDs = imageIDs;
	this->targetSeconds = targetSeconds;
	this->currentSeconds = 0.0;
}

void SkyInstance::init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition)
{
	auto addObjectInst = [this](const Double3 &baseDirection, ImageID imageID, double width, double height)
	{
		this->objectInsts.emplace_back(baseDirection, imageID, width, height);
	};

	// Spawn all sky objects from the ready-to-bake format.
	int landInstCount = 0;
	for (int i = 0; i < skyDefinition.getLandPlacementDefCount(); i++)
	{
		const SkyDefinition::LandPlacementDef &placementDef = skyDefinition.getLandPlacementDef(i);
		const SkyDefinition::LandDefID defID = placementDef.id;
		const LandObjectDefinition &objectDef = skyInfoDefinition.getLand(defID);

		for (const Radians position : placementDef.positions)
		{
			// Convert radians to direction.
			DebugNotImplemented();
		}

		landInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->landStart = 0;
	this->landEnd = this->landStart + landInstCount;

	int airInstCount = 0;
	for (int i = 0; i < skyDefinition.getAirPlacementDefCount(); i++)
	{
		const SkyDefinition::AirPlacementDef &placementDef = skyDefinition.getAirPlacementDef(i);
		const SkyDefinition::AirDefID defID = placementDef.id;
		const AirObjectDefinition &objectDef = skyInfoDefinition.getAir(defID);

		for (const std::pair<Radians, Radians> &position : placementDef.positions)
		{
			// Convert X and Y radians to direction.
			DebugNotImplemented();
		}

		airInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->airStart = this->landEnd;
	this->airEnd = this->airStart + airInstCount;

	int starInstCount = 0;
	for (int i = 0; i < skyDefinition.getStarPlacementDefCount(); i++)
	{
		const SkyDefinition::StarPlacementDef &placementDef = skyDefinition.getStarPlacementDef(i);
		const SkyDefinition::StarDefID defID = placementDef.id;
		const StarObjectDefinition &objectDef = skyInfoDefinition.getStar(defID);

		for (const Double3 &position : placementDef.positions)
		{
			// Use star direction directly.
			DebugNotImplemented();
		}

		starInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->starStart = this->airEnd;
	this->starEnd = this->starStart + starInstCount;

	int sunInstCount = 0;
	for (int i = 0; i < skyDefinition.getSunPlacementDefCount(); i++)
	{
		const SkyDefinition::SunPlacementDef &placementDef = skyDefinition.getSunPlacementDef(i);
		const SkyDefinition::SunDefID defID = placementDef.id;
		const SunObjectDefinition &objectDef = skyInfoDefinition.getSun(defID);

		for (const double position : placementDef.positions)
		{
			// Convert starting sun latitude to direction.
			DebugNotImplemented();
		}

		sunInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->sunStart = this->starEnd;
	this->sunEnd = this->sunStart + sunInstCount;

	int moonInstCount = 0;
	for (int i = 0; i < skyDefinition.getMoonPlacementDefCount(); i++)
	{
		const SkyDefinition::MoonPlacementDef &placementDef = skyDefinition.getMoonPlacementDef(i);
		const SkyDefinition::MoonDefID defID = placementDef.id;
		const MoonObjectDefinition &objectDef = skyInfoDefinition.getMoon(defID);

		for (const SkyDefinition::MoonPlacementDef::Position &position : placementDef.positions)
		{
			// Convert moon position to direction.
			DebugNotImplemented();
		}

		moonInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->moonStart = this->sunEnd;
	this->moonEnd = this->moonStart + moonInstCount;
}

int SkyInstance::getLandStartIndex() const
{
	return this->landStart;
}

int SkyInstance::getLandEndIndex() const
{
	return this->landEnd;
}

int SkyInstance::getAirStartIndex() const
{
	return this->airStart;
}

int SkyInstance::getAirEndIndex() const
{
	return this->airEnd;
}

int SkyInstance::getStarStartIndex() const
{
	return this->starStart;
}

int SkyInstance::getStarEndIndex() const
{
	return this->starEnd;
}

int SkyInstance::getSunStartIndex() const
{
	return this->sunStart;
}

int SkyInstance::getSunEndIndex() const
{
	return this->sunEnd;
}

int SkyInstance::getMoonStartIndex() const
{
	return this->moonStart;
}

int SkyInstance::getMoonEndIndex() const
{
	return this->moonEnd;
}

void SkyInstance::getObject(int index, Double3 *outDirection, ImageID *outImageID, double *outWidth,
	double *outHeight) const
{
	DebugAssertIndex(this->objectInsts, index);
	const ObjectInstance &objectInst = this->objectInsts[index];
	*outDirection = objectInst.transformedDirection;
	*outImageID = objectInst.imageID;
	*outWidth = objectInst.width;
	*outHeight = objectInst.height;
}

void SkyInstance::update(double dt, double latitude, double daytimePercent)
{
	// Update animations.
	const int animInstCount = static_cast<int>(this->animInsts.size());
	for (int i = 0; i < animInstCount; i++)
	{
		AnimInstance &animInst = this->animInsts[i];
		animInst.currentSeconds += dt;
		if (animInst.currentSeconds >= animInst.targetSeconds)
		{
			animInst.currentSeconds = std::fmod(animInst.currentSeconds, animInst.targetSeconds);
		}

		const int imageCount = animInst.imageIDs.getCount();
		const double animPercent = std::clamp(animInst.currentSeconds / animInst.targetSeconds, 0.0, 1.0);
		const int animIndex = static_cast<int>(static_cast<double>(imageCount) * animPercent);
		const ImageID newImageID = animInst.imageIDs.getID(animIndex);
		
		DebugAssertIndex(this->objectInsts, animInst.objectIndex);
		ObjectInstance &objectInst = this->objectInsts[animInst.objectIndex];
		objectInst.imageID = newImageID;
	}

	// Update transformed sky position of stars, suns, and moons.
	for (int i = this->starStart; i < this->starEnd; i++)
	{
		DebugNotImplemented();
	}

	for (int i = this->sunStart; i < this->sunEnd; i++)
	{
		DebugNotImplemented();
	}

	for (int i = this->moonStart; i < this->moonEnd; i++)
	{
		DebugNotImplemented();
	}
}
