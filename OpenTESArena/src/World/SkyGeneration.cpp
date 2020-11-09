#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

#include "ClimateType.h"
#include "LocationUtils.h"
#include "SkyDefinition.h"
#include "SkyGeneration.h"
#include "SkyInfoDefinition.h"
#include "WeatherType.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Media/Color.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace SkyGeneration
{
	// Original game values.
	constexpr int UNIQUE_ANGLES = 512;
	constexpr double IDENTITY_DIM = 320.0;
	constexpr Radians IDENTITY_ANGLE = 90.0 * Constants::DegToRad;

	// Helper struct for original game's distant land.
	struct ArenaLandTraits
	{
		int filenameIndex; // Index into ExeData mountain filenames.
		int position;
		int variation;
		int maxDigits; // Max number of digits in the filename for the variation.

		ArenaLandTraits(int filenameIndex, int position, int variation, int maxDigits)
		{
			this->filenameIndex = filenameIndex;
			this->position = position;
			this->variation = variation;
			this->maxDigits = maxDigits;
		}
	};

	const std::array<std::pair<ClimateType, ArenaLandTraits>, 3> ArenaLandTraitsMappings =
	{
		{
			{ ClimateType::Temperate, ArenaLandTraits(2, 4, 10, 2) },
			{ ClimateType::Desert, ArenaLandTraits(1, 6, 4, 1) },
			{ ClimateType::Mountain, ArenaLandTraits(0, 6, 11, 2) }
		}
	};

	const ArenaLandTraits &getArenaLandTraits(ClimateType climateType)
	{
		const auto iter = std::find_if(ArenaLandTraitsMappings.begin(), ArenaLandTraitsMappings.end(),
			[climateType](const auto &pair)
		{
			return pair.first == climateType;
		});

		DebugAssertMsg(iter != ArenaLandTraitsMappings.end(), "Invalid climate type \"" +
			std::to_string(static_cast<int>(climateType)) + "\".");

		return iter->second;
	}

	Buffer<Color> makeInteriorSkyColors(bool outdoorDungeon, TextureManager &textureManager)
	{
		// Interior sky color comes from the darkest row of an .LGT light palette.
		const char *lightPaletteName = outdoorDungeon ? "FOG.LGT" : "NORMAL.LGT";

		TextureManager::IdGroup<ImageID> imageIDs;
		if (!textureManager.tryGetImageIDs(lightPaletteName, &imageIDs))
		{
			DebugLogWarning("Couldn't get .LGT image for \"" + std::string(lightPaletteName) + "\".");
			return Buffer<Color>();
		}

		// Get darkest light palette and a suitable color for 'dark'.
		const Image &lightPalette = textureManager.getImageHandle(imageIDs.getID(imageIDs.getCount() - 1));
		const uint8_t lightColor = lightPalette.getPixel(16, 0);

		const std::string &paletteName = PaletteFile::fromName(PaletteName::Default);
		PaletteID paletteID;
		if (!textureManager.tryGetPaletteID(paletteName.c_str(), &paletteID))
		{
			DebugLogWarning("Couldn't get palette ID for \"" + paletteName + "\".");
			return Buffer<Color>();
		}

		const Palette &palette = textureManager.getPaletteHandle(paletteID);
		DebugAssertIndex(palette, lightColor);
		const Color &paletteColor = palette[lightColor];

		Buffer<Color> skyColors(1);
		skyColors.set(0, paletteColor);
		return skyColors;
	}

	Buffer<Color> makeExteriorSkyColors(WeatherType weatherType, TextureManager &textureManager)
	{
		// Get the palette name for the given weather.
		const std::string &paletteName = PaletteFile::fromName(
			(weatherType == WeatherType::Clear) ? PaletteName::Daytime : PaletteName::Dreary);

		// The palettes in the data files only cover half of the day, so some added darkness is
		// needed for the other half.
		PaletteID paletteID;
		if (!textureManager.tryGetPaletteID(paletteName.c_str(), &paletteID))
		{
			DebugLogWarning("Couldn't get palette ID for \"" + paletteName + "\".");
			return Buffer<Color>();
		}

		const Palette &palette = textureManager.getPaletteHandle(paletteID);

		// Fill sky palette with darkness. The first color in the palette is the closest to night.
		const Color &darkness = palette[0];
		Buffer<Color> fullPalette(static_cast<int>(palette.size()) * 2);
		fullPalette.fill(darkness);

		// Copy the sky palette over the center of the full palette.
		std::copy(palette.begin(), palette.end(), fullPalette.get() + (fullPalette.getCount() / 4));

		return fullPalette;
	}
}

void SkyGeneration::InteriorSkyGenInfo::init(bool outdoorDungeon)
{
	this->outdoorDungeon = outdoorDungeon;
}

void SkyGeneration::ExteriorSkyGenInfo::init(ClimateType climateType, WeatherType weatherType,
	int currentDay, int starCount, uint32_t citySeed, uint32_t skySeed, bool provinceHasAnimatedLand)
{
	this->weatherType = weatherType;
	this->climateType = climateType;
	this->currentDay = currentDay;
	this->starCount = starCount;
	this->citySeed = citySeed;
	this->skySeed = skySeed;
	this->provinceHasAnimatedLand = provinceHasAnimatedLand;
}

