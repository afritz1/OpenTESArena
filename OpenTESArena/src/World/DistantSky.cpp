#include <algorithm>
#include <array>

#include "ClimateType.h"
#include "DistantSky.h"
#include "Location.h"
#include "WeatherType.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/COLFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/MiscAssets.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Random.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Surface.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	struct DistantMountainTraits
	{
		int filenameIndex; // Index into ExeData mountain filenames.
		int position;
		int variation;
		int maxDigits; // Max number of digits in the filename for the variation.

		DistantMountainTraits(int filenameIndex, int position, int variation, int maxDigits)
		{
			this->filenameIndex = filenameIndex;
			this->position = position;
			this->variation = variation;
			this->maxDigits = maxDigits;
		}
	};

	const std::array<std::pair<ClimateType, DistantMountainTraits>, 3> MountainTraits =
	{
		{
			{ ClimateType::Temperate, DistantMountainTraits(2, 4, 10, 2) },
			{ ClimateType::Desert, DistantMountainTraits(1, 6, 4, 1) },
			{ ClimateType::Mountain, DistantMountainTraits(0, 6, 11, 2) }
		}
	};
}

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
	DebugAssert(frameTime > 0.0);

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
	DebugAssert(frameTime > 0.0);

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

DistantSky::MoonObject::MoonObject(const Surface &surface, double phasePercent,
	DistantSky::MoonObject::Type type)
{
	this->surface = &surface;
	this->phasePercent = phasePercent;
	this->type = type;
}

const Surface &DistantSky::MoonObject::getSurface() const
{
	return *this->surface;
}

double DistantSky::MoonObject::getPhasePercent() const
{
	return this->phasePercent;
}

DistantSky::MoonObject::Type DistantSky::MoonObject::getType() const
{
	return this->type;
}

DistantSky::StarObject DistantSky::StarObject::makeSmall(uint32_t color, const Double3 &direction)
{
	StarObject star;
	star.type = StarObject::Type::Small;

	StarObject::SmallStar &smallStar = star.small;
	smallStar.color = color;

	star.direction = direction;
	return star;
}

DistantSky::StarObject DistantSky::StarObject::makeLarge(const Surface &surface, const Double3 &direction)
{
	StarObject star;
	star.type = StarObject::Type::Large;

	StarObject::LargeStar &largeStar = star.large;
	largeStar.surface = &surface;

	star.direction = direction;
	return star;
}

DistantSky::StarObject::Type DistantSky::StarObject::getType() const
{
	return this->type;
}

const DistantSky::StarObject::SmallStar &DistantSky::StarObject::getSmallStar() const
{
	DebugAssert(this->type == StarObject::Type::Small);
	return this->small;
}

const DistantSky::StarObject::LargeStar &DistantSky::StarObject::getLargeStar() const
{
	DebugAssert(this->type == StarObject::Type::Large);
	return this->large;
}

const Double3 &DistantSky::StarObject::getDirection() const
{
	return this->direction;
}

const int DistantSky::UNIQUE_ANGLES = 512;
const double DistantSky::IDENTITY_DIM = 320.0;
const double DistantSky::IDENTITY_ANGLE_RADIANS = 90.0 * Constants::DegToRad;

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

int DistantSky::getMoonObjectCount() const
{
	return static_cast<int>(this->moonObjects.size());
}

int DistantSky::getStarObjectCount() const
{
	return static_cast<int>(this->starObjects.size());
}

bool DistantSky::hasSun() const
{
	return this->sunSurface != nullptr;
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

const DistantSky::MoonObject &DistantSky::getMoonObject(int index) const
{
	return this->moonObjects.at(index);
}

const DistantSky::StarObject &DistantSky::getStarObject(int index) const
{
	return this->starObjects.at(index);
}

const Surface &DistantSky::getSunSurface() const
{
	return *this->sunSurface;
}

int DistantSky::getStarCountFromDensity(int starDensity)
{
	if (starDensity == 0)
	{
		// Classic.
		return 40;
	}
	else if (starDensity == 1)
	{
		// Moderate.
		return 1000;
	}
	else if (starDensity == 2)
	{
		// High.
		return 8000;
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(starDensity));
	}
}

