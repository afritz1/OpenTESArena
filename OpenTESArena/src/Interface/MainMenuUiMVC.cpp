#include <numeric>
#include <vector>

#include "SDL_events.h"

#include "ChooseClassCreationUiState.h"
#include "CinematicUiState.h"
#include "GameWorldUiState.h"
#include "ImageSequenceUiState.h"
#include "LoadSaveUiState.h"
#include "MainMenuUiMVC.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Math/RandomUtils.h"
#include "../Player/ArenaPlayerUtils.h"
#include "../Rendering/Renderer.h"
#include "../Sky/SkyUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../Weather/ArenaWeatherUtils.h"
#include "../World/CardinalDirection.h"
#include "../World/MapType.h"
#include "../WorldMap/ArenaLocationUtils.h"
#include "../WorldMap/ProvinceDefinition.h"

#include "components/debug/Debug.h"
#include "components/utilities/Span2D.h"
#include "components/utilities/String.h"

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

std::string MainMenuUiModel::getTestButtonText()
{
	return "Test";
}

std::string MainMenuUiModel::getTestTypeName(int type)
{
	if (type == TestType_MainQuest)
	{
		return "Main Quest";
	}
	else if (type == TestType_Interior)
	{
		return "Interior";
	}
	else if (type == TestType_City)
	{
		return "City";
	}
	else if (type == TestType_Wilderness)
	{
		return "Wilderness";
	}
	else if (type == TestType_Dungeon)
	{
		return "Dungeon";
	}
	else
	{
		DebugUnhandledReturnMsg(std::string, std::to_string(type));
	}
}

std::string MainMenuUiModel::getSelectedTestName(Game &game, int testType, int testIndex, int testIndex2)
{
	if (testType == MainMenuUiModel::TestType_MainQuest)
	{
		const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const auto &exeData = binaryAssetLibrary.getExeData();

		// Decide how to get the main quest dungeon name.
		if (testIndex == 0)
		{
			// Start dungeon.
			return String::toUppercase(exeData.locations.startDungeonMifName);
		}
		else if (testIndex == (MainMenuUiModel::MainQuestLocationCount - 1))
		{
			// Final dungeon.
			return String::toUppercase(exeData.locations.finalDungeonMifName);
		}
		else
		{
			// Generate the location from the executable data, fetching data from a
			// global function.
			int locationID, provinceID;
			MainMenuUiModel::SpecialCaseType specialCaseType;
			MainMenuUiModel::getMainQuestLocationFromIndex(testIndex, exeData, &locationID, &provinceID, &specialCaseType);
			DebugAssert(specialCaseType == MainMenuUiModel::SpecialCaseType::None);

			// Calculate the .MIF name from the dungeon seed.
			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const uint32_t dungeonSeed = [&cityData, locationID, provinceID]()
			{
				const auto &province = cityData.getProvinceData(provinceID);
				const int localDungeonID = locationID - 32;
				return ArenaLocationUtils::getDungeonSeed(localDungeonID, provinceID, province);
			}();

			const std::string mifName = ArenaLocationUtils::getMainQuestDungeonMifName(dungeonSeed);
			return String::toUppercase(mifName);
		}
	}
	else if (testType == MainMenuUiModel::TestType_Interior)
	{
		const auto &interior = MainMenuUiModel::InteriorLocations.at(testIndex);
		return std::get<0>(interior) + std::to_string(testIndex2) + ".MIF";
	}
	else if (testType == MainMenuUiModel::TestType_City)
	{
		return MainMenuUiModel::CityLocations.at(testIndex);
	}
	else if (testType == MainMenuUiModel::TestType_Wilderness)
	{
		return MainMenuUiModel::WildernessLocations.at(testIndex);
	}
	else if (testType == MainMenuUiModel::TestType_Dungeon)
	{
		return MainMenuUiModel::DungeonLocations.at(testIndex);
	}
	else
	{
		DebugUnhandledReturnMsg(std::string, std::to_string(testType));
	}
}

std::optional<ArenaInteriorType> MainMenuUiModel::getSelectedTestInteriorType(int testType, int testIndex)
{
	if ((testType == MainMenuUiModel::TestType_MainQuest) || (testType == MainMenuUiModel::TestType_Dungeon))
	{
		return ArenaInteriorType::Dungeon;
	}
	else if (testType == MainMenuUiModel::TestType_Interior)
	{
		DebugAssertIndex(MainMenuUiModel::InteriorLocations, testIndex);
		const auto &interior = MainMenuUiModel::InteriorLocations[testIndex];
		return std::get<2>(interior);
	}
	else if ((testType == MainMenuUiModel::TestType_City) || (testType == MainMenuUiModel::TestType_Wilderness))
	{
		return std::nullopt;
	}
	else
	{
		DebugUnhandledReturnMsg(std::optional<ArenaInteriorType>, std::to_string(testType));
	}
}

