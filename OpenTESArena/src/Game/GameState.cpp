#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>

#include "SDL.h"

#include "ArenaClockUtils.h"
#include "Game.h"
#include "GameState.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ExeData.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityManager.h"
#include "../Entities/Player.h"
#include "../Interface/TextAlignment.h"
#include "../Interface/TextBox.h"
#include "../Math/Constants.h"
#include "../Media/FontName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../World/ArenaVoxelUtils.h"
#include "../World/LocationDefinition.h"
#include "../World/LocationInstance.h"
#include "../World/LocationType.h"
#include "../World/LocationUtils.h"
#include "../World/MapType.h"
#include "../World/VoxelGrid.h"
#include "../World/WeatherType.h"
#include "../World/WeatherUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	// Colors for UI text.
	const Color TriggerTextColor(215, 121, 8);
	const Color TriggerTextShadowColor(12, 12, 24);
	const Color ActionTextColor(195, 0, 0);
	const Color ActionTextShadowColor(12, 12, 24);
	const Color EffectTextColor(251, 239, 77);
	const Color EffectTextShadowColor(190, 113, 0);
}

void GameState::MapPair::init(MapDefinition &&mapDefinition, MapInstance &&mapInstance)
{
	this->definition = std::move(mapDefinition);
	this->instance = std::move(mapInstance);
}

GameState::GameState(Player &&player, const BinaryAssetLibrary &binaryAssetLibrary)
	: player(std::move(player))
{
	// Most values need to be initialized elsewhere in the program in order to determine
	// the world state, etc..
	DebugLog("Initializing.");

	// Initialize world map definition and instance to default.
	this->worldMapDef.init(binaryAssetLibrary);
	this->worldMapInst.init(this->worldMapDef);

	// @temp: set main quest dungeons visible for testing.
	for (int i = 0; i < this->worldMapInst.getProvinceCount(); i++)
	{
		ProvinceInstance &provinceInst = this->worldMapInst.getProvinceInstance(i);
		const int provinceDefIndex = provinceInst.getProvinceDefIndex();
		const ProvinceDefinition &provinceDef = this->worldMapDef.getProvinceDef(provinceDefIndex);

		for (int j = 0; j < provinceInst.getLocationCount(); j++)
		{
			LocationInstance &locationInst = provinceInst.getLocationInstance(j);
			const int locationDefIndex = locationInst.getLocationDefIndex();
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationDefIndex);
			const std::string &locationName = locationInst.getName(locationDef);

			const bool isMainQuestDungeon = locationDef.getType() == LocationDefinition::Type::MainQuestDungeon;
			const bool isStartDungeon = isMainQuestDungeon &&
				(locationDef.getMainQuestDungeonDefinition().type == LocationDefinition::MainQuestDungeonDefinition::Type::Start);
			const bool shouldSetVisible = (locationName.size() > 0) &&
				isMainQuestDungeon && !isStartDungeon && !locationInst.isVisible();

			if (shouldSetVisible)
			{
				locationInst.toggleVisibility();
			}
		}
	}

	// Do initial weather update (to set each value to a valid state).
	this->updateWeather(binaryAssetLibrary.getExeData());

	this->provinceIndex = -1;
	this->locationIndex = -1;
	this->chasmAnimSeconds = 0.0;
}

GameState::~GameState()
{
	DebugLog("Closing.");
}

