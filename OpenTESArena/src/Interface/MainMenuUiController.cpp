#include <memory>
#include <vector>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "CinematicPanel.h"
#include "GameWorldPanel.h"
#include "ImageSequencePanel.h"
#include "LoadSavePanel.h"
#include "MainMenuUiController.h"
#include "MainMenuUiModel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Audio/MusicUtils.h"
#include "../Game/Game.h"
#include "../World/MapType.h"
#include "../World/SkyUtils.h"
#include "../WorldMap/ArenaLocationUtils.h"

void MainMenuUiController::onLoadGameButtonSelected(Game &game)
{
	game.setPanel<LoadSavePanel>(LoadSavePanel::Type::Load);
}

void MainMenuUiController::onNewGameButtonSelected(Game &game)
{
	// Link together the opening scroll, intro cinematic, and character creation.
	auto changeToCharCreation = [](Game &game)
	{
		game.setCharacterCreationState(std::make_unique<CharacterCreationState>());
		game.setPanel<ChooseClassCreationPanel>();

		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
			MusicDefinition::Type::CharacterCreation, game.getRandom());

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing character creation music.");
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef);
	};

	auto changeToNewGameStory = [changeToCharCreation](Game &game)
	{
		std::vector<std::string> paletteNames
		{
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG",
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG",
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG"
		};

		std::vector<std::string> textureNames
		{
			"INTRO01.IMG", "INTRO02.IMG", "INTRO03.IMG",
			"INTRO04.IMG", "INTRO05.IMG", "INTRO06.IMG",
			"INTRO07.IMG", "INTRO08.IMG", "INTRO09.IMG"
		};

		std::vector<double> imageDurations
		{
			5.0, 5.0, 5.0,
			5.0, 5.0, 5.0,
			5.0, 5.0, 5.0
		};

		game.setPanel<ImageSequencePanel>(
			paletteNames,
			textureNames,
			imageDurations,
			changeToCharCreation);
	};

	game.setPanel<CinematicPanel>(
		ArenaTextureSequenceName::OpeningScroll,
		ArenaTextureSequenceName::OpeningScroll,
		1.0 / 24.0,
		changeToNewGameStory);

	const MusicLibrary &musicLibrary = game.getMusicLibrary();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
		MusicDefinition::Type::Cinematic, game.getRandom(), [](const MusicDefinition &def)
	{
		DebugAssert(def.getType() == MusicDefinition::Type::Cinematic);
		const auto &cinematicMusicDef = def.getCinematicMusicDefinition();
		return cinematicMusicDef.type == MusicDefinition::CinematicMusicDefinition::Type::Intro;
	});

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing intro music.");
	}

	AudioManager &audioManager = game.getAudioManager();
	audioManager.setMusic(musicDef);
}

void MainMenuUiController::onExitGameButtonSelected()
{
	SDL_Event e;
	e.quit.type = SDL_QUIT;
	e.quit.timestamp = 0;
	SDL_PushEvent(&e);
}