void DistantSky::init(int localCityID, int provinceID, WeatherType weatherType,
	int currentDay, int starCount, const MiscAssets &miscAssets, TextureManager &textureManager)
{
	// Add mountains and clouds first. Get the climate type of the city.
	const ClimateType climateType = Location::getCityClimateType(
		localCityID, provinceID, miscAssets);

	const auto &exeData = miscAssets.getExeData();
	const auto &distantMountainFilenames = exeData.locations.distantMountainFilenames;

	// Get the mountain traits associated with the given climate type.
	const DistantMountainTraits &mtnTraits = [climateType]()
	{
		const auto iter = std::find_if(MountainTraits.begin(), MountainTraits.end(),
			[climateType](const auto &pair)
		{
			return pair.first == climateType;
		});

		DebugAssertMsg(iter != MountainTraits.end(), "Invalid climate type \"" +
			std::to_string(static_cast<int>(climateType)) + "\".");

		return iter->second;
	}();

	const std::string &baseFilename = distantMountainFilenames.at(mtnTraits.filenameIndex);
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

			DebugAssert(digits.size() <= maxDigits);

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
	placeStaticObjects(count, baseFilename, mtnTraits.position, mtnTraits.variation,
		mtnTraits.maxDigits, false);

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

	// Add space objects if the weather conditions are permitting.
	const bool hasSpaceObjects = weatherType == WeatherType::Clear;

	if (hasSpaceObjects)
	{
		// Initialize moons.
		auto makeMoon = [currentDay, &textureManager, &exeData](MoonObject::Type type)
		{
			const int phaseCount = 32;
			const int phaseIndex = [currentDay, type, phaseCount]()
			{
				if (type == MoonObject::Type::First)
				{
					return currentDay % phaseCount;
				}
				else if (type == MoonObject::Type::Second)
				{
					return (currentDay + 14) % phaseCount;
				}
				else
				{
					DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(type)));
				}
			}();

			const int moonIndex = static_cast<int>(type);
			const std::string filename = String::toUppercase(
				exeData.locations.moonFilenames.at(moonIndex));
			const auto &surfaces = textureManager.getSurfaces(filename);
			const auto &surface = surfaces.at(phaseIndex);
			const double phasePercent = static_cast<double>(phaseIndex) /
				static_cast<double>(phaseCount);
			return MoonObject(surface, phasePercent, type);
		};

		this->moonObjects.push_back(makeMoon(MoonObject::Type::First));
		this->moonObjects.push_back(makeMoon(MoonObject::Type::Second));

		// Initialize stars.
		struct SubStar
		{
			int8_t dx, dy;
			uint8_t color;
		};

		struct Star
		{
			int16_t x, y, z;
			std::vector<SubStar> subList;
			int8_t type;
		};

		const int8_t NO_STAR_TYPE = -1;

		auto getRndCoord = [&random]()
		{
			const int16_t d = (0x800 + random.next()) & 0x0FFF;
			return ((d & 2) == 0) ? d : -d;
		};

		std::vector<Star> stars;
		std::array<bool, 3> planets = { false, false, false };

		random.srand(0x12345679);

		// The original game is hardcoded to 40 stars but it doesn't seem like very many, so
		// it is now a variable.
		for (int i = 0; i < starCount; i++)
		{
			Star star;
			star.x = getRndCoord();
			star.y = getRndCoord();
			star.z = getRndCoord();
			star.type = NO_STAR_TYPE;

			const uint8_t selection = random.next() % 4;
			if (selection != 0)
			{
				// Constellation.
				std::vector<SubStar> starList;
				const int n = 2 + (random.next() % 4);

				for (int j = 0; j < n; j++)
				{
					// Must convert to short for arithmetic right shift (to preserve sign bit).
					SubStar subStar;
					subStar.dx = static_cast<int16_t>(random.next()) >> 9;
					subStar.dy = static_cast<int16_t>(random.next()) >> 9;
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

		// Sort stars so large ones appear in front when rendered (it looks a bit better that way).
		std::sort(stars.begin(), stars.end(), [](const Star &a, const Star &b)
		{
			return a.type < b.type;
		});

		// Palette used to obtain colors for small stars in constellations.
		const Palette palette = []()
		{
			const std::string &colName = PaletteFile::fromName(PaletteName::Default);
			COLFile colFile;
			if (!colFile.init(colName.c_str()))
			{
				DebugCrash("Could not init .COL file \"" + colName + "\".");
			}

			return colFile.getPalette();
		}();

		// Convert stars to modern representation.
		for (const auto &star : stars)
		{
			const Double3 direction = Double3(
				static_cast<double>(star.x),
				static_cast<double>(star.y),
				static_cast<double>(star.z)).normalized();

			const bool isSmallStar = star.type == -1;
			if (isSmallStar)
			{
				for (const auto &subStar : star.subList)
				{
					const uint32_t color = [&palette, &subStar]()
					{
						const Color &paletteColor = palette.get().at(subStar.color);
						return paletteColor.toARGB();
					}();

					// Delta X and Y are applied after world-to-pixel projection of the base
					// direction in the original game, but we're doing angle calculations here
					// instead for the sake of keeping all the star generation code in one place.
					const Double3 subDirection = [&direction, &subStar]()
					{
						// Convert delta X and Y to percentages of the identity dimension (320px).
						const double dxPercent = static_cast<double>(subStar.dx) / DistantSky::IDENTITY_DIM;
						const double dyPercent = static_cast<double>(subStar.dy) / DistantSky::IDENTITY_DIM;

						// Convert percentages to radians. Positive X is counter-clockwise, positive
						// Y is up.
						const double dxRadians = dxPercent * DistantSky::IDENTITY_ANGLE_RADIANS;
						const double dyRadians = dyPercent * DistantSky::IDENTITY_ANGLE_RADIANS;

						// Apply rotations to base direction.
						const Matrix4d xRotation = Matrix4d::xRotation(dxRadians);
						const Matrix4d yRotation = Matrix4d::yRotation(dyRadians);
						const Double4 newDir = yRotation * (xRotation * Double4(direction, 0.0));

						return Double3(newDir.x, newDir.y, newDir.z);
					}();

					this->starObjects.push_back(StarObject::makeSmall(color, subDirection));
				}
			}
			else
			{
				const std::string starFilename = [&exeData, &star]()
				{
					const std::string typeStr = std::to_string(star.type + 1);
					std::string filename = exeData.locations.starFilename;
					const size_t index = filename.find('1');
					DebugAssert(index != std::string::npos);

					filename.replace(index, 1, typeStr);
					return String::toUppercase(filename);
				}();

				const Surface &surface = textureManager.getSurface(starFilename);
				this->starObjects.push_back(StarObject::makeLarge(surface, direction));
			}
		}

		// Initialize sun texture.
		const std::string &sunFilename = exeData.locations.sunFilename;
		this->sunSurface = &textureManager.getSurface(String::toUppercase(sunFilename));
	}
}

void DistantSky::tick(double dt)
{
	// Only animated distant land needs updating.
	for (auto &anim : this->animLandObjects)
	{
		anim.update(dt);
	}
}