bool GameState::tryMakeMapFromLocation(const LocationDefinition &locationDef, int raceID, WeatherType weatherType,
	int currentDay, int starCount, bool provinceHasAnimatedLand, const CharacterClassLibrary &charClassLibrary,
	const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
	const TextAssetLibrary &textAssetLibrary, TextureManager &textureManager, MapPair *outMapPair)
{
	// Decide how to load and instantiate the map.
	const LocationDefinition::Type locationType = locationDef.getType();
	if (locationType == LocationDefinition::Type::City)
	{
		const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
		Buffer<uint8_t> reservedBlocks = [&cityDef]()
		{
			DebugAssert(cityDef.reservedBlocks != nullptr);
			Buffer<uint8_t> buffer(cityDef.reservedBlocks->size());
			std::copy(cityDef.reservedBlocks->begin(), cityDef.reservedBlocks->end(), buffer.get());
			return buffer;
		}();
		
		const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
			[&cityDef]() -> std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
		{
			if (cityDef.hasMainQuestTempleOverride)
			{
				return cityDef.mainQuestTempleOverride;
			}
			else
			{
				return std::nullopt;
			}
		}();

		MapGeneration::CityGenInfo cityGenInfo;
		cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName), cityDef.type,
			cityDef.citySeed, cityDef.rulerSeed, raceID, cityDef.premade, cityDef.coastal,
			cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks), mainQuestTempleOverride,
			cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

		SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, weatherType, currentDay, starCount, cityDef.citySeed,
			cityDef.distantSkySeed, provinceHasAnimatedLand);

		MapDefinition mapDefinition;
		if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, charClassLibrary, entityDefLibrary,
			binaryAssetLibrary, textAssetLibrary, textureManager))
		{
			DebugLogError("Couldn't init city map for location \"" + locationDef.getName() + "\".");
			return false;
		}

		MapInstance mapInstance;
		mapInstance.init(mapDefinition, textureManager);
		outMapPair->init(std::move(mapDefinition), std::move(mapInstance));
	}
	else if (locationType == LocationDefinition::Type::Dungeon)
	{
		const LocationDefinition::DungeonDefinition &dungeonDef = locationDef.getDungeonDefinition();

		MapGeneration::InteriorGenInfo interiorGenInfo;
		constexpr bool isArtifactDungeon = false; // @todo: not supported yet.
		interiorGenInfo.initDungeon(&dungeonDef, isArtifactDungeon);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, charClassLibrary, entityDefLibrary,
			binaryAssetLibrary, textureManager))
		{
			DebugLogError("Couldn't init dungeon map for location \"" + locationDef.getName() + "\".");
			return false;
		}

		MapInstance mapInstance;
		mapInstance.init(mapDefinition, textureManager);
		outMapPair->init(std::move(mapDefinition), std::move(mapInstance));
	}
	else if (locationType == LocationDefinition::Type::MainQuestDungeon)
	{
		const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
			locationDef.getMainQuestDungeonDefinition();

		MapGeneration::InteriorGenInfo interiorGenInfo;
		constexpr std::optional<bool> rulerIsMale; // Unused for main quest dungeons.
		interiorGenInfo.initPrefab(std::string(mainQuestDungeonDef.mapFilename),
			ArenaTypes::InteriorType::Dungeon, rulerIsMale);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, charClassLibrary, entityDefLibrary,
			binaryAssetLibrary, textureManager))
		{
			DebugLogError("Couldn't init main quest dungeon map for location \"" + locationDef.getName() + "\".");
			return false;
		}

		MapInstance mapInstance;
		mapInstance.init(mapDefinition, textureManager);
		outMapPair->init(std::move(mapDefinition), std::move(mapInstance));
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(locationType)));
	}

	return true;
}

