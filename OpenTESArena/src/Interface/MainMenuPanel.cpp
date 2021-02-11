#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "CinematicPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "ImageSequencePanel.h"
#include "LoadSavePanel.h"
#include "MainMenuPanel.h"
#include "Surface.h"
#include "Texture.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/RMDFile.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Interface/RichTextString.h"
#include "../Interface/TextAlignment.h"
#include "../Interface/TextBox.h"
#include "../Math/Random.h"
#include "../Math/RandomUtils.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/MusicUtils.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../World/LocationType.h"
#include "../World/LocationUtils.h"
#include "../World/MapType.h"
#include "../World/SkyUtils.h"
#include "../World/WeatherType.h"
#include "../World/WeatherUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	const int MaxTestTypes = 5;
	const int TestType_MainQuest = 0;
	const int TestType_Interior = 1;
	const int TestType_City = 2;
	const int TestType_Wilderness = 3;
	const int TestType_Dungeon = 4;

	const Rect TestButtonRect(135, ArenaRenderUtils::SCREEN_HEIGHT - 17, 30, 14);

	// Main quest locations. There are eight map dungeons and eight staff dungeons.
	// The special cases are the start dungeon and the final dungeon.
	const int MainQuestLocationCount = 18;

	// Small hack for main menu testing.
	enum class SpecialCaseType { None, StartDungeon };

	void GetMainQuestLocationFromIndex(int testIndex, const ExeData &exeData,
		int *outLocationID, int *outProvinceID, SpecialCaseType *outSpecialCaseType)
	{
		if (testIndex == 0)
		{
			*outLocationID = -1;
			*outProvinceID = LocationUtils::CENTER_PROVINCE_ID;
			*outSpecialCaseType = SpecialCaseType::StartDungeon;
		}
		else if (testIndex == (MainQuestLocationCount - 1))
		{
			*outLocationID = 0;
			*outProvinceID = LocationUtils::CENTER_PROVINCE_ID;
			*outSpecialCaseType = SpecialCaseType::None;
		}
		else
		{
			// Generate the location from the executable data.
			const auto &staffProvinces = exeData.locations.staffProvinces;
			const int staffProvincesIndex = (testIndex - 1) / 2;
			DebugAssertIndex(staffProvinces, staffProvincesIndex);
			*outProvinceID = staffProvinces[staffProvincesIndex];
			*outLocationID = LocationUtils::dungeonToLocationID(testIndex % 2);
			*outSpecialCaseType = SpecialCaseType::None;
		}
	}

	std::vector<int> MakeShuffledLocationIndices(const ProvinceDefinition &provinceDef)
	{
		std::vector<int> indices(provinceDef.getLocationCount());
		std::iota(indices.begin(), indices.end(), 0);
		RandomUtils::shuffle(indices.data(), static_cast<int>(indices.size()));
		return indices;
	}

	const LocationDefinition *GetRandomCityLocationDefinitionIfType(const ProvinceDefinition &provinceDef,
		ArenaTypes::CityType cityType)
	{
		// Iterate over locations in the province in a random order.
		const std::vector<int> randomLocationIndices = MakeShuffledLocationIndices(provinceDef);

		for (const int locationIndex : randomLocationIndices)
		{
			const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
			if (curLocationDef.getType() == LocationDefinition::Type::City)
			{
				const auto &curCityDef = curLocationDef.getCityDefinition();
				if (curCityDef.type == cityType)
				{
					return &curLocationDef;
				}
			}
		}

		return nullptr;
	}

	int GetRandomCityLocationIndex(const ProvinceDefinition &provinceDef)
	{
		// Iterate over locations in the province in a random order.
		const std::vector<int> randomLocationIndices = MakeShuffledLocationIndices(provinceDef);

		for (const int locationIndex : randomLocationIndices)
		{
			const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
			if (curLocationDef.getType() == LocationDefinition::Type::City)
			{
				return locationIndex;
			}
		}

		return -1;
	}

	const LocationDefinition *GetRandomDungeonLocationDefinition(const ProvinceDefinition &provinceDef)
	{
		// Iterate over locations in the province in a random order.
		const std::vector<int> randomLocationIndices = MakeShuffledLocationIndices(provinceDef);

		for (const int locationIndex : randomLocationIndices)
		{
			const LocationDefinition &curLocationDef = provinceDef.getLocationDef(locationIndex);
			if (curLocationDef.getType() == LocationDefinition::Type::Dungeon)
			{
				return &curLocationDef;
			}
		}

		return nullptr;
	}

	// Prefixes for some .MIF files, with an inclusive min/max range of ID suffixes.
	// These also need ".MIF" appended at the end.
	const std::vector<std::tuple<std::string, std::pair<int, int>, ArenaTypes::InteriorType>> InteriorLocations =
	{
		{ "BS", { 1, 8 }, ArenaTypes::InteriorType::House },
		{ "EQUIP", { 1, 8 }, ArenaTypes::InteriorType::Equipment },
		{ "MAGE", { 1, 8 }, ArenaTypes::InteriorType::MagesGuild },
		{ "NOBLE", { 1, 8 }, ArenaTypes::InteriorType::Noble },
		{ "PALACE", { 1, 5 }, ArenaTypes::InteriorType::Palace },
		{ "TAVERN", { 1, 8 }, ArenaTypes::InteriorType::Tavern },
		{ "TEMPLE", { 1, 8 }, ArenaTypes::InteriorType::Temple },
		{ "TOWER", { 1, 8 }, ArenaTypes::InteriorType::Tower },
		{ "TOWNPAL", { 1, 3 }, ArenaTypes::InteriorType::Palace },
		{ "VILPAL", { 1, 3 }, ArenaTypes::InteriorType::Palace },
		{ "WCRYPT", { 1, 8 }, ArenaTypes::InteriorType::Crypt }
	};

	const std::string ImperialMIF = "IMPERIAL.MIF";
	const std::string RandomCity = "Random City";
	const std::string RandomTown = "Random Town";
	const std::string RandomVillage = "Random Village";
	const std::vector<std::string> CityLocations =
	{
		ImperialMIF,
		RandomCity,
		RandomTown,
		RandomVillage
	};

	const std::vector<std::string> WildernessLocations =
	{
		"WILD" // Arbitrary name (since it's not loading WILD.MIF anymore).
	};

	const std::string RandomNamedDungeon = "Random Named";
	const std::string RandomWildDungeon = "Random Wild";
	const std::vector<std::string> DungeonLocations =
	{
		RandomNamedDungeon,
		RandomWildDungeon
	};

	// Values for testing.
	const std::vector<WeatherType> Weathers =
	{
		WeatherType::Clear,
		WeatherType::Overcast,
		WeatherType::Rain,
		WeatherType::Snow,
		WeatherType::SnowOvercast,
		WeatherType::Rain2,
		WeatherType::Overcast2,
		WeatherType::SnowOvercast2
	};

	const std::unordered_map<WeatherType, std::string> WeatherTypeNames =
	{
		{ WeatherType::Clear, "Clear" },
		{ WeatherType::Overcast, "Overcast" },
		{ WeatherType::Rain, "Rain" },
		{ WeatherType::Snow, "Snow" },
		{ WeatherType::SnowOvercast, "Snow Overcast" },
		{ WeatherType::Rain2, "Rain 2" },
		{ WeatherType::Overcast2, "Overcast 2" },
		{ WeatherType::SnowOvercast2, "Snow Overcast 2" }
	};
}

