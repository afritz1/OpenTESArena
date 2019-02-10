#include <cassert>

#include "ClimateType.h"
#include "DistantSky.h"
#include "Location.h"
#include "WeatherType.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

DistantSky::LandObject::LandObject(const Surface &surface, double angleRadians)
{
	this->surface = &surface;
	this->angleRadians = angleRadians;
}

const Surface &DistantSky::LandObject::getSurface() const
{
	return *this->surface;
}

double DistantSky::LandObject::getAngleRadians() const
{
	return this->angleRadians;
}

DistantSky::AnimatedLandObject::AnimatedLandObject(double angleRadians, double frameTime)
{
	// Frame time must be positive.
	assert(frameTime > 0.0);

	this->angleRadians = angleRadians;
	this->targetFrameTime = frameTime;
	this->currentFrameTime = 0.0;
	this->index = 0;
}

DistantSky::AnimatedLandObject::AnimatedLandObject(double angleRadians)
	: AnimatedLandObject(angleRadians, AnimatedLandObject::DEFAULT_FRAME_TIME) { }

int DistantSky::AnimatedLandObject::getSurfaceCount() const
{
	return static_cast<int>(this->surfaces.size());
}

const Surface &DistantSky::AnimatedLandObject::getSurface(int index) const
{
	return *this->surfaces.at(index);
}

double DistantSky::AnimatedLandObject::getAngleRadians() const
{
	return this->angleRadians;
}

double DistantSky::AnimatedLandObject::getFrameTime() const
{
	return this->targetFrameTime;
}

int DistantSky::AnimatedLandObject::getIndex() const
{
	return this->index;
}

void DistantSky::AnimatedLandObject::addSurface(const Surface &surface)
{
	this->surfaces.push_back(&surface);
}

void DistantSky::AnimatedLandObject::setFrameTime(double frameTime)
{
	// Frame time must be positive.
	assert(frameTime > 0.0);

	this->targetFrameTime = frameTime;
}

void DistantSky::AnimatedLandObject::setIndex(int index)
{
	this->index = index;
}

void DistantSky::AnimatedLandObject::update(double dt)
{
	// Must have at least one image.
	const int surfaceCount = this->getSurfaceCount();
	if (surfaceCount > 0)
	{
		// Animate based on delta time.
		this->currentFrameTime += dt;

		while (this->currentFrameTime >= this->targetFrameTime)
		{
			this->currentFrameTime -= this->targetFrameTime;
			this->index = (this->index < (surfaceCount - 1)) ? (this->index + 1) : 0;
		}
	}
}

DistantSky::AirObject::AirObject(const Surface &surface, double angleRadians, double height)
{
	this->surface = &surface;
	this->angleRadians = angleRadians;
	this->height = height;
}

const Surface &DistantSky::AirObject::getSurface() const
{
	return *this->surface;
}

double DistantSky::AirObject::getAngleRadians() const
{
	return this->angleRadians;
}

double DistantSky::AirObject::getHeight() const
{
	return this->height;
}

DistantSky::SpaceObject::SpaceObject(const Surface &surface, const Double3 &direction)
{
	this->surface = &surface;
	this->direction = direction;
}

const Surface &DistantSky::SpaceObject::getSurface() const
{
	return *this->surface;
}

const Double3 &DistantSky::SpaceObject::getDirection() const
{
	return this->direction;
}

DistantSky::DistantSky()
{
	this->sunSurface = nullptr;
}

int DistantSky::getLandObjectCount() const
{
	return static_cast<int>(this->landObjects.size());
}

int DistantSky::getAnimatedLandObjectCount() const
{
	return static_cast<int>(this->animLandObjects.size());
}

int DistantSky::getAirObjectCount() const
{
	return static_cast<int>(this->airObjects.size());
}

int DistantSky::getSpaceObjectCount() const
{
	return static_cast<int>(this->spaceObjects.size());
}

const DistantSky::LandObject &DistantSky::getLandObject(int index) const
{
	return this->landObjects.at(index);
}

const DistantSky::AnimatedLandObject &DistantSky::getAnimatedLandObject(int index) const
{
	return this->animLandObjects.at(index);
}

const DistantSky::AirObject &DistantSky::getAirObject(int index) const
{
	return this->airObjects.at(index);
}

const DistantSky::SpaceObject &DistantSky::getSpaceObject(int index) const
{
	return this->spaceObjects.at(index);
}

const Surface &DistantSky::getSunSurface() const
{
	return *this->sunSurface;
}