ArenaWeatherType MainMenuUiModel::getSelectedTestWeatherType(int testWeather)
{
	DebugAssertIndex(MainMenuUiModel::Weathers, testWeather);
	return MainMenuUiModel::Weathers[testWeather];
}

MapType MainMenuUiModel::getSelectedTestMapType(int testType)
{
	if ((testType == MainMenuUiModel::TestType_MainQuest) ||
		(testType == MainMenuUiModel::TestType_Interior) ||
		(testType == MainMenuUiModel::TestType_Dungeon))
	{
		return MapType::Interior;
	}
	else if (testType == MainMenuUiModel::TestType_City)
	{
		return MapType::City;
	}
	else if (testType == MainMenuUiModel::TestType_Wilderness)
	{
		return MapType::Wilderness;
	}
	else
	{
		DebugUnhandledReturnMsg(MapType, std::to_string(testType));
	}
}

void MainMenuUiModel::getMainQuestLocationFromIndex(int testIndex, const ExeData &exeData, int *outLocationID,
	int *outProvinceID, SpecialCaseType *outSpecialCaseType)
{
	if (testIndex == 0)
	{
		*outLocationID = -1;
		*outProvinceID = ArenaLocationUtils::CENTER_PROVINCE_ID;
		*outSpecialCaseType = SpecialCaseType::StartDungeon;
	}
	else if (testIndex == (MainQuestLocationCount - 1))
	{
		*outLocationID = 0;
		*outProvinceID = ArenaLocationUtils::CENTER_PROVINCE_ID;
		*outSpecialCaseType = SpecialCaseType::None;
	}
	else
	{
		// Generate the location from the executable data.
		const auto &staffProvinces = exeData.locations.staffProvinces;
		const int staffProvincesIndex = (testIndex - 1) / 2;
		DebugAssertIndex(staffProvinces, staffProvincesIndex);
		*outProvinceID = staffProvinces[staffProvincesIndex];
		*outLocationID = ArenaLocationUtils::dungeonToLocationID(testIndex % 2);
		*outSpecialCaseType = SpecialCaseType::None;
	}
}

std::vector<int> MainMenuUiModel::makeShuffledLocationIndices(const ProvinceDefinition &provinceDef)
{
	std::vector<int> indices(provinceDef.getLocationCount());
	std::iota(indices.begin(), indices.end(), 0);
	RandomUtils::shuffle<int>(indices);
	return indices;
}

std::optional<int> MainMenuUiModel::getRandomCityLocationDefIndexIfType(const ProvinceDefinition &provinceDef, ArenaCityType cityType)
{
	// Iterate over locations in the province in a random order.
	const std::vector<int> randomLocationIndices = MainMenuUiModel::makeShuffledLocationIndices(provinceDef);

	for (const int locationIndex : randomLocationIndices)
	{
		const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
		if (curLocationDef.getType() == LocationDefinitionType::City)
		{
			const auto &curCityDef = curLocationDef.getCityDefinition();
			if (curCityDef.type == cityType)
			{
				return locationIndex;
			}
		}
	}

	return std::nullopt;
}

int MainMenuUiModel::getRandomCityLocationIndex(const ProvinceDefinition &provinceDef)
{
	// Iterate over locations in the province in a random order.
	const std::vector<int> randomLocationIndices = MainMenuUiModel::makeShuffledLocationIndices(provinceDef);

	for (const int locationIndex : randomLocationIndices)
	{
		const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
		if (curLocationDef.getType() == LocationDefinitionType::City)
		{
			return locationIndex;
		}
	}

	return -1;
}

std::optional<int> MainMenuUiModel::getRandomDungeonLocationDefIndex(const ProvinceDefinition &provinceDef)
{
	// Iterate over locations in the province in a random order.
	const std::vector<int> randomLocationIndices = MainMenuUiModel::makeShuffledLocationIndices(provinceDef);

	for (const int locationIndex : randomLocationIndices)
	{
		const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
		if (curLocationDef.getType() == LocationDefinitionType::Dungeon)
		{
			return locationIndex;
		}
	}

	return std::nullopt;
}

Rect MainMenuUiView::getLoadButtonRect()
{
	return Rect(Int2(168, 58), 150, 20);
}