MainMenuPanel::MainMenuPanel(Game &game)
	: Panel(game)
{
	this->loadButton = []()
	{
		Int2 center(168, 58);
		int width = 150;
		int height = 20;
		auto function = [](Game &game)
		{
			game.setPanel<LoadSavePanel>(game, LoadSavePanel::Type::Load);
		};
		return Button<Game&>(center, width, height, function);
	}();

	this->newButton = []()
	{
		Int2 center(168, 112);
		int width = 150;
		int height = 20;
		auto function = [](Game &game)
		{
			// Link together the opening scroll, intro cinematic, and character creation.
			auto changeToCharCreation = [](Game &game)
			{
				game.setCharacterCreationState(std::make_unique<CharacterCreationState>());
				game.setPanel<ChooseClassCreationPanel>(game);

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
					5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0
				};

				game.setPanel<ImageSequencePanel>(
					game,
					paletteNames,
					textureNames,
					imageDurations,
					changeToCharCreation);
			};

			game.setPanel<CinematicPanel>(
				game,
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
		};

		return Button<Game&>(center, width, height, function);
	}();

	this->quickStartButton = [&game]()
	{
		auto function = [](Game &game, int testType, int testIndex, const std::string &mifName,
			const std::optional<ArenaTypes::InteriorType> &optInteriorType, WeatherType weatherType, MapType mapType)
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
			auto gameData = std::make_unique<GameData>(Player::makeRandom(
				game.getCharacterClassLibrary(), binaryAssetLibrary.getExeData(), game.getRandom()),
				binaryAssetLibrary);

			const int starCount = SkyUtils::getStarCountFromDensity(
				options.getMisc_StarDensity());

			// Load the selected level based on world type (writing into active game data).
			if (mapType == MapType::Interior)
			{
				if (testType != TestType_Dungeon)
				{
					MIFFile mif;
					if (!mif.init(mifName.c_str()))
					{
						DebugCrash("Could not init .MIF file \"" + mifName + "\".");
					}

					const Player &player = gameData->getPlayer();
					const WorldMapDefinition &worldMapDef = gameData->getWorldMapDefinition();

					// Set some interior location data for testing, depending on whether it's a
					// main quest dungeon.
					int locationIndex, provinceIndex;
					if (testType == TestType_MainQuest)
					{
						// Fetch from a global function.
						const auto &exeData = game.getBinaryAssetLibrary().getExeData();
						SpecialCaseType specialCaseType;
						GetMainQuestLocationFromIndex(testIndex, exeData, &locationIndex, &provinceIndex, &specialCaseType);

						if (specialCaseType == SpecialCaseType::None)
						{
							// Do nothing.
						}
						else if (specialCaseType == SpecialCaseType::StartDungeon)
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
						locationIndex = GetRandomCityLocationIndex(worldMapDef.getProvinceDef(provinceIndex));
					}

					const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
					const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);

					DebugAssert(optInteriorType.has_value());
					const ArenaTypes::InteriorType interiorType = *optInteriorType;
					if (!gameData->loadInterior(locationDef, provinceDef, interiorType, mif,
						game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
						binaryAssetLibrary, game.getRandom(), game.getTextureManager(), renderer))
					{
						DebugCrash("Couldn't load interior \"" + locationDef.getName() + "\".");
					}
				}
				else
				{
					// Pick a random dungeon based on the dungeon type.
					const WorldMapDefinition &worldMapDef = gameData->getWorldMapDefinition();
					const ProvinceDefinition &provinceDef = [&game, &worldMapDef]()
					{
						const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
						return worldMapDef.getProvinceDef(provinceIndex);
					}();

					constexpr bool isArtifactDungeon = false;

					if (mifName == RandomNamedDungeon)
					{
						const LocationDefinition *locationDefPtr = GetRandomDungeonLocationDefinition(provinceDef);
						DebugAssertMsg(locationDefPtr != nullptr,
							"Couldn't find named dungeon in \"" + provinceDef.getName() + "\".");

						if (!gameData->loadNamedDungeon(*locationDefPtr, provinceDef, isArtifactDungeon,
							game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
							binaryAssetLibrary, game.getRandom(), game.getTextureManager(), renderer))
						{
							DebugCrash("Couldn't load named dungeon \"" + locationDefPtr->getName() + "\".");
						}

						// Set random named dungeon name and visibility for testing.
						LocationInstance &locationInst = gameData->getLocationInstance();
						locationInst.setNameOverride("Test Dungeon");

						if (!locationInst.isVisible())
						{
							locationInst.toggleVisibility();
						}
					}
					else if (mifName == RandomWildDungeon)
					{
						Random &random = game.getRandom();
						const int wildBlockX = random.next(RMDFile::WIDTH);
						const int wildBlockY = random.next(RMDFile::DEPTH);

						const int locationIndex = GetRandomCityLocationIndex(provinceDef);
						const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);

						if (!gameData->loadWildernessDungeon(locationDef, provinceDef, wildBlockX, wildBlockY,
							binaryAssetLibrary.getCityDataFile(), game.getEntityDefinitionLibrary(),
							game.getCharacterClassLibrary(), binaryAssetLibrary, game.getRandom(),
							game.getTextureManager(), renderer))
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
				if (mifName == ImperialMIF)
				{
					// Load city into game data.
					const int provinceIndex = LocationUtils::CENTER_PROVINCE_ID;
					const WorldMapDefinition &worldMapDef = gameData->getWorldMapDefinition();
					const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
					const LocationDefinition &locationDef = [&mifName, &provinceDef]()
					{
						int locationIndex = -1;
						for (int i = 0; i < provinceDef.getLocationCount(); i++)
						{
							const LocationDefinition &curLocationDef = provinceDef.getLocationDef(i);
							if (curLocationDef.getType() == LocationDefinition::Type::City)
							{
								const LocationDefinition::CityDefinition &cityDef = curLocationDef.getCityDefinition();
								if ((cityDef.type == ArenaTypes::CityType::CityState) && cityDef.premade &&
									cityDef.palaceIsMainQuestDungeon)
								{
									locationIndex = i;
									break;
								}
							}
						}

						DebugAssertMsg(locationIndex != -1, "Couldn't find location for \"" + mifName + "\".");
						return provinceDef.getLocationDef(locationIndex);
					}();

					if (!gameData->loadCity(locationDef, provinceDef, weatherType, starCount,
						game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
						binaryAssetLibrary, game.getTextAssetLibrary(), game.getRandom(),
						game.getTextureManager(), renderer))
					{
						DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
					}
				}
				else
				{
					// Pick a random location based on the .MIF name, excluding the center province.
					const WorldMapDefinition &worldMapDef = gameData->getWorldMapDefinition();
					const ProvinceDefinition &provinceDef = [&game, &worldMapDef]()
					{
						const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
						return worldMapDef.getProvinceDef(provinceIndex);
					}();

					const ArenaTypes::CityType targetCityType = [&mifName]()
					{
						if (mifName == RandomCity)
						{
							return ArenaTypes::CityType::CityState;
						}
						else if (mifName == RandomTown)
						{
							return ArenaTypes::CityType::Town;
						}
						else if (mifName == RandomVillage)
						{
							return ArenaTypes::CityType::Village;
						}
						else
						{
							DebugUnhandledReturnMsg(ArenaTypes::CityType, mifName);
						}
					}();

					const LocationDefinition *locationDefPtr = GetRandomCityLocationDefinitionIfType(provinceDef, targetCityType);
					DebugAssertMsg(locationDefPtr != nullptr, "Couldn't find city for \"" + mifName + "\".");

					const LocationDefinition::CityDefinition &cityDef = locationDefPtr->getCityDefinition();
					const WeatherType filteredWeatherType =
						WeatherUtils::getFilteredWeatherType(weatherType, cityDef.climateType);

					// Load city into game data. Location data is loaded, too.
					if (!gameData->loadCity(*locationDefPtr, provinceDef, filteredWeatherType, starCount,
						game.getEntityDefinitionLibrary(), game.getCharacterClassLibrary(),
						binaryAssetLibrary, game.getTextAssetLibrary(), game.getRandom(),
						game.getTextureManager(), renderer))
					{
						DebugCrash("Couldn't load city \"" + locationDefPtr->getName() + "\".");
					}
				}
			}
			else if (mapType == MapType::Wilderness)
			{
				// Pick a random location and province.
				const WorldMapDefinition &worldMapDef = gameData->getWorldMapDefinition();
				const ProvinceDefinition &provinceDef = [&game, &worldMapDef]()
				{
					const int provinceIndex = game.getRandom().next(worldMapDef.getProvinceCount() - 1);
					return worldMapDef.getProvinceDef(provinceIndex);
				}();

				const int locationIndex = GetRandomCityLocationIndex(provinceDef);
				const LocationDefinition &locationDef = provinceDef.getLocationDef(locationIndex);

				const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
				const WeatherType filteredWeatherType =
					WeatherUtils::getFilteredWeatherType(weatherType, cityDef.climateType);

				// Load wilderness into game data. Location data is loaded, too.
				const bool ignoreGatePos = true;
				if (!gameData->loadWilderness(locationDef, provinceDef, Int2(), Int2(), ignoreGatePos,
					filteredWeatherType, starCount, game.getEntityDefinitionLibrary(),
					game.getCharacterClassLibrary(), binaryAssetLibrary, game.getRandom(),
					game.getTextureManager(), renderer))
				{
					DebugCrash("Couldn't load wilderness \"" + locationDef.getName() + "\".");
				}
			}
			else
			{
				DebugCrash("Unrecognized world type \"" + 
					std::to_string(static_cast<int>(mapType)) + "\".");
			}

			// Set clock to 5:45am.
			auto &clock = gameData->getClock();
			clock = Clock(5, 45, 0);

			// Get the music that should be active on start.
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = [&game, &mifName, mapType, &gameData, &musicLibrary]()
			{
				const bool isExterior = (mapType == MapType::City) ||
					(mapType == MapType::Wilderness);

				// Exteriors depend on the time of day for which music to use. Interiors depend
				// on the current location's .MIF name (if any).
				if (isExterior)
				{
					// Make sure to get updated weather type from game data and not local variable
					// so it gets the filtered weather type.
					const WeatherType weatherType = gameData->getWeatherType();
					if (!gameData->nightMusicIsActive())
					{
						return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
							game.getRandom(), [weatherType](const MusicDefinition &def)
						{
							DebugAssert(def.getType() == MusicDefinition::Type::Weather);
							const auto &weatherMusicDef = def.getWeatherMusicDefinition();
							return weatherMusicDef.type == weatherType;
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
					MusicDefinition::InteriorMusicDefinition::Type interiorMusicType;
					if (MusicUtils::tryGetInteriorMusicType(mifName, &interiorMusicType))
					{
						// Non-dungeon interior.
						return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Interior,
							game.getRandom(), [interiorMusicType](const MusicDefinition &def)
						{
							DebugAssert(def.getType() == MusicDefinition::Type::Interior);
							const auto &interiorMusicDef = def.getInteriorMusicDefinition();
							return interiorMusicDef.type == interiorMusicType;
						});
					}
					else
					{
						// Dungeon.
						return musicLibrary.getRandomMusicDefinition(
							MusicDefinition::Type::Dungeon, game.getRandom());
					}
				}
			}();

			const MusicDefinition *jingleMusicDef = [&game, mapType, &gameData, &musicLibrary]()
				-> const MusicDefinition*
			{
				const LocationDefinition &locationDef = gameData->getLocationDefinition();
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

			// Set the game data before constructing the game world panel.
			game.setGameData(std::move(gameData));

			// Initialize game world panel.
			game.setPanel<GameWorldPanel>(game);

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef, jingleMusicDef);
		};

		return Button<Game&, int, int, const std::string&,
			const std::optional<ArenaTypes::InteriorType>&, WeatherType, MapType>(function);
	}();

	this->exitButton = []()
	{
		Int2 center(168, 158);
		int width = 45;
		int height = 20;
		auto function = []()
		{
			SDL_Event e;
			e.quit.type = SDL_QUIT;
			e.quit.timestamp = 0;
			SDL_PushEvent(&e);
		};
		return Button<>(center, width, height, function);
	}();

	this->testTypeUpButton = []()
	{
		const int x = 312;
		const int y = 164;
		const int width = 8;
		const int height = 8;
		auto function = [](MainMenuPanel &panel)
		{
			panel.testType = (panel.testType > 0) ? (panel.testType - 1) : (MaxTestTypes - 1);

			// Reset the other indices.
			panel.testIndex = 0;
			panel.testIndex2 = 1;
			panel.testWeather = 0;
		};
		return Button<MainMenuPanel&>(x, y, width, height, function);
	}();

	this->testTypeDownButton = [this]()
	{
		const int x = this->testTypeUpButton.getX();
		const int y = this->testTypeUpButton.getY() + this->testTypeUpButton.getHeight();
		const int width = this->testTypeUpButton.getWidth();
		const int height = this->testTypeUpButton.getHeight();
		auto function = [](MainMenuPanel &panel)
		{
			panel.testType = (panel.testType < (MaxTestTypes - 1)) ? (panel.testType + 1) : 0;

			// Reset the other indices.
			panel.testIndex = 0;
			panel.testIndex2 = 1;
			panel.testWeather = 0;
		};
		return Button<MainMenuPanel&>(x, y, width, height, function);
	}();

	this->testIndexUpButton = [this]()
	{
		const int x = this->testTypeUpButton.getX() - this->testTypeUpButton.getWidth() - 2;
		const int y = this->testTypeUpButton.getY() +
			(this->testTypeUpButton.getHeight() * 2) + 2;
		const int width = this->testTypeDownButton.getWidth();
		const int height = this->testTypeDownButton.getHeight();
		auto function = [](MainMenuPanel &panel)
		{
			const int count = [&panel]()
			{
				// Check test type to determine the max.
				if (panel.testType == TestType_MainQuest)
				{
					return MainQuestLocationCount;
				}
				else if (panel.testType == TestType_Interior)
				{
					return static_cast<int>(InteriorLocations.size());
				}
				else if (panel.testType == TestType_City)
				{
					return static_cast<int>(CityLocations.size());
				}
				else if (panel.testType == TestType_Wilderness)
				{
					return static_cast<int>(WildernessLocations.size());
				}
				else
				{
					return static_cast<int>(DungeonLocations.size());
				}
			}();

			panel.testIndex = (panel.testIndex > 0) ? (panel.testIndex - 1) : (count - 1);

			if (panel.testType == TestType_Interior)
			{
				// Reset the second index.
				panel.testIndex2 = 1;
			}
		};
		return Button<MainMenuPanel&>(x, y, width, height, function);
	}();

	this->testIndexDownButton = [this]()
	{
		const int x = this->testIndexUpButton.getX();
		const int y = this->testIndexUpButton.getY() + this->testIndexUpButton.getHeight();
		const int width = this->testIndexUpButton.getWidth();
		const int height = this->testIndexUpButton.getHeight();
		auto function = [](MainMenuPanel &panel)
		{
			const int count = [&panel]()
			{
				// Check test type to determine the max.
				if (panel.testType == TestType_MainQuest)
				{
					return MainQuestLocationCount;
				}
				else if (panel.testType == TestType_Interior)
				{
					return static_cast<int>(InteriorLocations.size());
				}
				else if (panel.testType == TestType_City)
				{
					return static_cast<int>(CityLocations.size());
				}
				else if (panel.testType == TestType_Wilderness)
				{
					return static_cast<int>(WildernessLocations.size());
				}
				else
				{
					return static_cast<int>(DungeonLocations.size());
				}
			}();

			panel.testIndex = (panel.testIndex < (count - 1)) ? (panel.testIndex + 1) : 0;

			if (panel.testType == TestType_Interior)
			{
				// Reset the second index.
				panel.testIndex2 = 1;
			}
		};
		return Button<MainMenuPanel&>(x, y, width, height, function);
	}();

	this->testIndex2UpButton = [this]()
	{
		const int x = this->testIndexUpButton.getX() + 10;
		const int y = this->testIndexUpButton.getY();
		const int width = this->testIndexUpButton.getWidth();
		const int height = this->testIndexUpButton.getHeight();
		auto function = [](MainMenuPanel &panel)
		{
			DebugAssert(panel.testType == TestType_Interior);

			// Interior range.
			const auto &interior = InteriorLocations.at(panel.testIndex);
			const int minIndex = std::get<1>(interior).first;
			const int maxIndex = std::get<1>(interior).second;

			panel.testIndex2 = (panel.testIndex2 < maxIndex) ? (panel.testIndex2 + 1) : minIndex;
		};
		return Button<MainMenuPanel&>(x, y, width, height, function);
	}();

	this->testIndex2DownButton = [this]()
	{
		const int x = this->testIndex2UpButton.getX();
		const int y = this->testIndex2UpButton.getY() + this->testIndex2UpButton.getHeight();
		const int width = this->testIndex2UpButton.getWidth();
		const int height = this->testIndex2UpButton.getHeight();
		auto function = [](MainMenuPanel &panel)
		{
			DebugAssert(panel.testType == TestType_Interior);

			// Interior range.
			const auto &interior = InteriorLocations.at(panel.testIndex);
			const int minIndex = std::get<1>(interior).first;
			const int maxIndex = std::get<1>(interior).second;

			panel.testIndex2 = (panel.testIndex2 > minIndex) ? (panel.testIndex2 - 1) : maxIndex;
		};
		return Button<MainMenuPanel&>(x, y, width, height, function);
	}();

	this->testWeatherUpButton = [this]()
	{
		const int x = this->testTypeUpButton.getX();
		const int y = this->testTypeUpButton.getY() - 2 -
			(2 * this->testTypeUpButton.getHeight());
		const int width = this->testTypeUpButton.getWidth();
		const int height = this->testTypeUpButton.getHeight();
		auto function = [](MainMenuPanel &panel)
		{
			DebugAssert((panel.testType == TestType_City) ||
				(panel.testType == TestType_Wilderness));

			panel.testWeather = [&panel]()
			{
				const int count = static_cast<int>(Weathers.size());
				return (panel.testWeather > 0) ? (panel.testWeather - 1) : (count - 1);
			}();
		};
		return Button<MainMenuPanel&>(x, y, width, height, function);
	}();

	this->testWeatherDownButton = [this]()
	{
		const int x = this->testWeatherUpButton.getX();
		const int y = this->testWeatherUpButton.getY() + this->testWeatherUpButton.getHeight();
		const int width = this->testWeatherUpButton.getWidth();
		const int height = this->testWeatherUpButton.getHeight();
		auto function = [](MainMenuPanel &panel)
		{
			DebugAssert((panel.testType == TestType_City) ||
				(panel.testType == TestType_Wilderness));

			panel.testWeather = [&panel]()
			{
				const int count = static_cast<int>(Weathers.size());
				return (panel.testWeather < (count - 1)) ? (panel.testWeather + 1) : 0;
			}();
		};
		return Button<MainMenuPanel&>(x, y, width, height, function);
	}();

	this->testType = 0;
	this->testIndex = 0;
	this->testIndex2 = 1;
	this->testWeather = 0;

	// The game data should not be active on the main menu.
	DebugAssert(!game.gameDataIsActive());
}

