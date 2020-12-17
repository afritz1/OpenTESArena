#include <cmath>

#include "AirObjectDefinition.h"
#include "ArenaSkyUtils.h"
#include "LandObjectDefinition.h"
#include "MoonObjectDefinition.h"
#include "SkyDefinition.h"
#include "SkyInfoDefinition.h"
#include "SkyInstance.h"
#include "SkyUtils.h"
#include "StarObjectDefinition.h"
#include "SunObjectDefinition.h"

#include "components/debug/Debug.h"

SkyInstance::ObjectInstance::ObjectInstance()
{
	this->type = static_cast<ObjectInstance::Type>(-1);
	this->width = 0.0;
	this->height = 0.0;
}

void SkyInstance::ObjectInstance::init(Type type, const Double3 &baseDirection, double width, double height)
{
	this->type = type;
	this->baseDirection = baseDirection;
	this->width = width;
	this->height = height;
}

void SkyInstance::ObjectInstance::initGeneral(const Double3 &baseDirection, double width, double height,
	ImageID imageID)
{
	this->init(ObjectInstance::Type::General, baseDirection, width, height);
	this->general.imageID = imageID;
}

void SkyInstance::ObjectInstance::initSmallStar(const Double3 &baseDirection, double width, double height,
	uint8_t paletteIndex)
{
	this->init(ObjectInstance::Type::SmallStar, baseDirection, width, height);
	this->smallStar.paletteIndex = paletteIndex;
}

SkyInstance::ObjectInstance::Type SkyInstance::ObjectInstance::getType() const
{
	return this->type;
}

const Double3 &SkyInstance::ObjectInstance::getBaseDirection() const
{
	return this->baseDirection;
}

const Double3 &SkyInstance::ObjectInstance::getTransformedDirection() const
{
	return this->transformedDirection;
}

double SkyInstance::ObjectInstance::getWidth() const
{
	return this->width;
}

double SkyInstance::ObjectInstance::getHeight() const
{
	return this->height;
}

SkyInstance::ObjectInstance::General &SkyInstance::ObjectInstance::getGeneral()
{
	DebugAssert(this->type == ObjectInstance::Type::General);
	return this->general;
}

const SkyInstance::ObjectInstance::General &SkyInstance::ObjectInstance::getGeneral() const
{
	DebugAssert(this->type == ObjectInstance::Type::General);
	return this->general;
}

const SkyInstance::ObjectInstance::SmallStar &SkyInstance::ObjectInstance::getSmallStar() const
{
	DebugAssert(this->type == ObjectInstance::Type::SmallStar);
	return this->smallStar;
}

void SkyInstance::ObjectInstance::setTransformedDirection(const Double3 &direction)
{
	this->transformedDirection = direction;
}

SkyInstance::AnimInstance::AnimInstance(int objectIndex, const TextureUtils::ImageIdGroup &imageIDs,
	double targetSeconds)
{
	this->objectIndex = objectIndex;
	this->imageIDs = imageIDs;
	this->targetSeconds = targetSeconds;
	this->currentSeconds = 0.0;
}

