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
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Player/ArenaPlayerUtils.h"
#include "../Sky/SkyUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../World/CardinalDirection.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"

namespace
{
	int GetRandomWeaponIdForClass(const CharacterClassDefinition &charClassDef, Random &random)
	{
		const int allowedWeaponCount = charClassDef.getAllowedWeaponCount();
		Buffer<int> weapons(allowedWeaponCount + 1);
		for (int i = 0; i < allowedWeaponCount; i++)
		{
			weapons.set(i, charClassDef.getAllowedWeapon(i));
		}

		weapons.set(allowedWeaponCount, ArenaItemUtils::FistsWeaponID);

		const int randIndex = random.next(weapons.getCount());
		return weapons.get(randIndex);
	}
}

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

		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
			MusicType::CharacterCreation, game.random);

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing character creation music.");
		}

		AudioManager &audioManager = game.audioManager;
		audioManager.setMusic(musicDef);
	};

	auto changeToNewGameStory = [changeToCharCreation](Game &game)
	{
		std::string paletteNames[] =
		{
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG",
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG",
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG"
		};

		std::string textureNames[] =
		{
			"INTRO01.IMG", "INTRO02.IMG", "INTRO03.IMG",
			"INTRO04.IMG", "INTRO05.IMG", "INTRO06.IMG",
			"INTRO07.IMG", "INTRO08.IMG", "INTRO09.IMG"
		};

		double imageDurations[] =
		{
			5.0, 5.0, 5.0,
			5.0, 5.0, 5.0,
			5.0, 5.0, 5.0
		};

		game.setPanel<ImageSequencePanel>(paletteNames, textureNames, imageDurations, changeToCharCreation);
	};

	const std::string &paletteFilename = ArenaTextureSequenceName::OpeningScroll;
	const std::string &sequenceFilename = ArenaTextureSequenceName::OpeningScroll;

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(sequenceFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for opening scroll animation \"" + sequenceFilename + "\".");
		return;
	}

	const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);
	const double secondsPerFrame = metadata.getSecondsPerFrame();
	game.setPanel<CinematicPanel>(paletteFilename, sequenceFilename, secondsPerFrame, changeToNewGameStory);

	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
		MusicType::Cinematic, game.random, [](const MusicDefinition &def)
	{
		DebugAssert(def.type == MusicType::Cinematic);
		const CinematicMusicDefinition &cinematicMusicDef = def.cinematic;
		return cinematicMusicDef.type == CinematicMusicType::Intro;
	});

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing intro music.");
	}

	AudioManager &audioManager = game.audioManager;
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
	// Game data instance, to be initialized further by one of the loading methods below.
	// Create a player with random data for testing.
	const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const auto &exeData = binaryAssetLibrary.getExeData();
	const auto &charClassLibrary = CharacterClassLibrary::getInstance();

	GameState &gameState = game.gameState;
	gameState.init(game.arenaRandom);

	Random &random = game.random;
	const std::string testPlayerName = "Player";
	const bool testIsMale = random.nextBool();
	const int testRaceID = random.next(8);
	const int testCharClassDefID = random.next(charClassLibrary.getDefinitionCount());
	const int testPortraitID = random.next(10);
	PrimaryAttributes testPrimaryAttributes;
	testPrimaryAttributes.init(testRaceID, testIsMale, exeData);
	const int testMaxHealth = ArenaPlayerUtils::calculateMaxHealthPoints(testCharClassDefID, random);
	const int testMaxStamina = ArenaPlayerUtils::calculateMaxStamina(testPrimaryAttributes.strength.maxValue, testPrimaryAttributes.endurance.maxValue);
	const int testMaxSpellPoints = ArenaPlayerUtils::calculateMaxSpellPoints(testCharClassDefID, testPrimaryAttributes.intelligence.maxValue);
	const int testGold = ArenaPlayerUtils::calculateStartingGold(random);
	const int testWeaponID = GetRandomWeaponIdForClass(charClassLibrary.getDefinition(testCharClassDefID), random);

	Player &player = game.player;
	player.init(testPlayerName, testIsMale, testRaceID, testCharClassDefID, testPortraitID, testPrimaryAttributes,
		testMaxHealth, testMaxStamina, testMaxSpellPoints, testGold, testWeaponID, exeData, game.physicsSystem);

	// Face west so we don't start looking at a wall.
	player.setCameraFrameFromAngles(CardinalDirection::DegreesWest, 0.0);

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const auto &options = game.options;
	const int starCount = SkyUtils::getStarCountFromDensity(options.getMisc_StarDensity());
	const int currentDay = gameState.getDate().getDay();

	// Load the selected level based on world type (writing into active game state).
	if (mapType == MapType::Interior)
	{
		if (testType != MainMenuUiModel::TestType_Dungeon)
		{
			const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();

			// Set some interior location data for testing, depending on whether it's a
			// main quest dungeon.
			int locationIndex, provinceIndex;
			if (testType == MainMenuUiModel::TestType_MainQuest)
			{
				// Fetch from a global function.
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
						if (curLocationDef.getType() == LocationDefinitionType::MainQuestDungeon)
						{
							const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = curLocationDef.getMainQuestDungeonDefinition();

							if (mainQuestDungeonDef.type == LocationMainQuestDungeonDefinitionType::Start)
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
				provinceIndex = random.next(worldMapDef.getProvinceCount() - 1);
				locationIndex = MainMenuUiModel::getRandomCityLocationIndex(worldMapDef.getProvinceDef(provinceIndex));
			}

			const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
			const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);

			DebugAssert(optInteriorType.has_value());
			const ArenaTypes::InteriorType interiorType = *optInteriorType;

			const std::optional<bool> rulerIsMale = [&locationDef]()
			{
				if (locationDef.getType() == LocationDefinitionType::City)
				{
					const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
					return cityDef.rulerIsMale;
				}
				else
				{
					return false;
				}
			}();

			MapGeneration::InteriorGenInfo interiorGenInfo;
			interiorGenInfo.initPrefab(mifName, interiorType, rulerIsMale);

			const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);

			MapDefinition mapDefinition;
			if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
			{
				DebugLogError("Couldn't init MapDefinition for interior \"" + locationDef.getName() + "\".");
				return;
			}

			gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true);
		}
		else
		{
			// Pick a random dungeon based on the dungeon type.
			const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();

			const int provinceIndex = random.next(worldMapDef.getProvinceCount() - 1);
			const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

			constexpr bool isArtifactDungeon = false;

			const VoxelInt2 playerStartOffset(
				ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_X,
				ArenaLevelUtils::RANDOM_DUNGEON_PLAYER_START_OFFSET_Z);

			if (mifName == MainMenuUiModel::RandomNamedDungeon)
			{
				const std::optional<int> locationIndex = MainMenuUiModel::getRandomDungeonLocationDefIndex(provinceDef);
				DebugAssertMsg(locationIndex.has_value(), "Couldn't find named dungeon in \"" + provinceDef.getName() + "\".");

				const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);
				const LocationDungeonDefinition &dungeonDef = locationDef.getDungeonDefinition();

				MapGeneration::InteriorGenInfo interiorGenInfo;
				interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

				const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);

				MapDefinition mapDefinition;
				if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
				{
					DebugLogError("Couldn't load named dungeon \"" + locationDef.getName() + "\".");
					return;
				}

				gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, playerStartOffset, worldMapLocationIDs, true);

				// Set random named dungeon name and visibility for testing.
				WorldMapInstance &worldMapInst = gameState.getWorldMapInstance();
				ProvinceInstance &provinceInst = worldMapInst.getProvinceInstance(worldMapLocationIDs.provinceID);
				LocationInstance &locationInst = provinceInst.getLocationInstance(worldMapLocationIDs.locationID);
				locationInst.setNameOverride("Test Dungeon");

				if (!locationInst.isVisible())
				{
					locationInst.toggleVisibility();
				}
			}
			else if (mifName == MainMenuUiModel::RandomWildDungeon)
			{
				const int wildBlockX = random.next(ArenaWildUtils::WILD_WIDTH);
				const int wildBlockY = random.next(ArenaWildUtils::WILD_HEIGHT);

				const int locationIndex = MainMenuUiModel::getRandomCityLocationIndex(provinceDef);
				const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);
				const LocationCityDefinition &cityDef = locationDef.getCityDefinition();

				const uint32_t dungeonSeed = cityDef.getWildDungeonSeed(wildBlockX, wildBlockY);
				constexpr int widthChunkCount = ArenaWildUtils::WILD_DUNGEON_WIDTH_CHUNKS;
				constexpr int heightChunkCount = ArenaWildUtils::WILD_DUNGEON_HEIGHT_CHUNKS;

				// Need to generate dungeon definition here since we don't have the wild chunk itself.
				LocationDungeonDefinition dungeonDef;
				dungeonDef.init(dungeonSeed, widthChunkCount, heightChunkCount);

				MapGeneration::InteriorGenInfo interiorGenInfo;
				interiorGenInfo.initDungeon(dungeonDef, isArtifactDungeon);

				const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);

				MapDefinition mapDefinition;
				if (!mapDefinition.initInterior(interiorGenInfo, textureManager))
				{
					DebugLogError("Couldn't init MapDefinition for wilderness dungeon \"" + locationDef.getName() + "\".");
					return;
				}
				
				gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, playerStartOffset, worldMapLocationIDs, true);
			}
			else
			{
				DebugCrash("Unrecognized dungeon type \"" + mifName + "\".");
			}
		}
	}
	else if (mapType == MapType::City)
	{
		TextAssetLibrary &textAssetLibrary = TextAssetLibrary::getInstance();

		// There is only one "premade" city (used by the center province). All others
		// are randomly generated.
		// @todo: the IMPERIAL.MIF and random city/town/village branches could be merged a bit.
		if (mifName == MainMenuUiModel::ImperialMIF)
		{
			// Load city into game state.
			const int provinceIndex = ArenaLocationUtils::CENTER_PROVINCE_ID;
			const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
			const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
			const std::optional<int> locationIndex = [&mifName, &provinceDef]() -> std::optional<int>
			{
				for (int i = 0; i < provinceDef.getLocationCount(); i++)
				{
					const LocationDefinition &curLocationDef = provinceDef.getLocationDef(i);
					if (curLocationDef.getType() == LocationDefinitionType::City)
					{
						const LocationCityDefinition &cityDef = curLocationDef.getCityDefinition();
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

			const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
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
			cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
				cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
				cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
				mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

			const WeatherDefinition overrideWeather = [&game, weatherType, &cityDef, currentDay]()
			{
				const ArenaTypes::WeatherType filteredWeatherType = ArenaWeatherUtils::getFilteredWeatherType(weatherType, cityDef.climateType);

				WeatherDefinition weatherDef;
				weatherDef.initFromClassic(filteredWeatherType, currentDay, game.random);
				return weatherDef;
			}();

			SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
			skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
				cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

			const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);

			MapDefinition mapDefinition;
			if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, game.textureManager))
			{
				DebugLogError("Couldn't init MapDefinition for city \"" + locationDef.getName() + "\".");
				return;
			}

			gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);
		}
		else
		{
			// Pick a random location based on the .MIF name, excluding the center province.
			const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
			const int provinceIndex = random.next(worldMapDef.getProvinceCount() - 1);
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
			const LocationCityDefinition &cityDef = locationDef.getCityDefinition();

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
			cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
				cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
				cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
				mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

			const WeatherDefinition overrideWeather = [&game, weatherType, &cityDef, currentDay]()
			{
				const ArenaTypes::WeatherType filteredWeatherType = ArenaWeatherUtils::getFilteredWeatherType(weatherType, cityDef.climateType);

				WeatherDefinition weatherDef;
				weatherDef.initFromClassic(filteredWeatherType, currentDay, game.random);
				return weatherDef;
			}();

			SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
			skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
				cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

			const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);

			MapDefinition mapDefinition;
			if (!mapDefinition.initCity(cityGenInfo, skyGenInfo, textureManager))
			{
				DebugLogError("Couldn't init MapDefinition for city \"" + locationDef.getName() + "\".");
				return;
			}

			gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);
		}
	}
	else if (mapType == MapType::Wilderness)
	{
		// Pick a random location and province.
		const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
		const int provinceIndex = random.next(worldMapDef.getProvinceCount() - 1);
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);

		const int locationIndex = MainMenuUiModel::getRandomCityLocationIndex(provinceDef);
		const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);
		const LocationCityDefinition &cityDef = locationDef.getCityDefinition();

		const auto &exeData = binaryAssetLibrary.getExeData();
		Buffer2D<ArenaWildUtils::WildBlockID> wildBlockIDs =
			ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, exeData.wild);

		MapGeneration::WildGenInfo wildGenInfo;
		wildGenInfo.init(std::move(wildBlockIDs), cityDef, cityDef.citySeed);

		const WeatherDefinition overrideWeather = [&game, weatherType, &cityDef, currentDay]()
		{
			const ArenaTypes::WeatherType filteredWeatherType = ArenaWeatherUtils::getFilteredWeatherType(weatherType, cityDef.climateType);

			WeatherDefinition weatherDef;
			weatherDef.initFromClassic(filteredWeatherType, currentDay, game.random);
			return weatherDef;
		}();

		SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
			cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

		const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, locationIndex);

		MapDefinition mapDefinition;
		if (!mapDefinition.initWild(wildGenInfo, skyGenInfo, textureManager))
		{
			DebugLogError("Couldn't init MapDefinition for wilderness \"" + locationDef.getName() + "\".");
			return;
		}

		// Don't have a city gate reference. Just pick somewhere in the center of the wilderness.
		const CoordInt2 startCoord(
			ChunkInt2(ArenaWildUtils::WILD_WIDTH / 2, ArenaWildUtils::WILD_HEIGHT / 2),
			VoxelInt2::Zero);

		gameState.queueMapDefChange(std::move(mapDefinition), startCoord, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true, overrideWeather);
	}
	else
	{
		DebugCrash("Unrecognized world type \"" + std::to_string(static_cast<int>(mapType)) + "\".");
	}

	// Set to 5:45am for testing.
	auto &clock = gameState.getClock();
	clock.init(5, 45, 0);

	GameState::SceneChangeMusicFunc musicFunc = [](Game &game)
	{
		// Get the music that should be active on start.
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		GameState &gameState = game.gameState;
		const MapType mapType = gameState.getActiveMapType();
		const bool isExterior = (mapType == MapType::City) || (mapType == MapType::Wilderness);

		// Exteriors depend on the time of day for which music to use. Interiors depend
		// on the current location's .MIF name (if any).
		const MusicDefinition *musicDef = nullptr;
		if (isExterior)
		{
			musicDef = MusicUtils::getExteriorMusicDefinition(gameState.getWeatherDefinition(), gameState.getClock(), game.random);
		}
		else
		{
			const MapSubDefinition &mapSubDef = gameState.getActiveMapDef().getSubDefinition();
			const ArenaTypes::InteriorType interiorType = mapSubDef.interior.interiorType;
			const InteriorMusicType interiorMusicType = MusicUtils::getInteriorMusicType(interiorType);

			musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicType::Interior,
				game.random, [interiorMusicType](const MusicDefinition &def)
			{
				DebugAssert(def.type == MusicType::Interior);
				const InteriorMusicDefinition &interiorMusicDef = def.interior;
				return interiorMusicDef.type == interiorMusicType;
			});
		}

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing start music.");
		}

		return musicDef;
	};

	GameState::SceneChangeMusicFunc jingleMusicFunc = [](Game &game)
	{
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		GameState &gameState = game.gameState;
		const MapType mapType = gameState.getActiveMapType();
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		const bool isCity = (mapType == MapType::City) && (locationDef.getType() == LocationDefinitionType::City);

		const MusicDefinition *musicDef = nullptr;
		if (isCity)
		{
			const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
			musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicType::Jingle,
				game.random, [&cityDef](const MusicDefinition &def)
			{
				DebugAssert(def.type == MusicType::Jingle);
				const JingleMusicDefinition &jingleMusicDef = def.jingle;
				return (jingleMusicDef.cityType == cityDef.type) && (jingleMusicDef.climateType == cityDef.climateType);
			});
		}

		return musicDef;
	};

	gameState.queueMusicOnSceneChange(musicFunc, jingleMusicFunc);

	// Initialize game world panel.
	game.setPanel<GameWorldPanel>();
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