std::string MainMenuPanel::getSelectedTestName() const
{
	if (this->testType == TestType_MainQuest)
	{
		auto &game = this->getGame();
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		const auto &exeData = binaryAssetLibrary.getExeData();

		// Decide how to get the main quest dungeon name.
		if (this->testIndex == 0)
		{
			// Start dungeon.
			return String::toUppercase(exeData.locations.startDungeonMifName);
		}
		else if (this->testIndex == (MainQuestLocationCount - 1))
		{
			// Final dungeon.
			return String::toUppercase(exeData.locations.finalDungeonMifName);
		}
		else
		{
			// Generate the location from the executable data, fetching data from a
			// global function.
			int locationID, provinceID;
			SpecialCaseType specialCaseType;
			GetMainQuestLocationFromIndex(this->testIndex, exeData, &locationID, &provinceID, &specialCaseType);
			DebugAssert(specialCaseType == SpecialCaseType::None);

			// Calculate the .MIF name from the dungeon seed.
			const auto &cityData = binaryAssetLibrary.getCityDataFile();
			const uint32_t dungeonSeed = [&cityData, locationID, provinceID]()
			{
				const auto &province = cityData.getProvinceData(provinceID);
				const int localDungeonID = locationID - 32;
				return LocationUtils::getDungeonSeed(localDungeonID, provinceID, province);
			}();

			const std::string mifName = LocationUtils::getMainQuestDungeonMifName(dungeonSeed);
			return String::toUppercase(mifName);
		}
	}
	else if (this->testType == TestType_Interior)
	{
		const auto &interior = InteriorLocations.at(this->testIndex);
		return std::get<0>(interior) + std::to_string(this->testIndex2) + ".MIF";
	}
	else if (this->testType == TestType_City)
	{
		return CityLocations.at(this->testIndex);
	}
	else if (this->testType == TestType_Wilderness)
	{
		return WildernessLocations.at(this->testIndex);
	}
	else
	{
		return DungeonLocations.at(this->testIndex);
	}
}