void MainMenuUiController::onQuickStartButtonSelected(Game &game, int testType, int testIndex,
	const std::string &mifName, const std::optional<ArenaTypes::InteriorType> &optInteriorType,
	ArenaTypes::WeatherType weatherType, MapType mapType)
{
	// Initialize 3D renderer.
	auto &renderer = game.getRenderer();
	const auto &options = game.getOptions();
	const bool fullGameWindow = options.getGraphics_ModernInterface();
	renderer.initializeWorldRendering(options.getGraphics_ResolutionScale(),
		fullGameWindow, options.getGraphics_RenderThreadsMode());

	// Game data instance, to be initialized further by one of the loading methods below.
	// Create a player with random data for testing.
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	auto gameState = std::make_unique<GameState>(Player::makeRandom(
		game.getCharacterClassLibrary(), binaryAssetLibrary.getExeData(), game.getRandom()),
		binaryAssetLibrary);

	const int starCount = SkyUtils::getStarCountFromDensity(options.getMisc_StarDensity());
	const int currentDay = gameState->getDate().getDay();

	// Load the selected level based on world type (writing into active game state).
	if (mapType == MapType::Interior)
	{
		if (testType != MainMenuUiModel::TestType_Dungeon)
		{
			const Player &player = gameState->getPlayer();
			const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();

			// Set some interior location data for testing, depending on whether it's a
			// main quest dungeon.
			int locationIndex, provinceIndex;
			if (testType == MainMenuUiModel::TestType_MainQuest)
			{
				// Fetch from a global function.
				const auto &exeData = game.getBinaryAssetLibrary().getExeData();
				MainMenuUiModel::SpecialCaseType specialCaseType;
				MainMenuUiModel::getMainQuestLocationFromIndex(testIndex, exeData, &locationIndex, &provinceIndex, &specialCaseType);

				if (specialCaseType == MainMenuUiModel::SpecialCaseType::None)
				{
					// Do nothing.
				}
				else if (specialCaseType == MainMenuUiModel::SpecialCaseType::StartDungeon)
				{
					// @temp: hacky search for start dungeon location definition.
					const ProvinceDefinition &tempProvinceDef = worldMapDef.getProvinceDef(provinceIndex);
					for (int i = 0; i < tempProvinceDef.getLocationCount(); i++)
					{
						const LocationDefinition &curLocationDef = tempProvinceDef.getLocationDef(i);
						if (curLocationDef.getType() == LocationDefinition::Type::MainQuestDungeon)
						{
							const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
								curLocationDef.getMainQuestDungeonDefinition();

							if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Start)
							{
								locationIndex = i;
								break;
							}
						}
					}

					DebugAssertMsg(locationIndex != -1, "Couldn't find start dungeon location definition.");
				}
				else
				{
					DebugNotImplementedMsg(std::to_string(static_cast<int>(specialCaseType)));
				}
			}
			else
			{
				// Any province besides center province.
				// @temp: mildly disorganized
				provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
				locationIndex = MainMenuUiModel::getRandomCityLocationIndex(worldMapDef.getProvinceDef(provinceIndex));
			}

			const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);

			DebugAssert(optInteriorType.has_value());
			const ArenaTypes::InteriorType interiorType = *optInteriorType;

			const std::optional<bool> rulerIsMale = [&locationDef]()
			{
				if (locationDef.getType() == LocationDefinition::Type::City)
				{
					const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
					return cityDef.rulerIsMale;
				}
				else
				{
					return false;
				}
			}();

			MapGeneration::InteriorGenInfo interiorGenInfo;
			interiorGenInfo.initPrefab(std::string(mifName), interiorType, rulerIsMale);

			const std::optional<VoxelInt2> playerStartOffset; // Unused for main quest/interiors.

			const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);
			if (!gameState->trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
				game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
				game.getBinaryAssetLibrary(), game.getTextureManager(), renderer))
			{
				DebugCrash("Couldn't load interior \"" + locationDef.getName() + "\".");
			}
		}
		else
		{
			// Pick a random dungeon based on the dungeon type.
			const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();

			const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
			const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

			constexpr bool isArtifactDungeon = false;

			const std::optional<VoxelInt2> playerStartOffset = VoxelInt2(
				ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_X,
				ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_Z);

			if (mifName == MainMenuUiModel::RandomNamedDungeon)
			{
				const std::optional<int> locationIndex = MainMenuUiModel::getRandomDungeonLocationDefIndex(provinceDef);
				DebugAssertMsg(locationIndex.has_value(), "Couldn't find named dungeon in \"" + provinceDef.getName() + "\".");

				const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);
				const LocationDefinition::DungeonDefinition &dungeonDef = locationDef.getDungeonDefinition();

				MapGeneration::InteriorGenInfo interiorGenInfo;
				interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

				const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);
				if (!gameState->trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
					game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
					game.getBinaryAssetLibrary(), game.getTextureManager(), game.getRenderer()))
				{
					DebugCrash("Couldn't load named dungeon \"" + locationDef.getName() + "\".");
				}

				// Set random named dungeon name and visibility for testing.
				LocationInstance &locationInst = gameState->getLocationInstance();
				locationInst.setNameOverride("Test Dungeon");

				if (!locationInst.isVisible())
				{
					locationInst.toggleVisibility();
				}
			}
			else if (mifName == MainMenuUiModel::RandomWildDungeon)
			{
				Random &random = game.getRandom();
				const int wildBlockX = random.next(ArenaWildUtils::WILD_WIDTH);
				const int wildBlockY = random.next(ArenaWildUtils::WILD_HEIGHT);

				const int locationIndex = MainMenuUiModel::getRandomCityLocationIndex(provinceDef);
				const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);
				const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

				const uint32_t dungeonSeed = cityDef.getWildDungeonSeed(wildBlockX, wildBlockY);
				constexpr int widthChunkCount = ArenaWildUtils::WILD_DUNGEON_WIDTH_CHUNKS;
				constexpr int heightChunkCount = ArenaWildUtils::WILD_DUNGEON_HEIGHT_CHUNKS;

				// Need to generate dungeon definition here since we don't have the wild chunk itself.
				LocationDefinition::DungeonDefinition dungeonDef;
				dungeonDef.init(dungeonSeed, widthChunkCount, heightChunkCount);

				MapGeneration::InteriorGenInfo interiorGenInfo;
				interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

				const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);
				if (!gameState->trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
					game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
					binaryAssetLibrary, game.getTextureManager(), renderer))
				{
					DebugCrash("Couldn't load wilderness dungeon \"" + locationDef.getName() + "\".");
				}
			}
			else
			{
				DebugCrash("Unrecognized dungeon type \"" + mifName + "\".");
			}
		}
	}
	else if (mapType == MapType::City)
	{
		// There is only one "premade" city (used by the center province). All others
		// are randomly generated.
		// @todo: the IMPERIAL.MIF and random city/town/village branches could be merged a bit.
		if (mifName == MainMenuUiModel::ImperialMIF)
		{
			// Load city into game state.
			const int provinceIndex = ArenaLocationUtils::CENTER_PROVINCE_ID;
			const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();
			const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
			const std::optional<int> locationIndex = [&mifName, &provinceDef]() -> std::optional<int>
			{
				for (int i = 0; i < provinceDef.getLocationCount(); i++)
				{
					const LocationDefinition &curLocationDef = provinceDef.getLocationDef(i);
					if (curLocationDef.getType() == LocationDefinition::Type::City)
					{
						const LocationDefinition::CityDefinition &cityDef = curLocationDef.getCityDefinition();
						if ((cityDef.type == ArenaTypes::CityType::CityState) && cityDef.premade &&
							cityDef.palaceIsMainQuestDungeon)
						{
							return i;
						}
					}
				}

				return std::nullopt;
			}();

			DebugAssertMsg(locationIndex.has_value(), "Couldn't find premade city with main quest palace dungeon.");
			const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);

			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
			Buffer<uint8_t> reservedBlocks = [&cityDef]()
			{
				const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
				DebugAssert(cityReservedBlocks != nullptr);
				Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
				std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.get());
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
			cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
				cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
				cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
				mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

			const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
			{
				WeatherDefinition weatherDef;
				weatherDef.initFromClassic(weatherType, currentDay, game.getRandom());
				return weatherDef;
			}();

			SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
			skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
				cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

			const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);
			if (!gameState->trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
				game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), binaryAssetLibrary,
				game.getTextAssetLibrary(), game.getTextureManager(), renderer))
			{
				DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
			}
		}
		else
		{
			// Pick a random location based on the .MIF name, excluding the center province.
			const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();
			const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
			const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

			const ArenaTypes::CityType targetCityType = [&mifName]()
			{
				if (mifName == MainMenuUiModel::RandomCity)
				{
					return ArenaTypes::CityType::CityState;
				}
				else if (mifName == MainMenuUiModel::RandomTown)
				{
					return ArenaTypes::CityType::Town;
				}
				else if (mifName == MainMenuUiModel::RandomVillage)
				{
					return ArenaTypes::CityType::Village;
				}
				else
				{
					DebugUnhandledReturnMsg(ArenaTypes::CityType, mifName);
				}
			}();

			const std::optional<int> locationIndex = MainMenuUiModel::getRandomCityLocationDefIndexIfType(provinceDef, targetCityType);
			DebugAssertMsg(locationIndex.has_value(), "Couldn't find city for \"" + mifName + "\".");

			const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);
			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

			Buffer<uint8_t> reservedBlocks = [&cityDef]()
			{
				const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
				DebugAssert(cityReservedBlocks != nullptr);
				Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
				std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.get());
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
			cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
				cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
				cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
				mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

			const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
			{
				WeatherDefinition weatherDef;
				weatherDef.initFromClassic(weatherType, currentDay, game.getRandom());
				return weatherDef;
			}();

			SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
			skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
				cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

			const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);

			// Load city into game state.
			if (!gameState->trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
				game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), binaryAssetLibrary,
				game.getTextAssetLibrary(), game.getTextureManager(), renderer))
			{
				DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
			}
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		// Pick a random location and province.
		const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();
		const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

		const int locationIndex = MainMenuUiModel::getRandomCityLocationIndex(provinceDef);
		const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);
		const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

		const auto &exeData = binaryAssetLibrary.getExeData();
		Buffer2D<ArenaWildUtils::WildBlockID> wildBlockIDs =
			ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, exeData.wild);

		MapGeneration::WildGenInfo wildGenInfo;
		wildGenInfo.init(std::move(wildBlockIDs), cityDef, cityDef.citySeed);

		// Use current weather.
		const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
		{
			WeatherDefinition weatherDef;
			weatherDef.initFromClassic(weatherType, currentDay, game.getRandom());
			return weatherDef;
		}();

		SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
			cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

		// No previous start coordinate available. Let the loader decide.
		const std::optional<CoordInt3> startCoord;

		// Load wilderness into game state.
		const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);
		if (!gameState->trySetWilderness(wildGenInfo, skyGenInfo, overrideWeather, startCoord,
			worldMapLocationIDs, game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
			binaryAssetLibrary, game.getTextureManager(), renderer))
		{
			DebugCrash("Couldn't load wilderness \"" + locationDef.getName() + "\".");
		}
	}
	else
	{
		DebugCrash("Unrecognized world type \"" + std::to_string(static_cast<int>(mapType)) + "\".");
	}

	// Set clock to 5:45am.
	auto &clock = gameState->getClock();
	clock = Clock(5, 45, 0);

	// Get the music that should be active on start.
	const MusicLibrary &musicLibrary = game.getMusicLibrary();
	const MusicDefinition *musicDef = [&game, &mifName, &optInteriorType, mapType, &gameState, &musicLibrary]()
	{
		const bool isExterior = (mapType == MapType::City) || (mapType == MapType::Wilderness);

		// Exteriors depend on the time of day for which music to use. Interiors depend
		// on the current location's .MIF name (if any).
		if (isExterior)
		{
			if (!gameState->nightMusicIsActive())
			{
				const WeatherDefinition &weatherDef = gameState->getWeatherDefinition();
				return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
					game.getRandom(), [weatherDef](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Weather);
					const auto &weatherMusicDef = def.getWeatherMusicDefinition();
					return weatherMusicDef.weatherDef == weatherDef;
				});
			}
			else
			{
				return musicLibrary.getRandomMusicDefinition(
					MusicDefinition::Type::Night, game.getRandom());
			}
		}
		else
		{
			DebugAssert(optInteriorType.has_value());
			const MusicDefinition::InteriorMusicDefinition::Type interiorMusicType =
				MusicUtils::getInteriorMusicType(*optInteriorType);

			return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Interior,
				game.getRandom(), [interiorMusicType](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Interior);
				const auto &interiorMusicDef = def.getInteriorMusicDefinition();
				return interiorMusicDef.type == interiorMusicType;
			});
		}
	}();

	const MusicDefinition *jingleMusicDef = [&game, mapType, &gameState, &musicLibrary]()
		-> const MusicDefinition*
	{
		const LocationDefinition &locationDef = gameState->getLocationDefinition();
		const bool isCity = (mapType == MapType::City) &&
			(locationDef.getType() == LocationDefinition::Type::City);

		if (isCity)
		{
			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
			return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
				game.getRandom(), [&cityDef](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
				const auto &jingleMusicDef = def.getJingleMusicDefinition();
				return (jingleMusicDef.cityType == cityDef.type) &&
					(jingleMusicDef.climateType == cityDef.climateType);
			});
		}
		else
		{
			return nullptr;
		}
	}();

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing start music.");
	}

	// Set the game state before constructing the game world panel.
	game.setGameState(std::move(gameState));

	// Initialize game world panel.
	game.setPanel<GameWorldPanel>();

	AudioManager &audioManager = game.getAudioManager();
	audioManager.setMusic(musicDef, jingleMusicDef);
}

