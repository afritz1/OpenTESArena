#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <type_traits>

#include "ArenaSkyUtils.h"
#include "ClimateType.h"
#include "LocationUtils.h"
#include "SkyDefinition.h"
#include "SkyGeneration.h"
#include "SkyInfoDefinition.h"
#include "WeatherType.h"
#include "WeatherUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Random.h"
#include "../Math/Vector4.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace SkyGeneration
{
	// Mapping caches of Arena sky objects to modern sky info entries. Don't need caches for sun
	// and moons since they're not spawned in bulk.
	using ArenaLandMappingCache = std::unordered_map<std::string, SkyDefinition::LandDefID>;
	using ArenaAirMappingCache = std::unordered_map<std::string, SkyDefinition::AirDefID>;
	using ArenaSmallStarMappingCache = std::unordered_map<uint8_t, SkyDefinition::StarDefID>;
	using ArenaLargeStarMappingCache = std::unordered_map<std::string, SkyDefinition::StarDefID>;

	Buffer<Color> makeInteriorSkyColors(bool outdoorDungeon, TextureManager &textureManager)
	{
		// Interior sky color comes from the darkest row of an .LGT light palette.
		const char *lightPaletteName = outdoorDungeon ? "FOG.LGT" : "NORMAL.LGT";

		const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
			textureManager.tryGetTextureBuilderIDs(lightPaletteName);
		if (!textureBuilderIDs.has_value())
		{
			DebugLogWarning("Couldn't get texture builder IDs for \"" + std::string(lightPaletteName) + "\".");
			return Buffer<Color>();
		}

		// Get darkest light palette and a suitable color for 'dark'.
		const TextureBuilderID darkestTextureBuilderID = textureBuilderIDs->getID(textureBuilderIDs->getCount() - 1);
		const TextureBuilder &lightPaletteTextureBuilder = textureManager.getTextureBuilderHandle(darkestTextureBuilderID);
		DebugAssert(lightPaletteTextureBuilder.getType() == TextureBuilder::Type::Paletted);
		const TextureBuilder::PalettedTexture &lightPaletteTexture = lightPaletteTextureBuilder.getPaletted();
		const Buffer2D<uint8_t> &lightPaletteTexels = lightPaletteTexture.texels;
		const uint8_t lightColor = lightPaletteTexels.get(16, 0);

		const std::string &paletteName = ArenaPaletteName::Default;
		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
		if (!paletteID.has_value())
		{
			DebugLogWarning("Couldn't get palette ID for \"" + paletteName + "\".");
			return Buffer<Color>();
		}

		const Palette &palette = textureManager.getPaletteHandle(*paletteID);
		DebugAssertIndex(palette, lightColor);
		const Color &paletteColor = palette[lightColor];

		Buffer<Color> skyColors(1);
		skyColors.set(0, paletteColor);
		return skyColors;
	}

	Buffer<Color> makeExteriorSkyColors(WeatherType weatherType, TextureManager &textureManager)
	{
		// Get the palette name for the given weather.
		const std::string &paletteName = (weatherType == WeatherType::Clear) ?
			ArenaPaletteName::Daytime : ArenaPaletteName::Dreary;

		// The palettes in the data files only cover half of the day, so some added darkness is
		// needed for the other half.
		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
		if (!paletteID.has_value())
		{
			DebugLogWarning("Couldn't get palette ID for \"" + paletteName + "\".");
			return Buffer<Color>();
		}

		const Palette &palette = textureManager.getPaletteHandle(*paletteID);

		// Fill sky palette with darkness. The first color in the palette is the closest to night.
		const Color &darkness = palette[0];
		Buffer<Color> fullPalette(static_cast<int>(palette.size()) * 2);
		fullPalette.fill(darkness);

		// Copy the sky palette over the center of the full palette.
		std::copy(palette.begin(), palette.end(), fullPalette.get() + (fullPalette.getCount() / 4));

		return fullPalette;
	}

	// Used with mountains and clouds.
	template <bool IsLandObject>
	bool tryGenerateArenaStaticObject(const std::string &baseFilename, int position, int variation,
		int maxDigits, ArenaRandom &random, TextureManager &textureManager, SkyDefinition *outSkyDef,
		SkyInfoDefinition *outSkyInfoDef, ArenaLandMappingCache *landCache, ArenaAirMappingCache *airCache)
	{
		// Digits for the filename variant. Allowed up to two digits.
		const std::string digits = [&random, variation]()
		{
			const int randVal = random.next() % variation;
			return std::to_string((randVal == 0) ? variation : randVal);
		}();

		DebugAssert(digits.size() <= maxDigits);

		const std::string imageFilename = [&baseFilename, position, maxDigits, &digits]()
		{
			std::string name = baseFilename;
			const int digitCount = static_cast<int>(digits.size());

			// Push the starting position right depending on the max digits.
			const int offset = maxDigits - digitCount;

			for (int digitIndex = 0; digitIndex < digitCount; digitIndex++)
			{
				const int nameIndex = position + offset + digitIndex;
				DebugAssertIndex(name, nameIndex);
				name[nameIndex] = digits[digitIndex];
			}

			return String::toUppercase(name);
		}();

		// Convert from Arena units to radians.
		const int arenaAngle = random.next() % ArenaSkyUtils::UNIQUE_ANGLES;
		const Radians angleX = ArenaSkyUtils::arenaAngleToRadians(arenaAngle);

		// The object is either a mountain or cloud.
		TextureAssetReference textureAssetRef = TextureAssetReference(std::string(imageFilename)); // Most vexing parse.
		if constexpr (IsLandObject)
		{
			SkyDefinition::LandDefID landDefID;
			const auto iter = landCache->find(imageFilename);
			if (iter != landCache->end())
			{
				landDefID = iter->second;
			}
			else
			{
				SkyLandDefinition skyLandDef;
				skyLandDef.init(std::move(textureAssetRef), SkyLandDefinition::ShadingType::Ambient);
				landDefID = outSkyInfoDef->addLand(std::move(skyLandDef));
				landCache->emplace(imageFilename, landDefID);
			}

			outSkyDef->addLand(landDefID, angleX);
		}
		else
		{
			const Radians angleY = [&random]()
			{
				constexpr int yPosLimit = 64;
				const int yPos = random.next() % yPosLimit;
				const double heightPercent = static_cast<double>(yPos) / static_cast<double>(yPosLimit);

				// @todo: convert to Y angle properly.
				const Radians angleLimit = 60.0 * Constants::DegToRad;
				return heightPercent * angleLimit;
			}();

			SkyDefinition::AirDefID airDefID;
			const auto iter = airCache->find(imageFilename);
			if (iter != airCache->end())
			{
				airDefID = iter->second;
			}
			else
			{
				SkyAirDefinition skyAirDef;
				skyAirDef.init(std::move(textureAssetRef));
				airDefID = outSkyInfoDef->addAir(std::move(skyAirDef));
				airCache->emplace(imageFilename, airDefID);
			}

			outSkyDef->addAir(airDefID, angleX, angleY);
		}

		return true;
	}

	// Includes distant mountains and clouds.
	void generateArenaStatics(ClimateType climateType, WeatherType weatherType, int currentDay,
		uint32_t skySeed, const ExeData &exeData, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef)
	{
		ArenaRandom random(skySeed);

		// Mountain generation.
		const ArenaSkyUtils::LandTraits &landTraits = ArenaSkyUtils::getLandTraits(climateType);
		const auto &landFilenames = exeData.locations.distantMountainFilenames;
		DebugAssertIndex(landFilenames, landTraits.filenameIndex);
		const std::string &landFilename = landFilenames[landTraits.filenameIndex];

		ArenaLandMappingCache landCache;
		const int landStaticsCount = (random.next() % 4) + 2;
		for (int i = 0; i < landStaticsCount; i++)
		{
			const int position = landTraits.position;
			const int variation = landTraits.variation;
			const int maxDigits = landTraits.maxDigits;
			if (!SkyGeneration::tryGenerateArenaStaticObject<true>(landFilename, position, variation,
				maxDigits, random, textureManager, outSkyDef, outSkyInfoDef, &landCache, nullptr))
			{
				DebugLogWarning("Couldn't generate sky static land \"" + landFilename + "\" (position: " +
					std::to_string(position) + ", variation: " + std::to_string(variation) + ", max digits: " +
					std::to_string(maxDigits) + ").");
			}
		}

		// Cloud generation, only if the sky is clear.
		if (WeatherUtils::isClear(weatherType))
		{
			const uint32_t cloudSeed = random.getSeed() + (currentDay % 32);
			random.srand(cloudSeed);

			constexpr int cloudCount = 7;
			const std::string &cloudFilename = exeData.locations.cloudFilename;

			ArenaAirMappingCache airCache;
			for (int i = 0; i < cloudCount; i++)
			{
				constexpr int cloudPosition = 5;
				constexpr int cloudVariation = 17;
				constexpr int cloudMaxDigits = 2;
				constexpr bool randomHeight = true;
				if (!SkyGeneration::tryGenerateArenaStaticObject<false>(cloudFilename, cloudPosition,
					cloudVariation, cloudMaxDigits, random, textureManager, outSkyDef, outSkyInfoDef,
					nullptr, &airCache))
				{
					DebugLogWarning("Couldn't generate sky static air \"" + cloudFilename + "\" (position: " +
						std::to_string(cloudPosition) + ", variation: " +
						std::to_string(cloudVariation) + ", max digits: " +
						std::to_string(cloudMaxDigits) + ").");
				}
			}
		}
	}

	// Assumes that animated land can only appear in the one hardcoded province.
	void generateArenaAnimatedLand(uint32_t citySeed, const ExeData &exeData,
		TextureManager &textureManager, SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef)
	{
		// Position of animated land on province map; determines where it is on the horizon
		// for each location.
		const Int2 animLandGlobalPos(132, 52);
		const Int2 locationGlobalPos = LocationUtils::getLocalCityPoint(citySeed);

		// Distance on province map from current location to the animated land.
		const int dist = LocationUtils::getMapDistance(locationGlobalPos, animLandGlobalPos);

		// Use a different animation based on world map distance.
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
		const std::string animFilename = String::toUppercase(animFilenames[animIndex]);

		// Determine which frames the animation will have. DFAs have multiple frames while
		// IMGs do not, although we can use the same texture manager function for both.
		std::vector<TextureAssetReference> textureAssetRefs =
			TextureUtils::makeTextureAssetRefs(animFilename, textureManager);

		// Position on the horizon.
		const Radians angleX = std::atan2(
			static_cast<double>(locationGlobalPos.y - animLandGlobalPos.y),
			static_cast<double>(animLandGlobalPos.x - locationGlobalPos.x));

		const double animSeconds = ArenaSkyUtils::ANIMATED_LAND_SECONDS_PER_FRAME *
			static_cast<double>(textureAssetRefs.size());

		SkyLandDefinition skyLandDef;
		skyLandDef.init(std::move(textureAssetRefs), animSeconds, SkyLandDefinition::ShadingType::Bright);
		const SkyDefinition::LandDefID landDefID = outSkyInfoDef->addLand(std::move(skyLandDef));
		outSkyDef->addLand(landDefID, angleX);
	}

	void generateArenaStars(int starCount, const ExeData &exeData, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef)
	{
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

		constexpr int8_t NO_STAR_TYPE = -1;

		std::vector<Star> stars;
		std::array<bool, 3> planets = { false, false, false };

		ArenaRandom random(0x12345679);
		auto getRndCoord = [&random]()
		{
			const int16_t d = (0x800 + random.next()) & 0x0FFF;
			return ((d & 2) == 0) ? d : -d;
		};

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

		ArenaSmallStarMappingCache smallStarCache;
		ArenaLargeStarMappingCache largeStarCache;

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
				// Group of small stars around the primary direction.
				for (const auto &subStar : star.subList)
				{
					const uint8_t paletteIndex = subStar.color;

					// Delta X and Y are applied after world-to-pixel projection of the base
					// direction in the original game, but we're doing angle calculations here
					// instead for the sake of keeping all the star generation code in one place.
					const Double3 subDirection = [&direction, &subStar]()
					{
						// Convert delta X and Y to percentages of the identity dimension (320px).
						const double dxPercent = static_cast<double>(subStar.dx) / ArenaSkyUtils::IDENTITY_DIM;
						const double dyPercent = static_cast<double>(subStar.dy) / ArenaSkyUtils::IDENTITY_DIM;

						// Convert percentages to radians. Positive X is counter-clockwise, positive
						// Y is up.
						const Radians dxRadians = dxPercent * ArenaSkyUtils::IDENTITY_ANGLE;
						const Radians dyRadians = dyPercent * ArenaSkyUtils::IDENTITY_ANGLE;

						// Apply rotations to base direction.
						const Matrix4d xRotation = Matrix4d::xRotation(dxRadians);
						const Matrix4d yRotation = Matrix4d::yRotation(dyRadians);
						const Double4 newDir = yRotation * (xRotation * Double4(direction, 0.0));

						return Double3(newDir.x, newDir.y, newDir.z);
					}();

					SkyDefinition::StarDefID starDefID;
					const auto iter = smallStarCache.find(paletteIndex);
					if (iter != smallStarCache.end())
					{
						starDefID = iter->second;
					}
					else
					{
						SkyStarDefinition skyStarDef;
						skyStarDef.initSmall(paletteIndex);
						starDefID = outSkyInfoDef->addStar(std::move(skyStarDef));
						smallStarCache.emplace(paletteIndex, starDefID);
					}

					outSkyDef->addStar(starDefID, subDirection);
				}
			}
			else
			{
				// Large star.
				const std::string starFilename = [&exeData, &star]()
				{
					const std::string typeStr = std::to_string(star.type + 1);
					std::string filename = exeData.locations.starFilename;
					const size_t index = filename.find('1');
					DebugAssert(index != std::string::npos);

					filename.replace(index, 1, typeStr);
					return String::toUppercase(filename);
				}();

				TextureAssetReference textureAssetRef = TextureAssetReference(std::string(starFilename)); // Most vexing parse.
				SkyDefinition::StarDefID starDefID;
				const auto iter = largeStarCache.find(starFilename);
				if (iter != largeStarCache.end())
				{
					starDefID = iter->second;
				}
				else
				{
					SkyStarDefinition skyStarDef;
					skyStarDef.initLarge(std::move(textureAssetRef));
					starDefID = outSkyInfoDef->addStar(std::move(skyStarDef));
					largeStarCache.emplace(starFilename, starDefID);
				}

				outSkyDef->addStar(starDefID, direction);
			}
		}
	}

	void generateArenaSun(const ExeData &exeData, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef)
	{
		std::string sunFilename = String::toUppercase(exeData.locations.sunFilename);
		TextureAssetReference textureAssetRef(std::move(sunFilename));

		SkySunDefinition skySunDef;
		skySunDef.init(std::move(textureAssetRef));
		const SkyDefinition::SunDefID sunDefID = outSkyInfoDef->addSun(std::move(skySunDef));
		outSkyDef->addSun(sunDefID, ArenaSkyUtils::SUN_BONUS_LATITUDE);
	}

	void generateArenaMoons(int currentDay, const ExeData &exeData, TextureManager &textureManager,
		SkyDefinition *outSkyDef, SkyInfoDefinition *outSkyInfoDef)
	{
		auto generateMoon = [currentDay, &exeData, &textureManager, outSkyDef,
			outSkyInfoDef](bool isFirstMoon)
		{
			constexpr int phaseCount = 32;
			const int phaseIndex = (currentDay + (isFirstMoon ? 0 : 14)) % phaseCount;

			const int moonFilenameIndex = isFirstMoon ? 0 : 1;
			const auto &moonFilenames = exeData.locations.moonFilenames;
			DebugAssertIndex(moonFilenames, moonFilenameIndex);
			std::string moonFilename = String::toUppercase(moonFilenames[moonFilenameIndex]);
			std::vector<TextureAssetReference> textureAssetRefs =
				TextureUtils::makeTextureAssetRefs(moonFilename, textureManager);

			// Base direction from original game values.
			// @todo: move to ArenaSkyUtils
			const Double3 baseDir = (isFirstMoon ?
				Double3(0.0, -57536.0, 0.0) : Double3(-3000.0, -53536.0, 0.0)).normalized();

			const double orbitPercent = static_cast<double>(phaseIndex) / static_cast<double>(phaseCount);
			const double bonusLatitude = isFirstMoon ?
				ArenaSkyUtils::MOON_1_BONUS_LATITUDE : ArenaSkyUtils::MOON_2_BONUS_LATITUDE;

			SkyMoonDefinition skyMoonDef;
			skyMoonDef.init(std::move(textureAssetRefs));
			const SkyDefinition::MoonDefID moonDefID = outSkyInfoDef->addMoon(std::move(skyMoonDef));
			outSkyDef->addMoon(moonDefID, baseDir, orbitPercent, bonusLatitude, phaseIndex);
		};

		generateMoon(true);
		generateMoon(false);
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
	const auto &exeData = binaryAssetLibrary.getExeData();

	// Generate static land and air objects.
	SkyGeneration::generateArenaStatics(skyGenInfo.climateType, skyGenInfo.weatherType,
		skyGenInfo.currentDay, skyGenInfo.skySeed, exeData, textureManager, outSkyDef, outSkyInfoDef);

	// Generate animated land if the province has it.
	if (skyGenInfo.provinceHasAnimatedLand)
	{
		SkyGeneration::generateArenaAnimatedLand(skyGenInfo.citySeed, exeData, textureManager,
			outSkyDef, outSkyInfoDef);
	}

	// Add space objects if the weather permits it.
	if (WeatherUtils::isClear(skyGenInfo.weatherType))
	{
		SkyGeneration::generateArenaMoons(skyGenInfo.currentDay, exeData, textureManager,
			outSkyDef, outSkyInfoDef);
		SkyGeneration::generateArenaStars(skyGenInfo.starCount, exeData, textureManager,
			outSkyDef, outSkyInfoDef);
		SkyGeneration::generateArenaSun(exeData, textureManager, outSkyDef, outSkyInfoDef);
	}
}
