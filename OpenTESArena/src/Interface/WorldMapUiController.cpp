#include "FastTravelSubPanel.h"
#include "GameWorldPanel.h"
#include "MainQuestSplashPanel.h"
#include "ProvinceMapPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/Game.h"
#include "../Sky/SkyUtils.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../WorldMap/ArenaLocationUtils.h"

void WorldMapUiController::onBackToGameButtonSelected(Game &game)
{
	// Clear selected map location.
	auto &gameState = game.getGameState();
	gameState.setTravelData(std::nullopt);

	game.setPanel<GameWorldPanel>();
}

void WorldMapUiController::onProvinceButtonSelected(Game &game, int provinceID)
{
	game.setPanel<ProvinceMapPanel>(provinceID);
}

void FastTravelUiController::onAnimationFinished(Game &game, int targetProvinceID, int targetLocationID, int travelDays)
{
	// Clear selected map location.
	auto &gameState = game.getGameState();
	gameState.setTravelData(std::nullopt);

	// Handle fast travel behavior and decide which panel to switch to.
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();

	// Update game clock.
	// @todo: maybe move this to a WorldMapLogicController namespace
	FastTravelUiModel::tickTravelTime(game, travelDays);

	// Update weathers.
	gameState.updateWeatherList(exeData);

	// Clear the lore text (action text and effect text are unchanged).
	gameState.resetTriggerTextDuration();

	// Clear any on-voxel-enter event to avoid things like fast travelling out of the
	// starting dungeon then being teleported to a random city when going through any
	// subsequent LEVELUP voxel.
	gameState.getOnLevelUpVoxelEnter() = std::function<void(Game&)>();

	// Pop this sub-panel on the next game loop. The game loop pops old sub-panels before
	// pushing new ones, so call order doesn't matter.
	game.popSubPanel();

	const auto &travelProvinceDef = worldMapDef.getProvinceDef(targetProvinceID);
	const auto &travelLocationDef = travelProvinceDef.getLocationDef(targetLocationID);

	TextureManager &textureManager = game.getTextureManager();

	// Decide how to load the location.
	if (travelLocationDef.getType() == LocationDefinitionType::City)
	{
		// Get weather type from game state.
		const LocationCityDefinition &cityDef = travelLocationDef.getCityDefinition();
		const ArenaTypes::WeatherType weatherType = [&game, &gameState, &binaryAssetLibrary,
			&travelProvinceDef, &travelLocationDef, &cityDef]()
		{
			const Int2 localPoint(travelLocationDef.getScreenX(), travelLocationDef.getScreenY());
			const Int2 globalPoint = ArenaLocationUtils::getGlobalPoint(localPoint, travelProvinceDef.getGlobalRect());

			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const int globalQuarter = ArenaLocationUtils::getGlobalQuarter(globalPoint, cityData);

			BufferView<const ArenaTypes::WeatherType> worldMapWeathers = gameState.getWorldMapWeathers();
			DebugAssertIndex(worldMapWeathers, globalQuarter);
			return ArenaWeatherUtils::getFilteredWeatherType(worldMapWeathers[globalQuarter], cityDef.climateType);
		}();

		const int starCount = SkyUtils::getStarCountFromDensity(game.getOptions().getMisc_StarDensity());

		// Get city generation values.
		Buffer<uint8_t> reservedBlocks = [&cityDef]()
		{
			const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
			DebugAssert(cityReservedBlocks != nullptr);
			Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
			std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.begin());
			return buffer;
		}();

		const std::optional<LocationCityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
			[&cityDef]() -> std::optional<LocationCityDefinition::MainQuestTempleOverride>
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
			cityDef.citySeed, cityDef.rulerSeed, travelProvinceDef.getRaceID(), cityDef.premade, cityDef.coastal,
			cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks), mainQuestTempleOverride,
			cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

		const int currentDay = gameState.getDate().getDay();
		const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
		{
			WeatherDefinition weatherDef;
			weatherDef.initFromClassic(weatherType, currentDay, game.getRandom());
			return weatherDef;
		}();

		SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
			cityDef.skySeed, travelProvinceDef.hasAnimatedDistantLand());

		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);

		MapDefinition mapDefinition;
		if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, textureManager))
		{
			DebugCrash("Couldn't init MapDefinition for city \"" + travelLocationDef.getName() + "\".");
			return;
		}

		// Load the destination city.
		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);

		// Choose time-based music and enter the game world.
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = [&game, &gameState, &overrideWeather, &musicLibrary]()
		{
			const Clock &clock = gameState.getClock();
			if (!ArenaClockUtils::nightMusicIsActive(clock))
			{
				return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
					game.getRandom(), [&overrideWeather](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Weather);
					const auto &weatherMusicDef = def.getWeatherMusicDefinition();
					return weatherMusicDef.weatherDef == overrideWeather;
				});
			}
			else
			{
				return musicLibrary.getRandomMusicDefinition(
					MusicDefinition::Type::Night, game.getRandom());
			}
		}();

		const MusicDefinition *jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(
			MusicDefinition::Type::Jingle, game.getRandom(), [&cityDef](const MusicDefinition &def)
		{
			DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
			const auto &jingleMusicDef = def.getJingleMusicDefinition();
			return (jingleMusicDef.cityType == cityDef.type) &&
				(jingleMusicDef.climateType == cityDef.climateType);
		});

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing exterior music.");
		}

		if (jingleMusicDef == nullptr)
		{
			DebugLogWarning("Missing jingle music.");
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef, jingleMusicDef);

		game.setPanel<GameWorldPanel>();

		// Push a text sub-panel for the city arrival pop-up.
		std::unique_ptr<Panel> arrivalPopUp = FastTravelUiModel::makeCityArrivalPopUp(
			game, targetProvinceID, targetLocationID, travelDays);
		game.pushSubPanel(std::move(arrivalPopUp));
	}
	else if (travelLocationDef.getType() == LocationDefinitionType::Dungeon)
	{
		// Random named dungeon.
		constexpr bool isArtifactDungeon = false;
		const auto &travelProvinceDef = worldMapDef.getProvinceDef(targetProvinceID);
		const auto &travelLocationDef = travelProvinceDef.getLocationDef(targetLocationID);
		const LocationDungeonDefinition &dungeonDef = travelLocationDef.getDungeonDefinition();

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

		const VoxelInt2 playerStartOffset(
			ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_X,
			ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_Z);

		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
		{
			DebugCrash("Couldn't init MapDefinition for named dungeon \"" + travelLocationDef.getName() + "\".");
			return;
		}

		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, playerStartOffset, worldMapLocationIDs, true);

		// Choose random dungeon music and enter game world.
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
			MusicDefinition::Type::Interior, game.getRandom(), [](const MusicDefinition &def)
		{
			DebugAssert(def.getType() == MusicDefinition::Type::Interior);
			const auto &interiorMusicDef = def.getInteriorMusicDefinition();
			return interiorMusicDef.type == MusicDefinition::InteriorMusicDefinition::Type::Dungeon;
		});

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing dungeon music.");
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef);

		game.setPanel<GameWorldPanel>();
	}
	else if (travelLocationDef.getType() == LocationDefinitionType::MainQuestDungeon)
	{
		// Main quest dungeon. The staff dungeons have a splash image before going to the game world panel.
		const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = travelLocationDef.getMainQuestDungeonDefinition();

		constexpr std::optional<bool> rulerIsMale; // Not needed.

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initPrefab(std::string(mainQuestDungeonDef.mapFilename), ArenaTypes::InteriorType::Dungeon, rulerIsMale);

		const std::optional<VoxelInt2> playerStartOffset; // Unused for main quest dungeon.
		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
		{
			DebugLogError("Couldn't init MapDefinition for main quest interior \"" + travelLocationDef.getName() + "\".");
			return;
		}

		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true);

		if (mainQuestDungeonDef.type == LocationMainQuestDungeonDefinitionType::Staff)
		{
			// Go to staff dungeon splash image first.
			game.setPanel<MainQuestSplashPanel>(targetProvinceID);
		}
		else
		{
			// Choose random dungeon music and enter game world.
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicDefinition::Type::Interior, game.getRandom(), [](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Interior);
				const auto &interiorMusicDef = def.getInteriorMusicDefinition();
				return interiorMusicDef.type == MusicDefinition::InteriorMusicDefinition::Type::Dungeon;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing dungeon music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);

			game.setPanel<GameWorldPanel>();
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(travelLocationDef.getType())));
	}
}

void FastTravelUiController::onCityArrivalPopUpSelected(Game &game)
{
	game.popSubPanel();
}