void MainMenuUiController::onTestTypeUpButtonSelected(int *testType, int *testIndex, int *testIndex2, int *testWeather)
{
	*testType = (*testType > 0) ? (*testType - 1) : (MainMenuUiModel::MaxTestTypes - 1);

	// Reset the other indices.
	*testIndex = 0;
	*testIndex2 = 1;
	*testWeather = 0;
}

void MainMenuUiController::onTestTypeDownButtonSelected(int *testType, int *testIndex, int *testIndex2,
	int *testWeather)
{
	*testType = (*testType < (MainMenuUiModel::MaxTestTypes - 1)) ? (*testType + 1) : 0;

	// Reset the other indices.
	*testIndex = 0;
	*testIndex2 = 1;
	*testWeather = 0;
}

void MainMenuUiController::onTestIndexUpButtonSelected(int *testType, int *testIndex, int *testIndex2)
{
	const int count = [testType]()
	{
		// Check test type to determine the max.
		if (*testType == MainMenuUiModel::TestType_MainQuest)
		{
			return MainMenuUiModel::MainQuestLocationCount;
		}
		else if (*testType == MainMenuUiModel::TestType_Interior)
		{
			return static_cast<int>(MainMenuUiModel::InteriorLocations.size());
		}
		else if (*testType == MainMenuUiModel::TestType_City)
		{
			return static_cast<int>(MainMenuUiModel::CityLocations.size());
		}
		else if (*testType == MainMenuUiModel::TestType_Wilderness)
		{
			return static_cast<int>(MainMenuUiModel::WildernessLocations.size());
		}
		else
		{
			return static_cast<int>(MainMenuUiModel::DungeonLocations.size());
		}
	}();

	*testIndex = (*testIndex > 0) ? (*testIndex - 1) : (count - 1);

	if (*testType == MainMenuUiModel::TestType_Interior)
	{
		// Reset the second index.
		*testIndex2 = 1;
	}
}

