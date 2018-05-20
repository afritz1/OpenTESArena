#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "Game.h"
#include "GameData.h"
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
#include "../Media/MusicFile.h"
#include "../Media/MusicName.h"
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

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<WeatherType>
	{
		size_t operator()(const WeatherType &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

namespace
{
	// Arbitrary fog distances for each weather; the distance at which fog is maximum.
	const std::unordered_map<WeatherType, double> WeatherFogDistances =
	{
		{ WeatherType::Clear, 100.0 },
		{ WeatherType::Overcast, 30.0 },
		{ WeatherType::Rain, 50.0 },
		{ WeatherType::Snow, 25.0 },
		{ WeatherType::SnowOvercast, 20.0 },
		{ WeatherType::Rain2, 50.0 },
		{ WeatherType::Overcast2, 30.0 },
		{ WeatherType::SnowOvercast2, 20.0 }
	};
}

GameData::TimedTextBox::TimedTextBox(double remainingDuration, std::unique_ptr<TextBox> textBox)
	: textBox(std::move(textBox))
{
	this->remainingDuration = remainingDuration;
}

GameData::TimedTextBox::TimedTextBox()
	: TimedTextBox(0.0, nullptr) { }

void GameData::TimedTextBox::reset()
{
	this->remainingDuration = 0.0;
	this->textBox = nullptr;
}

// Arbitrary value for testing. One real second = six game minutes.
// The value used in Arena is one real second = twenty game seconds.
const double GameData::TIME_SCALE = static_cast<double>(Clock::SECONDS_IN_A_DAY) / 240.0;

const double GameData::DEFAULT_INTERIOR_FOG_DIST = 25.0;

GameData::GameData(Player &&player, const MiscAssets &miscAssets)
	: player(std::move(player))
{
	// Most values need to be initialized elsewhere in the program in order to determine
	// the world state, etc..
	DebugMention("Initializing.");

	// Make a copy of the global constant city data. This is the "instance" city data
	// that can be assigned to.
	this->cityData = miscAssets.getCityDataFile();

	// Set default location visibilities.
	for (int i = 0; i < 8; i++)
	{
		auto &provinceData = this->cityData.getProvinceData(i);
		for (auto &cityState : provinceData.cityStates)
		{
			cityState.setVisible(true);
		}

		for (auto &town : provinceData.towns)
		{
			town.setVisible(true);
		}

		for (auto &village : provinceData.villages)
		{
			village.setVisible(true);
		}

		// Make main quest dungeons visible for testing.
		provinceData.firstDungeon.setVisible(true);
		provinceData.secondDungeon.setVisible(true);

		for (auto &dungeon : provinceData.randomDungeons)
		{
			dungeon.setVisible(false);
		}
	}

	auto &centerProvinceData = this->cityData.getProvinceData(8);
	centerProvinceData.cityStates.front().setVisible(true);

	// Do initial weather update (to set each value to a valid state).
	this->updateWeather(miscAssets.getExeData());
}

GameData::~GameData()
{
	DebugMention("Closing.");
}

std::string GameData::getDateString(const Date &date, const ExeData &exeData)
{
	std::string text = exeData.status.date;

	// Replace carriage returns with newlines.
	text = String::replace(text, '\r', '\n');

	// Replace first %s with weekday.
	const std::string &weekdayString =
		exeData.calendar.weekdayNames.at(date.getWeekday());
	size_t index = text.find("%s");
	text.replace(index, 2, weekdayString);

	// Replace %u%s with day and ordinal suffix.
	const std::string dayString = date.getOrdinalDay();
	index = text.find("%u%s");
	text.replace(index, 4, dayString);

	// Replace third %s with month.
	const std::string &monthString =
		exeData.calendar.monthNames.at(date.getMonth());
	index = text.find("%s");
	text.replace(index, 2, monthString);

	// Replace %d with year.
	index = text.find("%d");
	text.replace(index, 2, std::to_string(date.getYear()));

	return text;
}

std::vector<uint32_t> GameData::makeExteriorSkyPalette(WeatherType weatherType,
	TextureManager &textureManager)
{
	// Get the palette name for the given weather.
	const std::string paletteName = (weatherType == WeatherType::Clear) ?
		"DAYTIME.COL" : "DREARY.COL";

	// The palettes in the data files only cover half of the day, so some added
	// darkness is needed for the other half.
	const Surface &palette = textureManager.getSurface(paletteName);
	const uint32_t *pixels = static_cast<const uint32_t*>(palette.getPixels());
	const int pixelCount = palette.getWidth() * palette.getHeight();

	// Fill palette with darkness (the first color in the palette is the closest to night).
	const uint32_t darkness = pixels[0];
	std::vector<uint32_t> fullPalette(pixelCount * 2, darkness);

	// Copy the sky palette over the center of the full palette.
	std::copy(pixels, pixels + pixelCount, 
		fullPalette.data() + (fullPalette.size() / 4));

	return fullPalette;
}

double GameData::getFogDistanceFromWeather(WeatherType weatherType)
{
	return WeatherFogDistances.at(weatherType);
}

MusicName GameData::getExteriorMusicName(WeatherType weatherType)
{
	return MusicFile::fromWeather(weatherType);
}

MusicName GameData::getDungeonMusicName(Random &random)
{
	const std::array<MusicName, 5> DungeonMusics =
	{
		MusicName::Dungeon1,
		MusicName::Dungeon2,
		MusicName::Dungeon3,
		MusicName::Dungeon4,
		MusicName::Dungeon5
	};

	return DungeonMusics.at(random.next(static_cast<int>(DungeonMusics.size())));
}

MusicName GameData::getInteriorMusicName(const std::string &mifName, Random &random)
{
	// Check against all of the non-dungeon interiors first.
	const bool isEquipmentStore = mifName.find("EQUIP") != std::string::npos;
	const bool isHouse = (mifName.find("BS") != std::string::npos) ||
		(mifName.find("NOBLE") != std::string::npos);
	const bool isMagesGuild = mifName.find("MAGE") != std::string::npos;
	const bool isPalace = (mifName.find("PALACE") != std::string::npos) ||
		(mifName.find("TOWNPAL") != std::string::npos) ||
		(mifName.find("VILPAL") != std::string::npos);
	const bool isTavern = mifName.find("TAVERN") != std::string::npos;
	const bool isTemple = mifName.find("TEMPLE") != std::string::npos;

	if (isEquipmentStore)
	{
		return MusicName::Equipment;
	}
	else if (isHouse)
	{
		return MusicName::Sneaking;
	}
	else if (isMagesGuild)
	{
		return MusicName::Magic;
	}
	else if (isPalace)
	{
		return MusicName::Palace;
	}
	else if (isTavern)
	{
		const std::array<MusicName, 2> TavernMusics =
		{
			MusicName::Square,
			MusicName::Tavern
		};

		return TavernMusics.at(random.next(static_cast<int>(TavernMusics.size())));
	}
	else if (isTemple)
	{
		return MusicName::Temple;
	}
	else
	{
		// Dungeon.
		return GameData::getDungeonMusicName(random);
	}
}

void GameData::loadInterior(const MIFFile &mif, const Location &location,
	TextureManager &textureManager, Renderer &renderer)
{
	// Call interior WorldData loader.
	this->worldData = WorldData::loadInterior(mif);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData.getStartPoints().front();
	const auto &levelData = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	this->player.teleport(Double3(
		startPoint.x, levelData.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

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

void GameData::loadNamedDungeon(int localDungeonID, int provinceID, bool isArtifactDungeon,
	TextureManager &textureManager, Renderer &renderer)
{
	// Dungeon ID must be for a named dungeon, not main quest dungeon.
	DebugAssert(localDungeonID >= 2, "Dungeon ID \"" + std::to_string(localDungeonID) +
		"\" must not be for main quest dungeon.");

	// Generate dungeon seed.
	const uint32_t dungeonSeed = this->cityData.getDungeonSeed(localDungeonID, provinceID);

	// Call dungeon WorldData loader with parameters specific to named dungeons.
	const int widthChunks = 2;
	const int depthChunks = 1;
	this->worldData = WorldData::loadDungeon(
		dungeonSeed, widthChunks, depthChunks, isArtifactDungeon);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData.getStartPoints().front();
	const auto &levelData = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	this->player.teleport(Double3(
		startPoint.x - 1.0, levelData.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location.
	this->location = Location::makeDungeon(localDungeonID, provinceID);

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

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData.getStartPoints().front();
	const auto &levelData = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	this->player.teleport(Double3(
		startPoint.x - 1.0, levelData.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location (since wilderness dungeons aren't their own location, use a placeholder
	// value for testing).
	this->location = Location::makeSpecialCase(Location::SpecialCaseType::WildDungeon, provinceID);

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

void GameData::loadPremadeCity(const MIFFile &mif, WeatherType weatherType,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	// Climate for center province.
	const int localCityID = 0;
	const int provinceID = Location::CENTER_PROVINCE_ID;
	const ClimateType climateType = Location::getCityClimateType(
		localCityID, provinceID, miscAssets);

	// Call premade WorldData loader.
	this->worldData = WorldData::loadPremadeCity(mif, climateType, weatherType);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = this->worldData.getStartPoints().front();
	const auto &levelData = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	this->player.teleport(Double3(
		startPoint.x, levelData.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location.
	this->location = Location::makeCity(localCityID, provinceID);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));
	
	// Set weather, fog, and night lights.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->clock.nightLightsAreActive());
}

void GameData::loadCity(int localCityID, int provinceID, WeatherType weatherType,
	const MiscAssets &miscAssets, TextureManager &textureManager, Renderer &renderer)
{
	const int globalCityID = CityDataFile::getGlobalCityID(localCityID, provinceID);

	// Check that the IDs are in the proper range. Although 256 is a valid city ID,
	// loadPremadeCity() should be called instead for that case.
	DebugAssert(provinceID != Location::CENTER_PROVINCE_ID,
		"Use loadPremadeCity() instead for center province.");
	DebugAssert((globalCityID >= 0) && (globalCityID < 256),
		"Invalid city ID \"" + std::to_string(globalCityID) + "\".");
	
	// Determine city traits from the given city ID.
	const LocationType locationType = Location::getCityType(localCityID);
	const ExeData::CityGeneration &cityGen = miscAssets.getExeData().cityGen;
	const bool isCityState = locationType == LocationType::CityState;
	const bool isCoastal = std::find(cityGen.coastalCityList.begin(),
		cityGen.coastalCityList.end(), globalCityID) != cityGen.coastalCityList.end();
	const int templateCount = CityDataFile::getCityTemplateCount(isCoastal, isCityState);
	const int templateID = globalCityID % templateCount;

	const MIFFile mif = [locationType, &cityGen, isCoastal, templateID]()
	{
		// Get the index into the template names array (town%d.mif, ..., cityw%d.mif).
		const int nameIndex = CityDataFile::getCityTemplateNameIndex(locationType, isCoastal);

		// Get the template name associated with the city ID.
		std::string templateName = cityGen.templateFilenames.at(nameIndex);
		templateName = String::replace(templateName, "%d", std::to_string(templateID + 1));
		templateName = String::toUppercase(templateName);

		return MIFFile(templateName);
	}();

	// City block count (6x6, 5x5, 4x4).
	const int cityDim = CityDataFile::getCityDimensions(locationType);

	// Get the reserved block list for the given city.
	const std::vector<uint8_t> &reservedBlocks = [&cityGen, isCoastal, templateID]()
	{
		const int index = CityDataFile::getCityReservedBlockListIndex(isCoastal, templateID);
		return cityGen.reservedBlockLists.at(index);
	}();

	// Get the starting position of city blocks within the city skeleton.
	const Int2 startPosition = [locationType, &cityGen, isCoastal, templateID]()
	{
		const int index = CityDataFile::getCityStartingPositionIndex(
			locationType, isCoastal, templateID);

		const auto &pair = cityGen.startingPositions.at(index);
		return Int2(pair.first, pair.second);
	}();

	// Call city WorldData loader.
	this->worldData = WorldData::loadCity(localCityID, provinceID, mif, cityDim,
		reservedBlocks, startPosition, weatherType, miscAssets);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set player starting position and velocity.
	const Double2 &startPoint = worldData.getStartPoints().front();
	const auto &levelData = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	this->player.teleport(Double3(
		startPoint.x, levelData.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location.
	this->location = Location::makeCity(localCityID, provinceID);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	// Set weather, fog, and night lights.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->clock.nightLightsAreActive());
}

void GameData::loadWilderness(int localCityID, int provinceID, int rmdTR, int rmdTL, int rmdBR,
	int rmdBL, WeatherType weatherType, const MiscAssets &miscAssets,
	TextureManager &textureManager, Renderer &renderer)
{
	// Get the location's climate type.
	const ClimateType climateType = Location::getCityClimateType(
		localCityID, provinceID, miscAssets);

	// Call wilderness WorldData loader.
	this->worldData = WorldData::loadWilderness(
		rmdTR, rmdTL, rmdBR, rmdBL, climateType, weatherType);
	this->worldData.setLevelActive(this->worldData.getCurrentLevel(), textureManager, renderer);

	// Set arbitrary player starting position and velocity (no starting point in WILD.MIF).
	const Double2 startPoint(63.50, 63.50);
	const auto &levelData = this->worldData.getLevels().at(this->worldData.getCurrentLevel());
	this->player.teleport(Double3(
		startPoint.x, levelData.getCeilingHeight() + Player::HEIGHT, startPoint.y));
	this->player.setVelocityToZero();

	// Set location.
	this->location = Location::makeCity(localCityID, provinceID);

	// Regular sky palette based on weather.
	const std::vector<uint32_t> skyPalette =
		GameData::makeExteriorSkyPalette(weatherType, textureManager);
	renderer.setSkyPalette(skyPalette.data(), static_cast<int>(skyPalette.size()));

	// Set weather, fog, and night lights.
	const double fogDistance = GameData::getFogDistanceFromWeather(weatherType);
	this->weatherType = weatherType;
	this->fogDistance = fogDistance;
	renderer.setFogDistance(fogDistance);
	renderer.setNightLightsActive(this->clock.nightLightsAreActive());
}

GameData::TimedTextBox &GameData::getTriggerText()
{
	return this->triggerText;
}

GameData::TimedTextBox &GameData::getActionText()
{
	return this->actionText;
}

GameData::TimedTextBox &GameData::getEffectText()
{
	return this->effectText;
}

const std::array<WeatherType, 36> &GameData::getWeathersArray() const
{
	return this->weathers;
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

CityDataFile &GameData::getCityDataFile()
{
	return this->cityData;
}

Date &GameData::getDate()
{
	return this->date;
}

Clock &GameData::getClock()
{
	return this->clock;
}

ArenaRandom &GameData::getRandom()
{
	return this->arenaRandom;
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

WeatherType GameData::getWeatherType() const
{
	return this->weatherType;
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

void GameData::updateWeather(const ExeData &exeData)
{
	const int seasonIndex = this->date.getSeason();

	for (size_t i = 0; i < this->weathers.size(); i++)
	{
		const int climateIndex = exeData.locations.climates.at(i);
		const int variantIndex = [this]()
		{
			// 40% for 2, 20% for 1, 20% for 3, 10% for 0, and 10% for 4.
			const int val = this->arenaRandom.next() % 100;

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
		this->updateWeather(exeData);
	}

	// Check if the clock hour looped back around.
	if (newHour < oldHour)
	{
		// Increment the day.
		this->date.incrementDay();
	}
}