Rect MainMenuUiView::getNewGameButtonRect()
{
	return Rect(Int2(168, 112), 150, 20);
}

Rect MainMenuUiView::getExitButtonRect()
{
	return Rect(Int2(168, 158), 45, 20);
}

Rect MainMenuUiView::getTestButtonRect()
{
	return Rect(135, ArenaRenderUtils::SCREEN_HEIGHT - 17, 30, 14);
}

Color MainMenuUiView::getTestButtonTextColor()
{
	return Colors::White;
}

Rect MainMenuUiView::getTestTypeUpButtonRect()
{
	return Rect(312, 164, 8, 8);
}

Rect MainMenuUiView::getTestTypeDownButtonRect()
{
	return Rect(312, 172, 8, 8);
}

Rect MainMenuUiView::getTestIndexUpButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestTypeUpButtonRect();
	return Rect(
		baseRect.getLeft() - baseRect.width - 2,
		baseRect.getTop() + (baseRect.height * 2) + 2,
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestIndexDownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndexUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestIndex2UpButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndexUpButtonRect();
	return Rect(
		baseRect.getLeft() + 10,
		baseRect.getTop(),
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestIndex2DownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestIndex2UpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestWeatherUpButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestTypeUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getTop() - 2 - (2 * baseRect.height),
		baseRect.width,
		baseRect.height);
}

Rect MainMenuUiView::getTestWeatherDownButtonRect()
{
	const Rect baseRect = MainMenuUiView::getTestWeatherUpButtonRect();
	return Rect(
		baseRect.getLeft(),
		baseRect.getBottom(),
		baseRect.width,
		baseRect.height);
}

TextureAsset MainMenuUiView::getBackgroundTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::MainMenu));
}

TextureAsset MainMenuUiView::getPaletteTextureAsset()
{
	return MainMenuUiView::getBackgroundTextureAsset();
}

TextureAsset MainMenuUiView::getTestArrowsTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::UpDown));
}

TextureAsset MainMenuUiView::getTestArrowsPaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaPaletteName::CharSheet));
}

UiTextureID MainMenuUiView::allocBackgroundTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = MainMenuUiView::getBackgroundTextureAsset();
	const TextureAsset paletteTextureAsset = MainMenuUiView::getPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for main menu background \"" + textureAsset.filename + "\".");
	}

	return textureID;
}

UiTextureID MainMenuUiView::allocTestArrowsTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset textureAsset = MainMenuUiView::getTestArrowsTextureAsset();
	const TextureAsset paletteTextureAsset = MainMenuUiView::getTestArrowsPaletteTextureAsset();

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for main menu test arrows \"" + textureAsset.filename + "\".");
	}

	return textureID;
}

UiTextureID MainMenuUiView::allocTestButtonTexture(TextureManager &textureManager, Renderer &renderer)
{
	const Rect rect = MainMenuUiView::getTestButtonRect();
	const Surface surface = TextureUtils::generate(MainMenuUiView::TestButtonPatternType, rect.width, rect.height, textureManager, renderer);

	Span2D<const uint32_t> pixels = surface.getPixels();
	const UiTextureID textureID = renderer.createUiTexture(pixels.getWidth(), pixels.getHeight());
	if (textureID < 0)
	{
		DebugLogError("Couldn't create UI texture for test button.");
		return -1;
	}

	if (!renderer.populateUiTextureNoPalette(textureID, pixels))
	{
		DebugLogError("Couldn't populate UI texture for test button.");
	}

	return textureID;
}

void MainMenuUiController::onLoadGameButtonSelected(Game &game)
{
	LoadSaveUI::state.type = LoadSaveType::Load;
	game.setNextContext(LoadSaveUI::ContextName);
}

