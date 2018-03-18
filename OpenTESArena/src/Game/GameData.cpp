#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "Game.h"
#include "GameData.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/ExeData.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Entities/Animation.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Doodad.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityManager.h"
#include "../Entities/GenderName.h"
#include "../Entities/NonPlayer.h"
#include "../Entities/Player.h"
#include "../Interface/TextBox.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/ClimateType.h"
#include "../World/LocationType.h"
#include "../World/VoxelGrid.h"
#include "../World/WeatherType.h"
#include "../World/WorldType.h"

// Arbitrary value for testing. One real second = six game minutes.
// The value used in Arena is one real second = twenty game seconds.
const double GameData::TIME_SCALE = static_cast<double>(Clock::SECONDS_IN_A_DAY) / 240.0;

const double GameData::DEFAULT_INTERIOR_FOG_DIST = 25.0;

GameData::GameData(Player &&player)	
	: player(std::move(player)), triggerText(0.0, nullptr), actionText(0.0, nullptr),
	effectText(0.0, nullptr)
{
	// Most values need to be initialized elsewhere in the program in order to determine
	// the world state, etc..
	DebugMention("Initializing.");

	this->weathers.fill(WeatherType::Clear);
}

GameData::~GameData()
{
	DebugMention("Closing.");
}

std::vector<uint32_t> GameData::makeExteriorSkyPalette(WeatherType weatherType,
	TextureManager &textureManager)
{
	// Get the palette name for the given weather.
	const std::string paletteName = (weatherType == WeatherType::Clear) ?
		"DAYTIME.COL" : "DREARY.COL";

	// The palettes in the data files only cover half of the day, so some added
	// darkness is needed for the other half.
	const SDL_Surface *palette = textureManager.getSurface(paletteName);
	const uint32_t *pixels = static_cast<const uint32_t*>(palette->pixels);
	const int pixelCount = palette->w * palette->h;

	std::vector<uint32_t> fullPalette(pixelCount * 2);

	// Fill with darkness (the first color in the palette is the closest to night).
	const uint32_t darkness = pixels[0];
	std::fill(fullPalette.begin(), fullPalette.end(), darkness);

	// Copy the sky palette over the center of the full palette.
	std::copy(pixels, pixels + pixelCount, 
		fullPalette.data() + (fullPalette.size() / 4));

	return fullPalette;
}

double GameData::getFogDistanceFromWeather(WeatherType weatherType)
{
	// Arbitrary values, the distance at which fog is maximum.
	if (weatherType == WeatherType::Clear)
	{
		return 75.0;
	}
	else if (weatherType == WeatherType::Overcast)
	{
		return 25.0;
	}
	else if (weatherType == WeatherType::Rain)
	{
		return 35.0;
	}
	else if (weatherType == WeatherType::Snow)
	{
		return 15.0;
	}
	else
	{
		throw std::runtime_error("Bad weather type \"" +
			std::to_string(static_cast<int>(weatherType)) + "\".");
	}
}