std::optional<ArenaTypes::InteriorType> MainMenuPanel::getSelectedTestInteriorType() const
{
	if (this->testType == TestType_MainQuest || this->testType == TestType_Dungeon)
	{
		return ArenaTypes::InteriorType::Dungeon;
	}
	else if (this->testType == TestType_Interior)
	{
		const auto &interior = InteriorLocations.at(this->testIndex);
		return std::get<2>(interior);
	}
	else if (this->testType == TestType_City || this->testType == TestType_Wilderness)
	{
		return std::nullopt;
	}
	else
	{
		DebugCrash("Unimplemented test type \"" + std::to_string(this->testType) + "\".");
		return std::nullopt;
	}
}

WeatherType MainMenuPanel::getSelectedTestWeatherType() const
{
	return Weathers.at(this->testWeather);
}

MapType MainMenuPanel::getSelectedTestMapType() const
{
	if ((this->testType == TestType_MainQuest) ||
		(this->testType == TestType_Interior) ||
		(this->testType == TestType_Dungeon))
	{
		return MapType::Interior;
	}
	else if (this->testType == TestType_City)
	{
		return MapType::City;
	}
	else
	{
		return MapType::Wilderness;
	}
}

std::optional<Panel::CursorData> MainMenuPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void MainMenuPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool lPressed = inputManager.keyPressed(e, SDLK_l);
	bool sPressed = inputManager.keyPressed(e, SDLK_s);
	bool ePressed = inputManager.keyPressed(e, SDLK_e);
	bool fPressed = inputManager.keyPressed(e, SDLK_f);

	if (lPressed)
	{
		this->loadButton.click(this->getGame());
	}
	else if (sPressed)
	{
		this->newButton.click(this->getGame());
	}
	else if (ePressed)
	{
		this->exitButton.click();
	}
	else if (fPressed)
	{
		// Enter the game world immediately (for testing purposes). Use the test traits
		// selected on the main menu.
		this->quickStartButton.click(this->getGame(),
			this->testType,
			this->testIndex,
			this->getSelectedTestName(),
			this->getSelectedTestInteriorType(),
			this->getSelectedTestWeatherType(),
			this->getSelectedTestMapType());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		if (this->loadButton.contains(originalPoint))
		{
			this->loadButton.click(this->getGame());
		}
		else if (this->newButton.contains(originalPoint))
		{
			this->newButton.click(this->getGame());
		}
		else if (this->exitButton.contains(originalPoint))
		{
			this->exitButton.click();
		}
		else if (TestButtonRect.contains(originalPoint))
		{
			// Enter the game world immediately (for testing purposes). Use the test traits
			// selected on the main menu.
			this->quickStartButton.click(this->getGame(),
				this->testType,
				this->testIndex,
				this->getSelectedTestName(),
				this->getSelectedTestInteriorType(),
				this->getSelectedTestWeatherType(),
				this->getSelectedTestMapType());
		}
		else if (this->testTypeUpButton.contains(originalPoint))
		{
			this->testTypeUpButton.click(*this);
		}
		else if (this->testTypeDownButton.contains(originalPoint))
		{
			this->testTypeDownButton.click(*this);
		}
		else if (this->testIndexUpButton.contains(originalPoint))
		{
			this->testIndexUpButton.click(*this);
		}
		else if (this->testIndexDownButton.contains(originalPoint))
		{
			this->testIndexDownButton.click(*this);
		}
		else if (this->testType == TestType_Interior)
		{
			// These buttons are only available when selecting interior names.
			if (this->testIndex2UpButton.contains(originalPoint))
			{
				this->testIndex2UpButton.click(*this);
			}
			else if (this->testIndex2DownButton.contains(originalPoint))
			{
				this->testIndex2DownButton.click(*this);
			}
		}
		else if (this->testType == TestType_City)
		{
			// These buttons are only available when selecting city names.
			if (this->testWeatherUpButton.contains(originalPoint))
			{
				this->testWeatherUpButton.click(*this);
			}
			else if (this->testWeatherDownButton.contains(originalPoint))
			{
				this->testWeatherDownButton.click(*this);
			}
		}
		else if (this->testType == TestType_Wilderness)
		{
			// These buttons are only available when selecting wilderness names.
			if (this->testWeatherUpButton.contains(originalPoint))
			{
				this->testWeatherUpButton.click(*this);
			}
			else if (this->testWeatherDownButton.contains(originalPoint))
			{
				this->testWeatherDownButton.click(*this);
			}
		}
	}
}