bool GameState::trySetFromWorldMap(int provinceID, int locationID, int currentDay, int starCount,
	const CharacterClassLibrary &charClassLibrary, const EntityDefinitionLibrary &entityDefLibrary,
	const BinaryAssetLibrary &binaryAssetLibrary, const TextAssetLibrary &textAssetLibrary,
	TextureManager &textureManager, Renderer &renderer)
{
	// Get the province and location definitions.
	if ((provinceID < 0) || (provinceID >= this->worldMapDef.getProvinceCount()))
	{
		DebugLogError("Invalid province ID \"" + std::to_string(provinceID) + "\".");
		return false;
	}

	const ProvinceDefinition &provinceDef = this->worldMapDef.getProvinceDef(provinceID);
	if ((locationID < 0) || (locationID >= provinceDef.getLocationCount()))
	{
		DebugLogError("Invalid location ID \"" + std::to_string(locationID) + "\" for province \"" + provinceDef.getName() + "\".");
		return false;
	}

	const LocationDefinition &locationDef = provinceDef.getLocationDef(locationID);

	MapPair mapPair;
	if (!GameState::tryMakeMapFromLocation(locationDef, provinceDef.getRaceID(), weatherType, currentDay,
		starCount, provinceDef.hasAnimatedDistantLand(), charClassLibrary, entityDefLibrary, binaryAssetLibrary,
		textAssetLibrary, textureManager, &mapPair))
	{
		DebugLogError("Couldn't make map from location \"" + locationDef.getName() + "\" in province \"" + provinceDef.getName() + "\".");
		return false;
	}

	this->clearMaps();
	this->maps.emplace(std::move(mapPair));

	const MapDefinition &activeMapDef = this->getActiveMapDef();
	MapInstance &activeMapInst = this->getActiveMapInst();
	LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();

	constexpr WeatherType weatherType = WeatherType::Clear; // @todo: generate weather type here, but only if it's an exterior. Interiors are always clear?

	DebugAssert(activeMapDef.getStartPointCount() > 0);
	const NewDouble2 &startPoint = activeMapDef.getStartPoint(0);
	const NewDouble3 newPlayerPos(
		startPoint.x,// @todo: may need some per-map-definition or map-type offset. Just +1.0 for procedural dungeons.
		activeLevelInst.getCeilingScale() + Player::HEIGHT,
		startPoint.y);

	// Set level active in the renderer.
	if (!this->trySetLevelActive(activeLevelInst, weatherType, newPlayerPos, textureManager, renderer))
	{
		DebugLogError("Couldn't set level active in the renderer for location \"" + locationDef.getName() + "\".");
		return false;
	}

	return true;
}

bool GameState::tryPushInterior(const MapGeneration::InteriorGenInfo &interiorGenInfo)
{
	// @todo: save returnVoxel
	DebugNotImplemented();
	return true;
}

bool GameState::trySetCity(const MapGeneration::CityGenInfo &cityGenInfo)
{
	DebugNotImplemented();
	return true;
}

bool GameState::trySetWilderness(const MapGeneration::WildGenInfo &wildGenInfo)
{
	// @todo: try to get gate position if current active map is for city -- need to have saved it from when the
	// gate was clicked in GameWorldPanel.

	DebugNotImplemented();
	return true;
}

bool GameState::tryPopMap()
{
	DebugNotImplemented();
	return true;
}

Player &GameState::getPlayer()
{
	return this->player;
}

const MapDefinition &GameState::getActiveMapDef() const
{
	DebugAssert(!this->maps.empty());
	const MapPair &pair = this->maps.top();
	return pair.definition;
}

MapInstance &GameState::getActiveMapInst()
{
	DebugAssert(!this->maps.empty());
	MapPair &pair = this->maps.top();
	return pair.instance;
}

const MapInstance &GameState::getActiveMapInst() const
{
	DebugAssert(!this->maps.empty());
	const MapPair &pair = this->maps.top();
	return pair.instance;
}

bool GameState::isActiveMapNested() const
{
	return this->maps.size() >= 2;
}

CitizenManager &GameState::getCitizenManager()
{
	return this->citizenManager;
}

WorldMapInstance &GameState::getWorldMapInstance()
{
	return this->worldMapInst;
}

const WorldMapDefinition &GameState::getWorldMapDefinition() const
{
	return this->worldMapDef;
}

const ProvinceDefinition &GameState::getProvinceDefinition() const
{
	return this->worldMapDef.getProvinceDef(this->provinceIndex);
}

const LocationDefinition &GameState::getLocationDefinition() const
{
	const ProvinceDefinition &provinceDef = this->getProvinceDefinition();
	return provinceDef.getLocationDef(this->locationIndex);
}

ProvinceInstance &GameState::getProvinceInstance()
{
	return this->worldMapInst.getProvinceInstance(this->provinceIndex);
}

LocationInstance &GameState::getLocationInstance()
{
	ProvinceInstance &provinceInst = this->getProvinceInstance();
	return provinceInst.getLocationInstance(this->locationIndex);
}

const GameState::WeatherList &GameState::getWeathersArray() const
{
	return this->weathers;
}

Date &GameState::getDate()
{
	return this->date;
}

Clock &GameState::getClock()
{
	return this->clock;
}

ArenaRandom &GameState::getRandom()
{
	return this->arenaRandom;
}

