#include <array>
#include <cassert>
#include <type_traits>

#include "ClimateType.h"
#include "DistantSky.h"
#include "Location.h"
#include "WeatherType.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
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

const int DistantSky::UNIQUE_ANGLES = 512;

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
	const uint32_t skySeed = cityDataFile.getDistantSkySeed(localCityID, provinceID);
	ArenaRandom random(skySeed);
	const int count = (random.next() % 4) + 2;

	// Converts an Arena angle to an actual angle in radians.
	auto arenaAngleToRadians = [](int angle)
	{
		// Arena angles: 0 = south, 128 = west, 256 = north, 384 = east.
		// Change from clockwise to counter-clockwise and move 0 to east.
		const double arenaRadians = Constants::TwoPi *
			(static_cast<double>(angle) / static_cast<double>(DistantSky::UNIQUE_ANGLES));
		const double flippedArenaRadians = Constants::TwoPi - arenaRadians;
		return flippedArenaRadians - Constants::HalfPi;
	};

	// Lambda for creating images with certain sky parameters.
	auto placeStaticObjects = [this, &textureManager, &random, &arenaAngleToRadians](int count,
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
			const int arenaAngle = random.next() % DistantSky::UNIQUE_ANGLES;
			const double angleRadians = arenaAngleToRadians(arenaAngle);

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

	// Add clouds if the weather conditions are permitting.
	const bool hasClouds = weatherType == WeatherType::Clear;
	if (hasClouds)
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

	// Initialize animated lands (if any).
	const bool hasAnimLand = provinceID == 3;
	if (hasAnimLand)
	{
		const uint32_t citySeed = cityDataFile.getCitySeed(localCityID, provinceID);

		// Position of animated land on province map; determines where it is on the horizon
		// for each location.
		const Int2 animLandGlobalPos(132, 52);
		const Int2 locationGlobalPos = cityDataFile.getLocalCityPoint(citySeed);

		// Distance on province map from current location to the animated land.
		const int dist = CityDataFile::getDistance(locationGlobalPos, animLandGlobalPos);

		// Position of the animated land on the horizon.
		const double angle = std::atan2(
			static_cast<double>(locationGlobalPos.y - animLandGlobalPos.y),
			static_cast<double>(animLandGlobalPos.x - locationGlobalPos.x));

		// Use different animations based on the map distance.
		const int animIndex = [dist]()
		{
			if (dist < 80)
			{
				return 0;
			}
			else if (dist < 150)
			{
				return 1;
			}
			else
			{
				return 2;
			}
		}();

		const auto &animFilenames = exeData.locations.animDistantMountainFilenames;
		const std::string animFilename = String::toUppercase(animFilenames.at(animIndex));

		// .DFAs have multiple frames, .IMGs do not.
		const bool hasMultipleFrames = animFilename.find(".DFA") != std::string::npos;

		AnimatedLandObject animLandObj(angle);

		// Determine which frames the animation will have.
		if (hasMultipleFrames)
		{
			const auto &animSurfaces = textureManager.getSurfaces(animFilename);
			for (auto &surface : animSurfaces)
			{
				animLandObj.addSurface(surface);
			}
		}
		else
		{
			const auto &surface = textureManager.getSurface(animFilename);
			animLandObj.addSurface(surface);
		}

		this->animLandObjects.push_back(std::move(animLandObj));
	}

	// Initialize stars.
	struct SubStar
	{
		int16_t dx, dy;
		uint8_t color;
	};

	struct Star
	{
		int16_t x, y, z;
		std::vector<SubStar> subList;
		int8_t type;
	};

	auto getRndCoord = [&random]()
	{
		const int16_t d = (0x800 + random.next()) & 0x0FFF;
		return ((d & 2) == 0) ? d : -d;
	};

	std::vector<Star> stars;
	std::array<bool, 3> planets = { false, false, false };

	random.srand(0x12345679);

	const int starCount = 40;
	for (int i = 0; i < starCount; i++)
	{
		Star star;
		star.x = getRndCoord();
		star.y = getRndCoord();
		star.z = getRndCoord();
		star.type = -1;

		const uint8_t selection = random.next() % 4;
		if (selection != 0)
		{
			// Constellation.
			std::vector<SubStar> starList;
			const int n = 2 + (random.next() % 4);

			for (int j = 0; j < n; j++)
			{
				// Must use arithmetic right shift, not logical right shift.
				// C++ only does arithmetic right shift when the value is signed.
				static_assert(std::is_signed<decltype(random.next())>::value, "Value not signed.");

				SubStar subStar;
				subStar.dx = random.next() >> 9;
				subStar.dy = random.next() >> 9;
				subStar.color = (random.next() % 10) + 64;
				starList.push_back(std::move(subStar));
			}

			star.subList = std::move(starList);
		}
		else
		{
			// Large star.
			int8_t value;
			do
			{
				value = random.next() % 8;
			} while ((value >= 5) && planets.at(value - 5));

			if (value >= 5)
			{
				planets.at(value - 5) = true;
			}

			star.type = value;
		}

		stars.push_back(std::move(star));
	}

	// @todo: convert stars to modern representation.
	// @todo: moons

	// Initialize sun texture.
	const std::string &sunFilename = exeData.locations.sunFilename;
	this->sunSurface = &textureManager.getSurface(String::toUppercase(sunFilename));
}

void DistantSky::tick(double dt)
{
	// Only animated distant land needs updating.
	for (auto &anim : this->animLandObjects)
	{
		anim.update(dt);
	}
}