void MainMenuUiController::onTestIndexDownButtonSelected(int *testType, int *testIndex, int *testIndex2)
{
	const int count = [testType]()
	{
		// Check test type to determine the max.
		if (*testType == MainMenuUiModel::TestType_MainQuest)
		{
			return MainMenuUiModel::MainQuestLocationCount;
		}
		else if (*testType == MainMenuUiModel::TestType_Interior)
		{
			return static_cast<int>(MainMenuUiModel::InteriorLocations.size());
		}
		else if (*testType == MainMenuUiModel::TestType_City)
		{
			return static_cast<int>(MainMenuUiModel::CityLocations.size());
		}
		else if (*testType == MainMenuUiModel::TestType_Wilderness)
		{
			return static_cast<int>(MainMenuUiModel::WildernessLocations.size());
		}
		else
		{
			return static_cast<int>(MainMenuUiModel::DungeonLocations.size());
		}
	}();

	*testIndex = (*testIndex < (count - 1)) ? (*testIndex + 1) : 0;

	if (*testType == MainMenuUiModel::TestType_Interior)
	{
		// Reset the second index.
		*testIndex2 = 1;
	}
}

void MainMenuUiController::onTestIndex2UpButtonSelected(int testType, int testIndex, int *testIndex2)
{
	DebugAssert(testType == MainMenuUiModel::TestType_Interior);

	// Interior range.
	const auto &interior = MainMenuUiModel::InteriorLocations.at(testIndex);
	const int minIndex = std::get<1>(interior).first;
	const int maxIndex = std::get<1>(interior).second;

	*testIndex2 = (*testIndex2 < maxIndex) ? (*testIndex2 + 1) : minIndex;
}