void SkyGeneration::generateInteriorSky(const InteriorSkyGenInfo &skyGenInfo, TextureManager &textureManager,
	SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef)
{
	// Only worry about sky color for interior skies.
	Buffer<Color> skyColors = SkyGeneration::makeInteriorSkyColors(skyGenInfo.outdoorDungeon, textureManager);
	outSkyDef->init(std::move(skyColors));
}

void SkyGeneration::generateExteriorSky(const ExteriorSkyGenInfo &skyGenInfo,
	const BinaryAssetLibrary &binaryAssetLibrary, TextureManager &textureManager,
	SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef)
{
	DebugNotImplemented();
	/*// Add mountains and clouds first. Get the land traits associated with the given climate type.
	const LandTraits &landTraits = GetLandTraits(skyGenInfo.climateType);

	const auto &exeData = binaryAssetLibrary.getExeData();
	const auto &landFilenames = exeData.locations.distantMountainFilenames;
	DebugAssertIndex(landFilenames, landTraits.filenameIndex);
	const std::string &baseFilename = landFilenames[landTraits.filenameIndex];

	ArenaRandom random(skyGenInfo.skySeed);
	const int count = (random.next() % 4) + 2;

	// Converts an Arena angle to an actual angle in radians.
	auto arenaAngleToRadians = [](int arenaAngle)
	{
		// Arena angles: 0 = south, 128 = west, 256 = north, 384 = east.
		// Change from clockwise to counter-clockwise and move 0 to east.
		const Radians arenaRadians = Constants::TwoPi *
			(static_cast<double>(arenaAngle) / static_cast<double>(SkyGeneration::UNIQUE_ANGLES));
		const Radians flippedArenaRadians = Constants::TwoPi - arenaRadians;
		return flippedArenaRadians - Constants::HalfPi;
	};

	// Lambda for creating images with certain sky parameters.
	auto placeStaticObjects = [&random, &arenaAngleToRadians](int count, const std::string &baseFilename,
		int pos, int var, int maxDigits, bool randomHeight)
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
			std::string filename = [&baseFilename, pos, maxDigits, &digits]()
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





			// @todo: decide if std::optional is necessary; also see if ImageIDs should be obtained here (probably;
			// make it like map definition creation you know?).
			// @todo: continue SkyObjectDefinition init methods.
			DebugNotImplemented();



			std::optional<int> entryIndex = this->getTextureEntryIndex(filename);
			auto fixEntryIndexIfMissing = [this, &textureManager, &filename, &entryIndex]()
			{
				if (!entryIndex.has_value())
				{
					ImageID imageID;
					if (!textureManager.tryGetImageID(filename.c_str(), &imageID))
					{
						DebugCrash("Couldn't get image ID for \"" + filename + "\".");
					}

					TextureEntry textureEntry(std::move(filename), imageID);
					this->textures.push_back(std::move(textureEntry));
					entryIndex = static_cast<int>(this->textures.size()) - 1;
				}
			};

			// The yPos parameter is optional, and is assigned depending on whether the object
			// is in the air.
			constexpr int yPosLimit = 64;
			const int yPos = randomHeight ? (random.next() % yPosLimit) : 0;

			// Convert from Arena units to radians.
			const int arenaAngle = random.next() % SkyGeneration::UNIQUE_ANGLES;
			const Radians angle = arenaAngleToRadians(arenaAngle);

			// The object is either land or a cloud, currently determined by 'randomHeight' as
			// a shortcut. Land objects have no height. I'm doing it this way because LandObject
			// and AirObject are two different types.
			const bool isLand = !randomHeight;

			SkyObjectDefinition skyObjectDef;
			if (isLand)
			{
				fixEntryIndexIfMissing();
				this->landObjects.push_back(LandObject(*entryIndex, angle));
			}
			else
			{
				fixEntryIndexIfMissing();
				const double height = static_cast<double>(yPos) / static_cast<double>(yPosLimit);
				this->airObjects.push_back(AirObject(*entryIndex, angle, height));
			}
		}
	};

	// Initial set of statics based on the climate.
	placeStaticObjects(count, baseFilename, landTraits.position, landTraits.variation,
		landTraits.maxDigits, false);

	// Add clouds if the weather conditions are permitting.
	const bool hasClouds = skyGenInfo.weatherType == WeatherType::Clear;
	if (hasClouds)
	{
		const uint32_t cloudSeed = random.getSeed() + (skyGenInfo.currentDay % 32);
		random.srand(cloudSeed);

		constexpr int cloudCount = 7;
		const std::string &cloudFilename = exeData.locations.cloudFilename;
		constexpr int cloudPos = 5;
		constexpr int cloudVar = 17;
		constexpr int cloudMaxDigits = 2;
		placeStaticObjects(cloudCount, cloudFilename, cloudPos, cloudVar, cloudMaxDigits, true);
	}

	// Initialize animated lands (if any).
	if (skyGenInfo.provinceHasAnimatedLand)
	{
		// Position of animated land on province map; determines where it is on the horizon
		// for each location.
		const Int2 animLandGlobalPos(132, 52);
		const Int2 locationGlobalPos = LocationUtils::getLocalCityPoint(skyGenInfo.citySeed);

		// Distance on province map from current location to the animated land.
		const int dist = LocationUtils::getMapDistance(locationGlobalPos, animLandGlobalPos);

		// Position of the animated land on the horizon.
		const Radians angle = std::atan2(
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
		DebugAssertIndex(animFilenames, animIndex);
		std::string animFilename = String::toUppercase(animFilenames[animIndex]);

		// See if there's an existing texture set entry. If not, make one.
		std::optional<int> setEntryIndex = this->getTextureSetEntryIndex(animFilename);

		if (!setEntryIndex.has_value())
		{
			// Determine which frames the animation will have. .DFAs have multiple frames while
			// .IMGs do not, although we can use the same texture manager function for both.
			TextureManager::IdGroup<ImageID> imageIDs;
			if (!textureManager.tryGetImageIDs(animFilename.c_str(), &imageIDs))
			{
				DebugCrash("Couldn't get image IDs for \"" + animFilename + "\".");
			}

			TextureSetEntry textureSetEntry(std::move(animFilename), std::move(imageIDs));
			this->textureSets.push_back(std::move(textureSetEntry));
			setEntryIndex = static_cast<int>(this->textureSets.size()) - 1;
		}

		AnimatedLandObject animLandObj(*setEntryIndex, angle);
		this->animLandObjects.push_back(std::move(animLandObj));
	}

	// Add space objects if the weather conditions are permitting.
	const bool hasSpaceObjects = weatherType == WeatherType::Clear;

	if (hasSpaceObjects)
	{
		// Initialize moons.
		auto makeMoon = [this, currentDay, &textureManager, &exeData](MoonObject::Type type)
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
			const auto &moonFilenames = exeData.locations.moonFilenames;
			DebugAssertIndex(moonFilenames, moonIndex);
			std::string filename = String::toUppercase(moonFilenames[moonIndex]);

			// See if there's an existing texture entry. If not, make one for the moon phase.
			std::optional<int> entryIndex = this->getTextureEntryIndex(filename);
			if (!entryIndex.has_value())
			{
				TextureManager::IdGroup<ImageID> imageIDs;
				if (!textureManager.tryGetImageIDs(filename.c_str(), &imageIDs))
				{
					DebugCrash("Couldn't get image IDs for \"" + filename + "\".");
				}

				const ImageID imageID = imageIDs.getID(phaseIndex);
				TextureEntry textureEntry(std::move(filename), imageID);
				this->textures.push_back(std::move(textureEntry));
				entryIndex = static_cast<int>(this->textures.size()) - 1;
			}

			const double phasePercent = static_cast<double>(phaseIndex) /
				static_cast<double>(phaseCount);

			return MoonObject(*entryIndex, phasePercent, type);
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
						DebugAssertIndex(palette, subStar.color);
						const Color &paletteColor = palette[subStar.color];
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
						const Radians dxRadians = dxPercent * DistantSky::IDENTITY_ANGLE;
						const Radians dyRadians = dyPercent * DistantSky::IDENTITY_ANGLE;

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
				std::string starFilename = [&exeData, &star]()
				{
					const std::string typeStr = std::to_string(star.type + 1);
					std::string filename = exeData.locations.starFilename;
					const size_t index = filename.find('1');
					DebugAssert(index != std::string::npos);

					filename.replace(index, 1, typeStr);
					return String::toUppercase(filename);
				}();

				// See if there's an existing texture entry. If not, make one.
				std::optional<int> entryIndex = this->getTextureEntryIndex(starFilename);
				if (!entryIndex.has_value())
				{
					ImageID imageID;
					if (!textureManager.tryGetImageID(starFilename.c_str(), &imageID))
					{
						DebugCrash("Couldn't get image ID for \"" + starFilename + "\".");
					}

					TextureEntry textureEntry(std::move(starFilename), imageID);
					this->textures.push_back(std::move(textureEntry));
					entryIndex = static_cast<int>(this->textures.size()) - 1;
				}

				this->starObjects.push_back(StarObject::makeLarge(*entryIndex, direction));
			}
		}

		// Initialize sun texture index.
		std::string sunFilename = String::toUppercase(exeData.locations.sunFilename);
		std::optional<int> sunTextureIndex = this->getTextureEntryIndex(sunFilename);
		if (!sunTextureIndex.has_value())
		{
			ImageID imageID;
			if (!textureManager.tryGetImageID(sunFilename.c_str(), &imageID))
			{
				DebugCrash("Couldn't get image ID for \"" + sunFilename + "\".");
			}

			TextureEntry textureEntry(std::move(sunFilename), imageID);
			this->textures.push_back(std::move(textureEntry));
			sunTextureIndex = static_cast<int>(this->textures.size()) - 1;
		}

		this->sunEntryIndex = *sunTextureIndex;
	}*/
}