double GameState::getDaytimePercent() const
{
	return this->clock.getPreciseTotalSeconds() /
		static_cast<double>(Clock::SECONDS_IN_A_DAY);
}

double GameState::getChasmAnimPercent() const
{
	const double percent = this->chasmAnimSeconds / ArenaVoxelUtils::CHASM_ANIM_SECONDS;
	return std::clamp(percent, 0.0, Constants::JustBelowOne);
}

WeatherType GameState::getWeatherType() const
{
	return this->weatherType;
}

double GameState::getAmbientPercent() const
{
	DebugAssert(!this->maps.empty());
	const MapDefinition &activeMapDef = this->maps.top().definition;

	if (activeMapDef.getMapType() == MapType::Interior)
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
		const double startBrighteningTime = ArenaClockUtils::AmbientStartBrightening.getPreciseTotalSeconds();
		const double endBrighteningTime = ArenaClockUtils::AmbientEndBrightening.getPreciseTotalSeconds();
		const double startDimmingTime = ArenaClockUtils::AmbientStartDimming.getPreciseTotalSeconds();
		const double endDimmingTime = ArenaClockUtils::AmbientEndDimming.getPreciseTotalSeconds();

		// In Arena, the min ambient is 0 and the max ambient is 1, but we're using
		// some values here that make testing easier.
		constexpr double minAmbient = 0.15;
		constexpr double maxAmbient = 1.0;

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

double GameState::getBetterAmbientPercent() const
{
	const double daytimePercent = this->getDaytimePercent();
	const double minAmbient = 0.20;
	const double maxAmbient = 0.90;
	const double diff = maxAmbient - minAmbient;
	const double center = minAmbient + (diff / 2.0);
	return center + ((diff / 2.0) * -std::cos(daytimePercent * (2.0 * Constants::Pi)));
}

bool GameState::nightMusicIsActive() const
{
	const double clockTime = this->clock.getPreciseTotalSeconds();
	const bool beforeDayMusicChange = clockTime < ArenaClockUtils::MusicSwitchToDay.getPreciseTotalSeconds();
	const bool afterNightMusicChange = clockTime >= ArenaClockUtils::MusicSwitchToNight.getPreciseTotalSeconds();
	return beforeDayMusicChange || afterNightMusicChange;
}

bool GameState::nightLightsAreActive() const
{
	const double clockTime = this->clock.getPreciseTotalSeconds();
	const bool beforeLamppostDeactivate = clockTime < ArenaClockUtils::LamppostDeactivate.getPreciseTotalSeconds();
	const bool afterLamppostActivate = clockTime >= ArenaClockUtils::LamppostActivate.getPreciseTotalSeconds();
	return beforeLamppostDeactivate || afterLamppostActivate;
}

std::function<void(Game&)> &GameState::getOnLevelUpVoxelEnter()
{
	return this->onLevelUpVoxelEnter;
}

bool GameState::triggerTextIsVisible() const
{
	return this->triggerText.hasRemainingDuration();
}

bool GameState::actionTextIsVisible() const
{
	return this->actionText.hasRemainingDuration();
}

bool GameState::effectTextIsVisible() const
{
	return this->effectText.hasRemainingDuration();
}

void GameState::getTriggerTextRenderInfo(const Texture **outTexture) const
{
	if (outTexture != nullptr)
	{
		*outTexture = &this->triggerText.textBox->getTexture();
	}
}

void GameState::getActionTextRenderInfo(const Texture **outTexture) const
{
	if (outTexture != nullptr)
	{
		*outTexture = &this->actionText.textBox->getTexture();
	}
}

void GameState::getEffectTextRenderInfo(const Texture **outTexture) const
{
	if (outTexture != nullptr)
	{
		*outTexture = &this->effectText.textBox->getTexture();
	}
}

void GameState::setTriggerText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer)
{
	const int lineSpacing = 1;
	const RichTextString richText(
		text,
		FontName::Arena,
		TriggerTextColor,
		TextAlignment::Center,
		lineSpacing,
		fontLibrary);

	const TextBox::ShadowData shadowData(TriggerTextShadowColor, Int2(-1, 0));

	// Create the text box for display (set position to zero; the renderer will
	// decide where to draw it).
	auto textBox = std::make_unique<TextBox>(
		Int2(0, 0),
		richText,
		&shadowData,
		fontLibrary,
		renderer);

	// Assign the text box and its duration to the triggered text member.
	const double duration = std::max(2.50, static_cast<double>(text.size()) * 0.050);
	this->triggerText = TimedTextBox(duration, std::move(textBox));
}