void MainMenuUiController::onTestIndex2DownButtonSelected(int testType, int testIndex, int *testIndex2)
{
	DebugAssert(testType == MainMenuUiModel::TestType_Interior);

	// Interior range.
	const auto &interior = MainMenuUiModel::InteriorLocations.at(testIndex);
	const int minIndex = std::get<1>(interior).first;
	const int maxIndex = std::get<1>(interior).second;

	*testIndex2 = (*testIndex2 > minIndex) ? (*testIndex2 - 1) : maxIndex;
}

void MainMenuUiController::onTestWeatherUpButtonSelected(int testType, int *testWeather)
{
	DebugAssert((testType == MainMenuUiModel::TestType_City) || (testType == MainMenuUiModel::TestType_Wilderness));

	const int count = static_cast<int>(MainMenuUiModel::Weathers.size());
	*testWeather = (*testWeather > 0) ? (*testWeather - 1) : (count - 1);
}

void MainMenuUiController::onTestWeatherDownButtonSelected(int testType, int *testWeather)
{
	DebugAssert((testType == MainMenuUiModel::TestType_City) || (testType == MainMenuUiModel::TestType_Wilderness));

	const int count = static_cast<int>(MainMenuUiModel::Weathers.size());
	*testWeather = (*testWeather < (count - 1)) ? (*testWeather + 1) : 0;
}
