#include "FastTravelSubPanel.h"
#include "GameWorldPanel.h"
#include "MainQuestSplashPanel.h"
#include "ProvinceMapPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Sky/SkyUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../WorldMap/ArenaLocationUtils.h"

void WorldMapUiController::onBackToGameButtonSelected(Game &game)
{
	// Clear selected map location.
	auto &gameState = game.gameState;
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
	auto &gameState = game.gameState;
	gameState.setTravelData(std::nullopt);

	// Handle fast travel behavior and decide which panel to switch to.
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();

	// Update game clock.
	// @todo: maybe move this to a WorldMapLogicController namespace
	FastTravelUiModel::tickTravelTime(game, travelDays);

	// Update weathers.
	gameState.updateWeatherList(game.arenaRandom, exeData);

	// Clear the lore text (action text and effect text are unchanged).
	gameState.resetTriggerTextDuration();

	// Clear keys inventory in case we're leaving a main quest dungeon.
	Player &player = game.player;
	player.clearKeyInventory();

	// Pop this sub-panel on the next game loop. The game loop pops old sub-panels before
	// pushing new ones, so call order doesn't matter.
	game.popSubPanel();

	const auto &travelProvinceDef = worldMapDef.getProvinceDef(targetProvinceID);
	const auto &travelLocationDef = travelProvinceDef.getLocationDef(targetLocationID);

	TextureManager &textureManager = game.textureManager;

	// Decide how to load the location.
	if (travelLocationDef.getType() == LocationDefinitionType::City)
	{
		// Get weather type from game state.
		const LocationCityDefinition &cityDef = travelLocationDef.getCityDefinition();
		const ArenaWeatherType weatherType = [&game, &gameState, &binaryAssetLibrary,
			&travelProvinceDef, &travelLocationDef, &cityDef]()
		{
			const Int2 localPoint(travelLocationDef.getScreenX(), travelLocationDef.getScreenY());
			const Int2 globalPoint = ArenaLocationUtils::getGlobalPoint(localPoint, travelProvinceDef.getGlobalRect());

			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const int globalQuarter = ArenaLocationUtils::getGlobalQuarter(globalPoint, cityData);

			Span<const ArenaWeatherType> worldMapWeathers = gameState.getWorldMapWeathers();
			DebugAssertIndex(worldMapWeathers, globalQuarter);
			return ArenaWeatherUtils::getFilteredWeatherType(worldMapWeathers[globalQuarter], cityDef.climateType);
		}();

		const int starCount = SkyUtils::getStarCountFromDensity(game.options.getMisc_StarDensity());

		// Get city generation values.
		Buffer<uint8_t> reservedBlocks = [&cityDef]()
		{
			const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
			DebugAssert(cityReservedBlocks != nullptr);
			Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
			std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.begin());
			return buffer;
		}();

		const std::optional<LocationCityMainQuestTempleOverride> mainQuestTempleOverride =
			[&cityDef]() -> std::optional<LocationCityMainQuestTempleOverride>
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

		MapGenerationCityInfo cityGenInfo;
		cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName), cityDef.type,
			cityDef.citySeed, cityDef.rulerSeed, travelProvinceDef.getRaceID(), cityDef.premade, cityDef.coastal,
			cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks), mainQuestTempleOverride,
			cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

		const int currentDay = gameState.getDate().getDay();
		const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
		{
			WeatherDefinition weatherDef;
			weatherDef.initFromClassic(weatherType, currentDay, game.random);
			return weatherDef;
		}();

		SkyGenerationExteriorInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
			cityDef.skySeed, travelProvinceDef.hasAnimatedDistantLand());

		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);

		MapDefinition mapDefinition;
		if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, textureManager))
		{
			DebugCrash("Couldn't init MapDefinition for city \"" + travelLocationDef.getName() + "\".");
			return;
		}

		GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
		{
			// Choose time-based music and enter the game world.
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
			const GameState &gameState = game.gameState;
			const MusicDefinition *musicDef = MusicUtils::getExteriorMusicDefinition(gameState.getWeatherDefinition(), gameState.getClock(), game.random);
			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing exterior music.");
			}

			return musicDef;
		};

		const ArenaCityType cityDefType = cityDef.type;
		const ArenaClimateType cityDefClimateType = cityDef.climateType;
		GameState::SceneChangeMusicFunc jingleMusicFunc = [cityDefType, cityDefClimateType](Game &game)
		{
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
			const MusicDefinition *jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicType::Jingle, game.random,
				[cityDefType, cityDefClimateType](const MusicDefinition &def)
			{
				DebugAssert(def.type == MusicType::Jingle);
				const JingleMusicDefinition &jingleMusicDef = def.jingle;
				return (jingleMusicDef.cityType == cityDefType) && (jingleMusicDef.climateType == cityDefClimateType);
			});

			if (jingleMusicDef == nullptr)
			{
				DebugLogWarning("Missing jingle music.");
			}

			return jingleMusicDef;
		};

		// Load the destination city.
		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);
		gameState.queueMusicOnSceneChange(musicFunc, jingleMusicFunc);

		game.setPanel<GameWorldPanel>();

		// Push a text sub-panel for the city arrival pop-up.
		std::unique_ptr<Panel> arrivalPopUp = FastTravelUiModel::makeCityArrivalPopUp(
			game, targetProvinceID, targetLocationID, travelDays);
		game.pushSubPanel(std::move(arrivalPopUp));
	}
	else if (travelLocationDef.getType() == LocationDefinitionType::Dungeon)
	{
		// Named dungeon.
		constexpr bool isArtifactDungeon = false;
		const auto &travelProvinceDef = worldMapDef.getProvinceDef(targetProvinceID);
		const auto &travelLocationDef = travelProvinceDef.getLocationDef(targetLocationID);
		const LocationDungeonDefinition &dungeonDef = travelLocationDef.getDungeonDefinition();

		MapGenerationInteriorInfo interiorGenInfo;
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

		GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
		{
			const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing dungeon music.");
			}

			return musicDef;
		};

		// Always use clear weather in interiors.
		WeatherDefinition overrideWeather;
		overrideWeather.initClear();

		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, playerStartOffset, worldMapLocationIDs, true, overrideWeather);
		gameState.queueMusicOnSceneChange(musicFunc);

		game.setPanel<GameWorldPanel>();
	}
	else if (travelLocationDef.getType() == LocationDefinitionType::MainQuestDungeon)
	{
		// Main quest dungeon. The staff dungeons have a splash image before going to the game world panel.
		const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = travelLocationDef.getMainQuestDungeonDefinition();

		constexpr std::optional<bool> rulerIsMale; // Not needed.

		MapGenerationInteriorInfo interiorGenInfo;
		interiorGenInfo.initPrefab(mainQuestDungeonDef.mapFilename, ArenaInteriorType::Dungeon, rulerIsMale);

		const std::optional<VoxelInt2> playerStartOffset; // Unused for main quest dungeon.
		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
		{
			DebugLogError("Couldn't init MapDefinition for main quest interior \"" + travelLocationDef.getName() + "\".");
			return;
		}

		// Always use clear weather in interiors.
		WeatherDefinition overrideWeather;
		overrideWeather.initClear();

		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);

		if (mainQuestDungeonDef.type == LocationMainQuestDungeonDefinitionType::Staff)
		{
			// Go to staff dungeon splash image first.
			game.setPanel<MainQuestSplashPanel>(targetProvinceID);
		}
		else
		{
			GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
			{
				const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
				if (musicDef == nullptr)
				{
					DebugLogWarning("Missing dungeon music.");
				}

				return musicDef;
			};

			gameState.queueMusicOnSceneChange(musicFunc);
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