void SkyInstance::init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition,
	const TextureManager &textureManager)
{
	auto addGeneralObjectInst = [this](const Double3 &baseDirection, double width, double height,
		ImageID imageID)
	{
		ObjectInstance objectInst;
		objectInst.initGeneral(baseDirection, width, height, imageID);
		this->objectInsts.emplace_back(std::move(objectInst));
	};

	auto addSmallStarObjectInst = [this](const Double3 &baseDirection, double width, double height,
		uint8_t paletteIndex)
	{
		ObjectInstance objectInst;
		objectInst.initSmallStar(baseDirection, width, height, paletteIndex);
		this->objectInsts.emplace_back(std::move(objectInst));
	};

	auto addAnimInst = [this](int objectIndex, const TextureUtils::ImageIdGroup &imageIDs,
		double targetSeconds)
	{
		this->animInsts.emplace_back(objectIndex, imageIDs, targetSeconds);
	};

	// Spawn all sky objects from the ready-to-bake format. Any animated objects start on their first frame.
	int landInstCount = 0;
	for (int i = 0; i < skyDefinition.getLandPlacementDefCount(); i++)
	{
		const SkyDefinition::LandPlacementDef &placementDef = skyDefinition.getLandPlacementDef(i);
		const SkyDefinition::LandDefID defID = placementDef.id;
		const LandObjectDefinition &objectDef = skyInfoDefinition.getLand(defID);

		DebugAssert(objectDef.getImageCount() > 0);
		const ImageID imageID = objectDef.getImageID(0);
		const Image &image = textureManager.getImageHandle(imageID);

		double width, height;
		SkyUtils::getSkyObjectDimensions(image.getWidth(), image.getHeight(), &width, &height);

		for (const Radians position : placementDef.positions)
		{
			// Convert radians to direction.
			const Radians angleY = 0.0;
			const Double3 direction = SkyUtils::getSkyObjectDirection(position, angleY);
			addGeneralObjectInst(direction, width, height, imageID);

			// Only land objects support animations (for now).
			if (objectDef.hasAnimation())
			{
				const int objectIndex = static_cast<int>(this->objectInsts.size()) - 1;
				const TextureUtils::ImageIdGroup imageIDs(objectDef.getImageID(0), objectDef.getImageCount());
				const double targetSeconds = static_cast<int>(static_cast<double>(imageIDs.getCount()) *
					ArenaSkyUtils::ANIMATED_LAND_SECONDS_PER_FRAME);
				addAnimInst(objectIndex, imageIDs, targetSeconds);
			}
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
		const ImageID imageID = objectDef.getImageID();
		const Image &image = textureManager.getImageHandle(imageID);

		double width, height;
		SkyUtils::getSkyObjectDimensions(image.getWidth(), image.getHeight(), &width, &height);

		for (const std::pair<Radians, Radians> &position : placementDef.positions)
		{
			// Convert X and Y radians to direction.
			const Radians angleX = position.first;
			const Radians angleY = position.second;
			const Double3 direction = SkyUtils::getSkyObjectDirection(angleX, angleY);
			addGeneralObjectInst(direction, width, height, imageID);
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

		// @todo: this is where the imageID design is kind of breaking, and getting a renderer sprite
		// resource ID would be better. SkyInstance::init() should be able to allocate textures IDs from
		// the renderer eventually, and look up cached ones by string.
		const StarObjectDefinition::Type starType = objectDef.getType();
		if (starType == StarObjectDefinition::Type::Small)
		{
			// Small stars are 1x1 pixels.
			const StarObjectDefinition::SmallStar &smallStar = objectDef.getSmallStar();
			const uint8_t paletteIndex = smallStar.paletteIndex;
			constexpr int imageWidth = 1;
			constexpr int imageHeight = imageWidth;

			double width, height;
			SkyUtils::getSkyObjectDimensions(imageWidth, imageHeight, &width, &height);

			for (const Double3 &position : placementDef.positions)
			{
				// Use star direction directly.
				addSmallStarObjectInst(position, width, height, paletteIndex);
			}
		}
		else if (starType == StarObjectDefinition::Type::Large)
		{
			const StarObjectDefinition::LargeStar &largeStar = objectDef.getLargeStar();
			const ImageID imageID = largeStar.imageID;
			const Image &image = textureManager.getImageHandle(imageID);

			double width, height;
			SkyUtils::getSkyObjectDimensions(image.getWidth(), image.getHeight(), &width, &height);

			for (const Double3 &position : placementDef.positions)
			{
				// Use star direction directly.
				addGeneralObjectInst(position, width, height, imageID);
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(starType)));
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
		const ImageID imageID = objectDef.getImageID();
		const Image &image = textureManager.getImageHandle(imageID);

		double width, height;
		SkyUtils::getSkyObjectDimensions(image.getWidth(), image.getHeight(), &width, &height);

		for (const double position : placementDef.positions)
		{
			// Convert starting sun latitude to direction.
			// @todo: just use fixed direction for now, see RendererUtils later.
			const Double3 tempDirection = Double3::UnitZ; // Temp: west. Ideally this would be -Y and rotated around +X (south).
			addGeneralObjectInst(tempDirection, width, height, imageID);
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

		// @todo: get the image from the current day, etc..
		DebugAssert(objectDef.getImageIdCount() > 0);
		const ImageID imageID = objectDef.getImageID(0);
		const Image &image = textureManager.getImageHandle(imageID);

		double width, height;
		SkyUtils::getSkyObjectDimensions(image.getWidth(), image.getHeight(), &width, &height);

		for (const SkyDefinition::MoonPlacementDef::Position &position : placementDef.positions)
		{
			// Convert moon position to direction.
			// @todo: just use fixed direction for now, see RendererUtils later.
			const Double3 tempDirection = Double3::UnitZ; // Temp: west. Ideally this would be -Y and rotated around +X (south).
			addGeneralObjectInst(tempDirection, width, height, imageID);
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

bool SkyInstance::isObjectSmallStar(int objectIndex) const
{
	DebugAssertIndex(this->objectInsts, objectIndex);
	return this->objectInsts[objectIndex].getType() != SkyInstance::ObjectInstance::Type::SmallStar;
}

void SkyInstance::getObject(int index, Double3 *outDirection, ImageID *outImageID, double *outWidth,
	double *outHeight) const
{
	DebugAssertIndex(this->objectInsts, index);
	const ObjectInstance &objectInst = this->objectInsts[index];
	*outDirection = objectInst.getTransformedDirection();

	DebugAssert(objectInst.getType() == SkyInstance::ObjectInstance::Type::General);
	*outImageID = objectInst.getGeneral().imageID;

	*outWidth = objectInst.getWidth();
	*outHeight = objectInst.getHeight();
}

void SkyInstance::getObjectSmallStar(int index, Double3 *outDirection, uint8_t *outPaletteIndex,
	double *outWidth, double *outHeight) const
{
	DebugAssertIndex(this->objectInsts, index);
	const ObjectInstance &objectInst = this->objectInsts[index];
	*outDirection = objectInst.getTransformedDirection();

	DebugAssert(objectInst.getType() == SkyInstance::ObjectInstance::Type::SmallStar);
	*outPaletteIndex = objectInst.getSmallStar().paletteIndex;

	*outWidth = objectInst.getWidth();
	*outHeight = objectInst.getHeight();
}

void SkyInstance::update(double dt, double latitude, double daytimePercent)
{
	// Update animations.
	const int animInstCount = static_cast<int>(this->animInsts.size());
	for (int i = 0; i < animInstCount; i++)
	{
		AnimInstance &animInst = this->animInsts[i];

		// Small stars don't have animations.
		if (this->isObjectSmallStar(animInst.objectIndex))
		{
			continue;
		}

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

		DebugAssert(objectInst.getType() == SkyInstance::ObjectInstance::Type::General);
		ObjectInstance::General &objectInstGeneral = objectInst.getGeneral();
		objectInstGeneral.imageID = newImageID;
	}

	// Update transformed sky position of stars, suns, and moons.
	for (int i = this->starStart; i < this->starEnd; i++)
	{
		DebugAssertIndex(this->objectInsts, i);
		ObjectInstance &objectInst = this->objectInsts[i];
		// @todo: actually transform direction based on latitude and time of day.
		const Double3 transformedDirection = objectInst.getBaseDirection();
		objectInst.setTransformedDirection(transformedDirection);
	}

	for (int i = this->sunStart; i < this->sunEnd; i++)
	{
		DebugAssertIndex(this->objectInsts, i);
		ObjectInstance &objectInst = this->objectInsts[i];
		// @todo: actually transform direction based on latitude and time of day.
		const Double3 transformedDirection = objectInst.getBaseDirection();
		objectInst.setTransformedDirection(transformedDirection);
	}

	for (int i = this->moonStart; i < this->moonEnd; i++)
	{
		DebugAssertIndex(this->objectInsts, i);
		ObjectInstance &objectInst = this->objectInsts[i];
		// @todo: actually transform direction based on latitude and time of day.
		const Double3 transformedDirection = objectInst.getBaseDirection();
		objectInst.setTransformedDirection(transformedDirection);
	}
}