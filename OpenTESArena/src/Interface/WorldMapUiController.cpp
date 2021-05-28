#include "FastTravelSubPanel.h"
#include "GameWorldPanel.h"
#include "MainQuestSplashPanel.h"
#include "WorldMapUiController.h"
#include "WorldMapUiModel.h"
#include "../Game/Game.h"
#include "../World/ArenaWeatherUtils.h"
#include "../World/SkyUtils.h"
#include "../WorldMap/LocationUtils.h"

void WorldMapUiController::onFastTravelAnimationFinished(Game &game, int targetProvinceID,
	int targetLocationID, int travelDays)
{
	// Handle fast travel behavior and decide which panel to switch to.
	auto &gameState = game.getGameState();
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();

	// Update game clock.
	WorldMapUiModel::tickTravelTime(game, travelDays);

	// Update weathers.
	gameState.updateWeatherList(exeData);

	// Clear the lore text (action text and effect text are unchanged).
	gameState.resetTriggerText();

	// Clear any on-voxel-enter event to avoid things like fast travelling out of the
	// starting dungeon then being teleported to a random city when going through any
	// subsequent LEVELUP voxel.
	gameState.getOnLevelUpVoxelEnter() = std::function<void(Game&)>();

	// Pop this sub-panel on the next game loop. The game loop pops old sub-panels before
	// pushing new ones, so call order doesn't matter.
	game.popSubPanel();

	const auto &travelProvinceDef = worldMapDef.getProvinceDef(targetProvinceID);
	const auto &travelLocationDef = travelProvinceDef.getLocationDef(targetLocationID);

	// Decide how to load the location.
	if (travelLocationDef.getType() == LocationDefinition::Type::City)
	{
		// Get weather type from game state.
		const LocationDefinition::CityDefinition &cityDef = travelLocationDef.getCityDefinition();
		const ArenaTypes::WeatherType weatherType = [&game, &gameState, &binaryAssetLibrary,
			&travelProvinceDef, &travelLocationDef, &cityDef]()
		{
			const Int2 localPoint(travelLocationDef.getScreenX(), travelLocationDef.getScreenY());
			const Int2 globalPoint = LocationUtils::getGlobalPoint(localPoint, travelProvinceDef.getGlobalRect());

			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const int globalQuarter = LocationUtils::getGlobalQuarter(globalPoint, cityData);

			const auto &weathersArray = gameState.getWeathersArray();
			DebugAssertIndex(weathersArray, globalQuarter);
			return ArenaWeatherUtils::getFilteredWeatherType(weathersArray[globalQuarter], cityDef.climateType);
		}();

		const int starCount = SkyUtils::getStarCountFromDensity(game.getOptions().getMisc_StarDensity());

		// Get city generation values.
		Buffer<uint8_t> reservedBlocks = [&cityDef]()
		{
			const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
			DebugAssert(cityReservedBlocks != nullptr);
			Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
			std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.get());
			return buffer;
		}();

		const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
			[&cityDef]() ->std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
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

		// Load the destination city.
		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);
		if (!gameState.trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), game.getBinaryAssetLibrary(),
			game.getTextAssetLibrary(), game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load city \"" + travelLocationDef.getName() + "\".");
		}

		// Choose time-based music and enter the game world.
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = [&game, &gameState, &overrideWeather, &musicLibrary]()
		{
			if (!gameState.nightMusicIsActive())
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

		game.setPanel<GameWorldPanel>(game);

		// Push a text sub-panel for the city arrival pop-up.
		std::unique_ptr<Panel> arrivalPopUp = WorldMapUiModel::makeCityArrivalPopUp(
			game, targetProvinceID, targetLocationID, travelDays);
		game.pushSubPanel(std::move(arrivalPopUp));
	}
	else if (travelLocationDef.getType() == LocationDefinition::Type::Dungeon)
	{
		// Random named dungeon.
		constexpr bool isArtifactDungeon = false;
		const auto &travelProvinceDef = worldMapDef.getProvinceDef(targetProvinceID);
		const auto &travelLocationDef = travelProvinceDef.getLocationDef(targetLocationID);
		const LocationDefinition::DungeonDefinition &dungeonDef = travelLocationDef.getDungeonDefinition();

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

		const std::optional<VoxelInt2> playerStartOffset = VoxelInt2(
			ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_X,
			ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_Z);

		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);
		if (!gameState.trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), game.getBinaryAssetLibrary(),
			game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load named dungeon \"" + travelLocationDef.getName() + "\".");
		}

		// Choose random dungeon music and enter game world.
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
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

		game.setPanel<GameWorldPanel>(game);
	}
	else if (travelLocationDef.getType() == LocationDefinition::Type::MainQuestDungeon)
	{
		// Main quest dungeon. The staff dungeons have a splash image before going to the game world panel.
		const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
			travelLocationDef.getMainQuestDungeonDefinition();

		constexpr std::optional<bool> rulerIsMale; // Not needed.

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initPrefab(std::string(mainQuestDungeonDef.mapFilename),
			ArenaTypes::InteriorType::Dungeon, rulerIsMale);

		const std::optional<VoxelInt2> playerStartOffset; // Unused for main quest dungeon.

		const GameState::WorldMapLocationIDs worldMapLocationIDs(targetProvinceID, targetLocationID);
		if (!gameState.trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), game.getBinaryAssetLibrary(),
			game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load main quest interior \"" + travelLocationDef.getName() + "\".");
		}

		if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Staff)
		{
			// Go to staff dungeon splash image first.
			game.setPanel<MainQuestSplashPanel>(game, targetProvinceID);
		}
		else
		{
			// Choose random dungeon music and enter game world.
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
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

			game.setPanel<GameWorldPanel>(game);
		}
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(travelLocationDef.getType())));
	}
}

void WorldMapUiController::onFastTravelCityArrivalPopUpSelected(Game &game)
{
	game.popSubPanel();
}