void MainMenuPanel::renderTestUI(Renderer &renderer)
{
	// Draw test buttons.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &arrowsPaletteFilename = ArenaPaletteName::CharSheet;
	const std::optional<PaletteID> arrowsPaletteID = textureManager.tryGetPaletteID(arrowsPaletteFilename.c_str());
	if (!arrowsPaletteID.has_value())
	{
		DebugLogError("Couldn't get arrows palette ID for \"" + arrowsPaletteFilename + "\".");
		return;
	}

	const std::string &arrowsTextureFilename = ArenaTextureName::UpDown;
	const std::optional<TextureBuilderID> arrowsTextureBuilderID =
		textureManager.tryGetTextureBuilderID(arrowsTextureFilename.c_str());
	if (!arrowsTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get arrows texture builder ID for \"" + arrowsTextureFilename + "\".");
		return;
	}

	renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
		this->testTypeUpButton.getX(), this->testTypeUpButton.getY(), textureManager);
	renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
		this->testIndexUpButton.getX(), this->testIndexUpButton.getY(), textureManager);

	if (this->testType == TestType_Interior)
	{
		renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
			this->testIndex2UpButton.getX(), this->testIndex2UpButton.getY(), textureManager);
	}
	else if ((this->testType == TestType_City) || (this->testType == TestType_Wilderness))
	{
		renderer.drawOriginal(*arrowsTextureBuilderID, *arrowsPaletteID,
			this->testWeatherUpButton.getX(), this->testWeatherUpButton.getY(), textureManager);
	}

	const Texture testButton = TextureUtils::generate(TextureUtils::PatternType::Custom1,
		TestButtonRect.getWidth(), TestButtonRect.getHeight(), textureManager, renderer);
	renderer.drawOriginal(testButton, TestButtonRect.getLeft(), TestButtonRect.getTop(),
		testButton.getWidth(), testButton.getHeight());

	// Draw test text.
	const auto &fontLibrary = this->getGame().getFontLibrary();
	const RichTextString testButtonText(
		"Test",
		FontName::Arena,
		Color::White,
		TextAlignment::Center,
		fontLibrary);

	const Int2 testButtonTextBoxPoint(
		TestButtonRect.getLeft() + (TestButtonRect.getWidth() / 2),
		TestButtonRect.getTop() + (TestButtonRect.getHeight() / 2));
	const TextBox testButtonTextBox(testButtonTextBoxPoint, testButtonText, fontLibrary, renderer);

	renderer.drawOriginal(testButtonTextBox.getTexture(),
		testButtonTextBox.getX(), testButtonTextBox.getY());

	const std::string testTypeName = [this]()
	{
		if (this->testType == TestType_MainQuest)
		{
			return "Main Quest";
		}
		else if (this->testType == TestType_Interior)
		{
			return "Interior";
		}
		else if (this->testType == TestType_City)
		{
			return "City";
		}
		else if (this->testType == TestType_Wilderness)
		{
			return "Wilderness";
		}
		else
		{
			return "Dungeon";
		}
	}();

	const RichTextString testTypeText(
		"Test type: " + testTypeName,
		testButtonText.getFontName(),
		testButtonText.getColor(),
		TextAlignment::Left,
		fontLibrary);

	const int testTypeTextBoxX = this->testTypeUpButton.getX() -
		testTypeText.getDimensions().x - 2;
	const int testTypeTextBoxY = this->testTypeUpButton.getY() +
		(testTypeText.getDimensions().y / 2);
	const TextBox testTypeTextBox(testTypeTextBoxX, testTypeTextBoxY,
		testTypeText, fontLibrary, renderer);

	renderer.drawOriginal(testTypeTextBox.getTexture(),
		testTypeTextBox.getX(), testTypeTextBox.getY());

	const RichTextString testNameText(
		"Test location: " + this->getSelectedTestName(),
		testTypeText.getFontName(),
		testTypeText.getColor(),
		testTypeText.getAlignment(),
		fontLibrary);
	const int testNameTextBoxX = this->testIndexUpButton.getX() -
		testNameText.getDimensions().x - 2;
	const int testNameTextBoxY = this->testIndexUpButton.getY() +
		(testNameText.getDimensions().y / 2);
	const TextBox testNameTextBox(testNameTextBoxX, testNameTextBoxY,
		testNameText, fontLibrary, renderer);
	renderer.drawOriginal(testNameTextBox.getTexture(),
		testNameTextBox.getX(), testNameTextBox.getY());

	// Draw weather text if applicable.
	if ((this->testType == TestType_City) || (this->testType == TestType_Wilderness))
	{
		const WeatherType weatherType = this->getSelectedTestWeatherType();
		const std::string &weatherName = WeatherTypeNames.at(weatherType);

		const RichTextString testWeatherText(
			"Test weather: " + weatherName,
			testTypeText.getFontName(),
			testTypeText.getColor(),
			testTypeText.getAlignment(),
			fontLibrary);

		const int testWeatherTextBoxX = this->testWeatherUpButton.getX() -
			testWeatherText.getDimensions().x - 2;
		const int testWeatherTextBoxY = this->testWeatherUpButton.getY() +
			(testWeatherText.getDimensions().y / 2);
		const TextBox testWeatherTextBox(testWeatherTextBoxX, testWeatherTextBoxY,
			testWeatherText, fontLibrary, renderer);

		renderer.drawOriginal(testWeatherTextBox.getTexture(),
			testWeatherTextBox.getX(), testWeatherTextBox.getY());
	}
}

void MainMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw main menu.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &mainMenuTextureFilename = ArenaTextureName::MainMenu;
	const std::string &mainMenuPaletteFilename = mainMenuTextureFilename;
	const std::optional<PaletteID> mainMenuPaletteID = textureManager.tryGetPaletteID(mainMenuPaletteFilename.c_str());
	if (!mainMenuPaletteID.has_value())
	{
		DebugLogError("Couldn't get main menu palette ID for \"" + mainMenuPaletteFilename + "\".");
		return;
	}

	const std::optional<TextureBuilderID> mainMenuTextureBuilderID =
		textureManager.tryGetTextureBuilderID(mainMenuTextureFilename.c_str());
	if (!mainMenuTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get main menu texture builder ID for \"" + mainMenuTextureFilename + "\".");
		return;
	}

	renderer.drawOriginal(*mainMenuTextureBuilderID, *mainMenuPaletteID, textureManager);
	this->renderTestUI(renderer);
}