void MainMenuUiController::onNewGameButtonSelected(Game &game)
{
	// Link together the opening scroll, intro cinematic, and character creation.
	auto changeToCharCreation = [&game]()
	{
		game.setCharacterCreationState(std::make_unique<CharacterCreationState>());
		game.setNextContext(ChooseClassCreationUI::ContextName);

		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(MusicType::CharacterCreation, game.random);
		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing character creation music.");
		}

		AudioManager &audioManager = game.audioManager;
		audioManager.setMusic(musicDef);
	};

	auto changeToNewGameStory = [&game, changeToCharCreation]()
	{
		const std::string paletteNames[] =
		{
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG",
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG",
			"SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG"
		};

		const std::string textureNames[] =
		{
			"INTRO01.IMG", "INTRO02.IMG", "INTRO03.IMG",
			"INTRO04.IMG", "INTRO05.IMG", "INTRO06.IMG",
			"INTRO07.IMG", "INTRO08.IMG", "INTRO09.IMG"
		};

		constexpr double imageDurations[] =
		{
			5.0, 5.0, 5.0,
			5.0, 5.0, 5.0,
			5.0, 5.0, 5.0
		};

		ImageSequenceUiInitInfo &imageSequenceInitInfo = ImageSequenceUI::state.initInfo;
		imageSequenceInitInfo.init(paletteNames, textureNames, imageDurations, changeToCharCreation);
		game.setNextContext(ImageSequenceUI::ContextName);
	};

	const std::string &paletteFilename = ArenaTextureSequenceName::OpeningScroll;
	const std::string &sequenceFilename = ArenaTextureSequenceName::OpeningScroll;

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(sequenceFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogErrorFormat("Couldn't get texture file metadata for opening scroll animation \"%s\".", sequenceFilename.c_str());
		return;
	}

	const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);
	const double secondsPerFrame = metadata.getSecondsPerFrame();

	CinematicUiInitInfo &cinematicInitInfo = CinematicUI::state.initInfo;
	cinematicInitInfo.init(paletteFilename, sequenceFilename, secondsPerFrame, changeToNewGameStory);
	game.setNextContext(CinematicUI::ContextName);

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
	const std::string &mifName, const std::optional<ArenaInteriorType> &optInteriorType,
	ArenaWeatherType weatherType, MapType mapType)
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
		testMaxHealth, testMaxStamina, testMaxSpellPoints, testGold, testWeaponID, game.options.getMisc_GhostMode(),
		exeData, game.physicsSystem);

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
			const ArenaInteriorType interiorType = *optInteriorType;

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

			const std::string interiorDisplayName = "Test Interior";

			MapGenerationInteriorInfo interiorGenInfo;
			interiorGenInfo.initPrefab(mifName, interiorType, rulerIsMale, interiorDisplayName);

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

				MapGenerationInteriorInfo interiorGenInfo;
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

				MapGenerationInteriorInfo interiorGenInfo;
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
						if ((cityDef.type == ArenaCityType::CityState) && cityDef.premade &&
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
			cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
				cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
				cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
				mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

			const WeatherDefinition overrideWeather = [&game, weatherType, &cityDef, currentDay]()
			{
				const ArenaWeatherType filteredWeatherType = ArenaWeatherUtils::getFilteredWeatherType(weatherType, cityDef.climateType);

				WeatherDefinition weatherDef;
				weatherDef.initFromClassic(filteredWeatherType, currentDay, game.random);
				return weatherDef;
			}();

			SkyGenerationExteriorInfo skyGenInfo;
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

			const ArenaCityType targetCityType = [&mifName]()
			{
				if (mifName == MainMenuUiModel::RandomCity)
				{
					return ArenaCityType::CityState;
				}
				else if (mifName == MainMenuUiModel::RandomTown)
				{
					return ArenaCityType::Town;
				}
				else if (mifName == MainMenuUiModel::RandomVillage)
				{
					return ArenaCityType::Village;
				}
				else
				{
					DebugUnhandledReturnMsg(ArenaCityType, mifName);
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
			cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
				cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
				cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
				mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

			const WeatherDefinition overrideWeather = [&game, weatherType, &cityDef, currentDay]()
			{
				const ArenaWeatherType filteredWeatherType = ArenaWeatherUtils::getFilteredWeatherType(weatherType, cityDef.climateType);

				WeatherDefinition weatherDef;
				weatherDef.initFromClassic(filteredWeatherType, currentDay, game.random);
				return weatherDef;
			}();

			SkyGenerationExteriorInfo skyGenInfo;
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
		Buffer2D<ArenaWildBlockID> wildBlockIDs = ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, exeData.wild);

		MapGenerationWildInfo wildGenInfo;
		wildGenInfo.init(std::move(wildBlockIDs), cityDef, cityDef.citySeed);

		const WeatherDefinition overrideWeather = [&game, weatherType, &cityDef, currentDay]()
		{
			const ArenaWeatherType filteredWeatherType = ArenaWeatherUtils::getFilteredWeatherType(weatherType, cityDef.climateType);

			WeatherDefinition weatherDef;
			weatherDef.initFromClassic(filteredWeatherType, currentDay, game.random);
			return weatherDef;
		}();

		SkyGenerationExteriorInfo skyGenInfo;
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
			const ArenaInteriorType interiorType = mapSubDef.interior.interiorType;
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

	// Initialize game world UI.
	game.setNextContext(GameWorldUI::ContextName);
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