void GameState::setActionText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer)
{
	const RichTextString richText(
		text,
		FontName::Arena,
		ActionTextColor,
		TextAlignment::Center,
		fontLibrary);

	const TextBox::ShadowData shadowData(ActionTextShadowColor, Int2(-1, 0));

	// Create the text box for display (set position to zero; the renderer will decide
	// where to draw it).
	auto textBox = std::make_unique<TextBox>(
		Int2(0, 0),
		richText,
		&shadowData,
		fontLibrary,
		renderer);

	// Assign the text box and its duration to the action text.
	const double duration = std::max(2.25, static_cast<double>(text.size()) * 0.050);
	this->actionText = TimedTextBox(duration, std::move(textBox));
}

void GameState::setEffectText(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer)
{
	// @todo
}

void GameState::resetTriggerText()
{
	this->triggerText.reset();
}

void GameState::resetActionText()
{
	this->actionText.reset();
}

void GameState::resetEffectText()
{
	this->effectText.reset();
}

void GameState::setTransitionedPlayerPosition(const NewDouble3 &position)
{
	const CoordDouble3 coord = VoxelUtils::newPointToCoord(position);
	this->player.teleport(coord);
	this->player.setVelocityToZero();
}

bool GameState::trySetLevelActive(LevelInstance &levelInst, WeatherType weatherType, const NewDouble3 &playerPosition,
	TextureManager &textureManager, Renderer &renderer)
{
	this->setTransitionedPlayerPosition(playerPosition);
	this->weatherType = weatherType;

	if (!levelInst.trySetActive(weatherType, this->nightLightsAreActive(), textureManager, renderer))
	{
		DebugLogError("Couldn't set level active in the renderer.");
		return false;
	}

	return true;
}

void GameState::clearMaps()
{
	while (!this->maps.empty())
	{
		this->maps.pop();
	}
}

void GameState::updateWeather(const ExeData &exeData)
{
	const int seasonIndex = this->date.getSeason();

	for (size_t i = 0; i < this->weathers.size(); i++)
	{
		static_assert(std::tuple_size<decltype(exeData.locations.climates)>::value ==
			std::tuple_size<decltype(this->weathers)>::value);
		
		const int climateIndex = exeData.locations.climates[i];
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
		this->weathers[i] = static_cast<WeatherType>(
			exeData.locations.weatherTable.at(weatherTableIndex));
	}
}

void GameState::tick(double dt, Game &game)
{
	DebugAssert(dt >= 0.0);

	// Tick the game clock.
	const int oldHour = this->clock.getHours24();
	this->clock.tick(dt * GameState::TIME_SCALE);
	const int newHour = this->clock.getHours24();

	// Check if the hour changed.
	if (newHour != oldHour)
	{
		// Update the weather.
		const auto &exeData = game.getBinaryAssetLibrary().getExeData();
		this->updateWeather(exeData);
	}

	// Check if the clock hour looped back around.
	if (newHour < oldHour)
	{
		// Increment the day.
		this->date.incrementDay();
	}

	// Tick chasm animation.
	this->chasmAnimSeconds += dt;
	if (this->chasmAnimSeconds >= ArenaVoxelUtils::CHASM_ANIM_SECONDS)
	{
		this->chasmAnimSeconds = std::fmod(this->chasmAnimSeconds, ArenaVoxelUtils::CHASM_ANIM_SECONDS);
	}

	// Tick on-screen text messages.
	auto tryTickTextBox = [dt](TimedTextBox &textBox)
	{
		if (textBox.hasRemainingDuration())
		{
			textBox.remainingDuration -= dt;
		}
	};

	tryTickTextBox(this->triggerText);
	tryTickTextBox(this->actionText);
	tryTickTextBox(this->effectText);

	// Tick citizen manager state (check if new citizens should spawn, etc.).
	this->citizenManager.tick(game);
}