void GameData::loadInterior(const MIFFile &mif, const Location &location,
	TextureManager &textureManager, Renderer &renderer)
{
	// Call interior WorldData loader.
	this->worldData = WorldData::loadInterior(mif);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position.
	const Double2 &startPoint = this->worldData.getStartPoints().front();
	this->player.teleport(Double3(startPoint.x, 1.0 + Player::HEIGHT, startPoint.y));

	// Set location.
	this->location = location;

	// Set interior sky palette.
	const auto &level = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	const uint32_t skyColor = level.getInteriorSkyColor();
	renderer.setSkyPalette(&skyColor, 1);

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::loadNamedDungeon(int dungeonID, int provinceID, bool isArtifactDungeon,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	// Dungeon ID must be for a named dungeon, not main quest dungeon.
	DebugAssert(dungeonID >= 2, "Dungeon ID \"" + std::to_string(dungeonID) +
		"\" must not be for main quest dungeon.");

	// Generate dungeon seed.
	const auto &cityData = miscAssets.getCityDataFile();
	const uint32_t dungeonSeed = cityData.getDungeonSeed(dungeonID, provinceID);

	// Call dungeon WorldData loader with parameters specific to named dungeons.
	const int widthChunks = 2;
	const int depthChunks = 1;
	this->worldData = WorldData::loadDungeon(
		dungeonSeed, widthChunks, depthChunks, isArtifactDungeon);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position.
	const Double2 &startPoint = this->worldData.getStartPoints().front();
	this->player.teleport(Double3(startPoint.x - 1.0, 1.0 + Player::HEIGHT, startPoint.y));

	// Set location.
	this->location.locationType = LocationType::Dungeon;
	this->location.provinceID = provinceID;
	this->location.name = "Test Named Dungeon";

	// Set interior sky palette.
	const auto &level = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	const uint32_t skyColor = level.getInteriorSkyColor();
	renderer.setSkyPalette(&skyColor, 1);

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::loadWildernessDungeon(int provinceID, int wildBlockX, int wildBlockY,
	const CityDataFile &cityData, TextureManager &textureManager, Renderer &renderer)
{
	// Verify that the wilderness block coordinates are valid (0..63).
	DebugAssert((wildBlockX >= 0) && (wildBlockX < RMDFile::WIDTH),
		"Wild block X \"" + std::to_string(wildBlockX) + "\" out of range.");
	DebugAssert((wildBlockY >= 0) && (wildBlockY < RMDFile::DEPTH),
		"Wild block Y \"" + std::to_string(wildBlockY) + "\" out of range.");

	// Generate wilderness dungeon seed.
	const uint32_t wildDungeonSeed = cityData.getWildernessDungeonSeed(
		provinceID, wildBlockX, wildBlockY);

	// Call dungeon WorldData loader with parameters specific to wilderness dungeons.
	const int widthChunks = 2;
	const int depthChunks = 2;
	const bool isArtifactDungeon = false;
	this->worldData = WorldData::loadDungeon(
		wildDungeonSeed, widthChunks, depthChunks, isArtifactDungeon);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position.
	const Double2 &startPoint = this->worldData.getStartPoints().front();
	this->player.teleport(Double3(startPoint.x - 1.0, 1.0 + Player::HEIGHT, startPoint.y));

	// Set location.
	this->location.locationType = LocationType::Dungeon;
	this->location.provinceID = provinceID;
	this->location.name = "Test Wild Dungeon";

	// Set interior sky palette.
	const auto &level = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	const uint32_t skyColor = level.getInteriorSkyColor();
	renderer.setSkyPalette(&skyColor, 1);

	// Arbitrary interior weather and fog.
	const double fogDistance = GameData::DEFAULT_INTERIOR_FOG_DIST;
	this->weatherType = WeatherType::Clear;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::loadPremadeCity(const MIFFile &mif, ClimateType climateType,
	WeatherType weatherType, const MiscAssets &miscAssets, TextureManager &textureManager,
	Renderer &renderer)
{
	// Call premade WorldData loader.
	this->worldData = WorldData::loadPremadeCity(mif, climateType, weatherType);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position.
	const Double2 &startPoint = this->worldData.getStartPoints().front();
	this->player.teleport(Double3(startPoint.x, 1.0 + Player::HEIGHT, startPoint.y));

	// Set location.
	const int provinceID = 8;
	const std::string locationName = [&miscAssets, provinceID]()
	{
		const auto &cityData = miscAssets.getCityDataFile();
		const auto &provinceData = cityData.getProvinceData(provinceID);
		const auto &locationData = provinceData.cityStates.front();
		return std::string(locationData.name.data());
	}();

	const LocationType locationType = LocationType::CityState;
	this->location = Location(locationName, provinceID, locationType, climateType);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));
	
	// Set weather and fog.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::loadCity(int localCityID, int provinceID, WeatherType weatherType,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	const int globalCityID = CityDataFile::getGlobalCityID(localCityID, provinceID);

	// Check that the IDs are in the proper range. Although 256 is a valid city ID,
	// loadPremadeCity() should be called instead for that case.
	DebugAssert(provinceID != 8, "Use loadPremadeCity() instead for center province.");
	DebugAssert((globalCityID >= 0) && (globalCityID < 256),
		"Invalid city ID \"" + std::to_string(globalCityID) + "\".");
	
	// Determine city traits from the given city ID.
	const LocationType locationType = WorldData::getCityLocationType(globalCityID);
	const ExeData::CityGeneration &cityGen = miscAssets.getExeData().cityGen;
	const bool isCity = locationType == LocationType::CityState;
	const bool isCoastal = std::find(cityGen.coastalCityList.begin(),
		cityGen.coastalCityList.end(), globalCityID) != cityGen.coastalCityList.end();
	const int templateCount = isCoastal ? (isCity ? 3 : 2) : 5;
	const int templateID = globalCityID % templateCount;

	const MIFFile mif = [locationType, &cityGen, isCoastal, templateID]()
	{
		// Get the index into the template names array (town%d.mif, ..., cityw%d.mif).
		const int templateNameIndex = [locationType, isCoastal]()
		{
			if (locationType == LocationType::CityState)
			{
				return isCoastal ? 5 : 4;
			}
			else if (locationType == LocationType::Town)
			{
				return isCoastal ? 1 : 0;
			}
			else if (locationType == LocationType::Village)
			{
				return isCoastal ? 3 : 2;
			}
			else
			{
				throw std::runtime_error("Bad location type \"" +
					std::to_string(static_cast<int>(locationType)) + "\".");
			}
		}();

		// Get the template name associated with the city ID.
		std::string templateName = cityGen.templateFilenames.at(templateNameIndex);
		templateName = String::replace(templateName, "%d", std::to_string(templateID + 1));
		templateName = String::toUppercase(templateName);

		return MIFFile(templateName);
	}();

	// Location-related data from the city data file.
	const auto &provinceData = miscAssets.getCityDataFile().getProvinceData(provinceID);
	const auto &locationData = [localCityID, locationType, &provinceData]()
	{
		if (locationType == LocationType::CityState)
		{
			return provinceData.cityStates.at(localCityID);
		}
		else if (locationType == LocationType::Town)
		{
			return provinceData.towns.at(localCityID - 8);
		}
		else if (locationType == LocationType::Village)
		{
			return provinceData.villages.at(localCityID - 16);
		}
		else
		{
			throw std::runtime_error("Bad location type \"" +
				std::to_string(static_cast<int>(locationType)) + "\".");
		}
	}();

	const int cityX = locationData.x;
	const int cityY = locationData.y;

	// City block count (6x6, 5x5, 4x4).
	const int cityDim = [locationType]()
	{
		if (locationType == LocationType::CityState)
		{
			return 6;
		}
		else if (locationType == LocationType::Town)
		{
			return 5;
		}
		else if (locationType == LocationType::Village)
		{
			return 4;
		}
		else
		{
			throw std::runtime_error("Bad location type \"" +
				std::to_string(static_cast<int>(locationType)) + "\".");
		}
	}();

	const std::vector<uint8_t> &reservedBlocks = [&cityGen, isCoastal, templateID]()
	{
		// Get the index into the reserved block list.
		const int index = isCoastal ? (5 + templateID) : templateID;
		return cityGen.reservedBlockLists.at(index);
	}();

	const Int2 &startPosition = [locationType, &cityGen, isCoastal, templateID]()
	{
		// Get the index into the starting positions array.
		const int index = [locationType, isCoastal, templateID]()
		{
			if (locationType == LocationType::CityState)
			{
				return isCoastal ? (19 + templateID) : (14 + templateID);
			}
			else if (locationType == LocationType::Town)
			{
				return isCoastal ? (5 + templateID) : templateID;
			}
			else if (locationType == LocationType::Village)
			{
				return isCoastal ? (12 + templateID) : (7 + templateID);
			}
			else
			{
				throw std::runtime_error("Bad location type \"" +
					std::to_string(static_cast<int>(locationType)) + "\".");
			}
		}();

		const std::pair<int, int> &pair = cityGen.startingPositions.at(index);
		return Int2(pair.first, pair.second);
	}();

	// Call city WorldData loader.
	this->worldData = WorldData::loadCity(globalCityID, mif, cityX, cityY, cityDim,
		reservedBlocks, startPosition, locationType, weatherType);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position.
	const Double2 &startPoint = worldData.getStartPoints().front();
	this->player.teleport(Double3(startPoint.x, 1.0 + Player::HEIGHT, startPoint.y));

	// Set location.
	// - To do: get original climate data (from WorldData?).
	this->location = Location(std::string(locationData.name.data()), provinceID,
		locationType, ClimateType::Temperate);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	// Set weather and fog.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

void GameData::loadWilderness(int localCityID, int provinceID, int rmdTR, int rmdTL, int rmdBR,
	int rmdBL, ClimateType climateType, WeatherType weatherType, const MiscAssets &miscAssets,
	TextureManager &textureManager, Renderer &renderer)
{
	// Call wilderness WorldData loader.
	this->worldData = WorldData::loadWilderness(
		rmdTR, rmdTL, rmdBR, rmdBL, climateType, weatherType);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set arbitrary player starting position (no starting point in WILD.MIF).
	const Double2 startPoint(63.50, 63.50);
	this->player.teleport(Double3(startPoint.x, 1.0 + Player::HEIGHT, startPoint.y));

	// Set location.
	const auto &cityData = miscAssets.getCityDataFile();
	const LocationType locationType = WorldData::getCityLocationType(localCityID);
	const auto &locationData = [localCityID, provinceID, &cityData, locationType]()
	{
		const auto &provinceData = cityData.getProvinceData(provinceID);
		if (locationType == LocationType::CityState)
		{
			return provinceData.cityStates.at(localCityID);
		}
		else if (locationType == LocationType::Town)
		{
			return provinceData.towns.at(localCityID - 8);
		}
		else if (locationType == LocationType::Village)
		{
			return provinceData.villages.at(localCityID - 16);
		}
		else
		{
			throw std::runtime_error("Bad location type \"" +
				std::to_string(static_cast<int>(locationType)) + "\".");
		}
	}();

	this->location = Location(std::string(locationData.name.data()), provinceID,
		locationType, climateType);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	// Set weather and fog.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
}

std::pair<double, std::unique_ptr<TextBox>> &GameData::getTriggerText()
{
	return this->triggerText;
}

std::pair<double, std::unique_ptr<TextBox>> &GameData::getActionText()
{
	return this->actionText;
}

std::pair<double, std::unique_ptr<TextBox>> &GameData::getEffectText()
{
	return this->effectText;
}

Player &GameData::getPlayer()
{
	return this->player;
}

WorldData &GameData::getWorldData()
{
	return this->worldData;
}

Location &GameData::getLocation()
{
	return this->location;
}

Date &GameData::getDate()
{
	return this->date;
}

Clock &GameData::getClock()
{
	return this->clock;
}

double GameData::getDaytimePercent() const
{
	return this->clock.getPreciseTotalSeconds() /
		static_cast<double>(Clock::SECONDS_IN_A_DAY);
}

double GameData::getFogDistance() const
{
	return this->fogDistance;
}

double GameData::getAmbientPercent() const
{
	if (this->worldData.getWorldType() == WorldType::Interior)
	{
		// Completely dark indoors (some places might be an exception to this, and those
		// would be handled eventually).
		return 0.0;
	}
	else
	{
		// The ambient light outside depends on the clock time.
		const double clockPreciseSeconds = this->clock.getPreciseTotalSeconds();

		// Time ranges where the ambient light changes. The start times are inclusive,
		// and the end times are exclusive.
		const double startBrighteningTime =
			Clock::AmbientStartBrightening.getPreciseTotalSeconds();
		const double endBrighteningTime =
			Clock::AmbientEndBrightening.getPreciseTotalSeconds();
		const double startDimmingTime =
			Clock::AmbientStartDimming.getPreciseTotalSeconds();
		const double endDimmingTime =
			Clock::AmbientEndDimming.getPreciseTotalSeconds();

		// In Arena, the min ambient is 0 and the max ambient is 1, but we're using
		// some values here that make testing easier.
		const double minAmbient = 0.30;
		const double maxAmbient = 1.0;

		if ((clockPreciseSeconds >= endBrighteningTime) &&
			(clockPreciseSeconds < startDimmingTime))
		{
			// Daytime ambient.
			return maxAmbient;
		}
		else if ((clockPreciseSeconds >= startBrighteningTime) &&
			(clockPreciseSeconds < endBrighteningTime))
		{
			// Interpolate brightening light (in the morning).
			const double timePercent = (clockPreciseSeconds - startBrighteningTime) /
				(endBrighteningTime - startBrighteningTime);
			return minAmbient + ((maxAmbient - minAmbient) * timePercent);
		}
		else if ((clockPreciseSeconds >= startDimmingTime) &&
			(clockPreciseSeconds < endDimmingTime))
		{
			// Interpolate dimming light (in the evening).
			const double timePercent = (clockPreciseSeconds - startDimmingTime) /
				(endDimmingTime - startDimmingTime);
			return maxAmbient + ((minAmbient - maxAmbient) * timePercent);
		}
		else
		{
			// Night ambient.
			return minAmbient;
		}
	}
}

double GameData::getBetterAmbientPercent() const
{
	const double daytimePercent = this->getDaytimePercent();
	const double minAmbient = 0.20;
	const double maxAmbient = 0.90;
	const double diff = maxAmbient - minAmbient;
	const double center = minAmbient + (diff / 2.0);
	return center + ((diff / 2.0) * -std::cos(daytimePercent * (2.0 * Constants::Pi)));
}

std::function<void(Game&)> &GameData::getOnLevelUpVoxelEnter()
{
	return this->onLevelUpVoxelEnter;
}

void GameData::updateWeather(ArenaRandom &random, const ExeData &exeData)
{
	const int seasonIndex = this->date.getSeason();

	for (size_t i = 0; i < this->weathers.size(); i++)
	{
		const int climateIndex = exeData.locations.climates.at(i);
		const int variantIndex = [&random]()
		{
			// 40% for 2, 20% for 1, 20% for 3, 10% for 0, and 10% for 4.
			const int val = random.next() % 100;

			if (val >= 60)
			{
				return 2;
			}
			else if (val >= 40)
			{
				return 1;
			}
			else if (val >= 20)
			{
				return 3;
			}
			else if (val >= 10)
			{
				return 0;
			}
			else
			{
				return 4;
			}
		}();

		const int weatherTableIndex = (climateIndex * 20) + (seasonIndex * 5) + variantIndex;
		this->weathers.at(i) = static_cast<WeatherType>(
			exeData.locations.weatherTable.at(weatherTableIndex));
	}
}

void GameData::tickTime(double dt, Game &game)
{
	assert(dt >= 0.0);

	// Tick the game clock.
	const int oldHour = this->clock.getHours24();
	this->clock.tick(dt * GameData::TIME_SCALE);
	const int newHour = this->clock.getHours24();

	// Check if the hour changed.
	if (newHour != oldHour)
	{
		// Update the weather.
		const auto &exeData = game.getMiscAssets().getExeData();

		// To do: see if this should be stored somewhere else (like in GameData).
		ArenaRandom random;
		this->updateWeather(random, exeData);
	}

	// Check if the clock hour looped back around.
	if (newHour < oldHour)
	{
		// Increment the day.
		this->date.incrementDay();
	}
}