void DistantSky::init(int localCityID, int provinceID, WeatherType weatherType,
	int currentDay, const MiscAssets &miscAssets, TextureManager &textureManager)
{
	// Add mountains and clouds first. Get the climate type of the city.
	const ClimateType climateType = Location::getCityClimateType(
		localCityID, provinceID, miscAssets);

	const auto &exeData = miscAssets.getExeData();
	const auto &distantMountainFilenames = exeData.locations.distantMountainFilenames;

	std::string baseFilename;
	int pos;
	int var;
	int maxDigits;

	// Decide the base image filename, etc. based on which climate the city is in.
	if (climateType == ClimateType::Temperate)
	{
		baseFilename = distantMountainFilenames.at(2);
		pos = 4;
		var = 10;
		maxDigits = 2;
	}
	else if (climateType == ClimateType::Desert)
	{
		baseFilename = distantMountainFilenames.at(1);
		pos = 6;
		var = 4;
		maxDigits = 1;
	}
	else if (climateType == ClimateType::Mountain)
	{
		baseFilename = distantMountainFilenames.at(0);
		pos = 6;
		var = 11;
		maxDigits = 2;
	}
	else
	{
		throw DebugException("Invalid climate type \"" +
			std::to_string(static_cast<int>(climateType)) + "\".");
	}

	const auto &cityDataFile = miscAssets.getCityDataFile();
	const uint32_t citySeed = cityDataFile.getCitySeed(localCityID, provinceID);
	ArenaRandom random(citySeed);
	const int count = (random.next() % 4) + 2;

	// Lambda for creating images with certain sky parameters.
	auto placeStaticObjects = [this, &textureManager, &random](int count,
		const std::string &baseFilename, int pos, int var, int maxDigits, bool randomHeight)
	{
		for (int i = 0; i < count; i++)
		{
			// Digits for the filename variant. Allowed up to two digits.
			const std::string digits = [&random, var]()
			{
				const int randVal = random.next() % var;
				return std::to_string((randVal == 0) ? var : randVal);
			}();

			assert(digits.size() <= maxDigits);

			// Actual filename for the image.
			const std::string filename = [&baseFilename, pos, maxDigits, &digits]()
			{
				std::string name = baseFilename;
				const int digitCount = static_cast<int>(digits.size());

				// Push the starting position right depending on the max digits.
				const int offset = maxDigits - digitCount;

				for (size_t index = 0; index < digitCount; index++)
				{
					name.at(pos + offset + index) = digits.at(index);
				}

				return String::toUppercase(name);
			}();

			const Surface &surface = textureManager.getSurface(filename);

			// The yPos parameter is optional, and is assigned depending on whether the object
			// is in the air.
			constexpr int yPosLimit = 64;
			const int yPos = randomHeight ? (random.next() % yPosLimit) : 0;

			// Convert from Arena units to radians.
			constexpr int arenaAngleLimit = 512;
			const int arenaAngle = random.next() % arenaAngleLimit;
			const double angleRadians = (Constants::Pi * 2.0) *
				(static_cast<double>(arenaAngle) / static_cast<double>(arenaAngleLimit));

			// The object is either land or a cloud, currently determined by 'randomHeight' as
			// a shortcut. Land objects have no height. I'm doing it this way because LandObject
			// and AirObject are two different types.
			const bool isLand = !randomHeight;

			if (isLand)
			{
				this->landObjects.push_back(LandObject(surface, angleRadians));
			}
			else
			{
				const double height = static_cast<double>(yPos) / static_cast<double>(yPosLimit);
				this->airObjects.push_back(AirObject(surface, angleRadians, height));
			}
		}
	};

	// Initial set of statics based on the climate.
	placeStaticObjects(count, baseFilename, pos, var, maxDigits, false);

	// If the weather is clear, add clouds.
	if (weatherType == WeatherType::Clear)
	{
		const uint32_t cloudSeed = random.getSeed() + (currentDay % 32);
		random.srand(cloudSeed);

		const int cloudCount = 7;
		const std::string &cloudFilename = exeData.locations.cloudFilename;
		const int cloudPos = 5;
		const int cloudVar = 17;
		const int cloudMaxDigits = 2;
		placeStaticObjects(cloudCount, cloudFilename, cloudPos, cloudVar, cloudMaxDigits, true);
	}

	// Other initializations (animated land, stars).
	// @todo

	const std::string &sunFilename = exeData.locations.sunFilename;
	this->sunSurface = &textureManager.getSurface(String::toUppercase(sunFilename));
}

void DistantSky::update(double dt)
{
	// Only animated distant land needs updating.
	for (auto &anim : this->animLandObjects)
	{
		anim.update(dt);
	}
}
