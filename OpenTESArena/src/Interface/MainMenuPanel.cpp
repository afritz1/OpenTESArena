#include <cassert>
#include <unordered_map>
#include <vector>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "CinematicPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "ImageSequencePanel.h"
#include "LoadGamePanel.h"
#include "MainMenuPanel.h"
#include "../Assets/CityDataFile.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/MiscAssets.h"
#include "../Assets/RMDFile.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Interface/RichTextString.h"
#include "../Interface/TextAlignment.h"
#include "../Interface/TextBox.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Rendering/Texture.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"
#include "../World/Location.h"
#include "../World/LocationType.h"
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
	const int MaxTestTypes = 5;
	const int TestType_MainQuest = 0;
	const int TestType_Interior = 1;
	const int TestType_City = 2;
	const int TestType_Wilderness = 3;
	const int TestType_Dungeon = 4;

	const Rect TestButtonRect(135, Renderer::ORIGINAL_HEIGHT - 17, 30, 14);

	// Main quest locations. There are eight map dungeons and eight staff dungeons.
	// The special cases are the start dungeon and the final dungeon.
	const int MainQuestLocationCount = 18;
	const Location StartDungeonLocation = Location::makeSpecialCase(
		Location::SpecialCaseType::StartDungeon, 8);
	const Location FinalDungeonLocation = Location::makeCity(0, 8);

	Location getMainQuestLocationFromIndex(Game &game, int testIndex)
	{
		if (testIndex == 0)
		{
			return StartDungeonLocation;
		}
		else if (testIndex == (MainQuestLocationCount - 1))
		{
			return FinalDungeonLocation;
		}
		else
		{
			// Generate the location from the executable data.
			const auto &miscAssets = game.getMiscAssets();
			const auto &exeData = miscAssets.getExeData();
			const auto &staffProvinces = exeData.locations.staffProvinces;
			const int localDungeonID = testIndex % 2;
			const int provinceID = staffProvinces.at((testIndex - 1) / 2);
			return Location::makeDungeon(localDungeonID, provinceID);
		}
	}

	// Prefixes for some .MIF files, with an inclusive min/max range of ID suffixes.
	// These also need ".MIF" appended at the end.
	const std::vector<std::pair<std::string, std::pair<int, int>>> InteriorLocations =
	{
		{ "BS", { 1, 5 } },
		{ "EQUIP", { 1, 8 } },
		{ "MAGE", { 1, 8 } },
		{ "NOBLE", { 1, 8 } },
		{ "PALACE", { 1, 5 } },
		{ "TAVERN", { 1, 8 } },
		{ "TEMPLE", { 1, 8 } },
		{ "TOWER", { 1, 8 } },
		{ "TOWNPAL", { 1, 3 } },
		{ "VILPAL", { 1, 3 } },
		{ "WCRYPT", { 1, 8 } }
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
		"WILD.MIF"
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
		WeatherType::Snow
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
			game.setPanel<LoadGamePanel>(game);
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
				game.setPanel<ChooseClassCreationPanel>(game);
				game.setMusic(MusicName::Sheet);
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
				PaletteFile::fromName(PaletteName::Default),
				TextureFile::fromName(TextureSequenceName::OpeningScroll),
				0.042,
				changeToNewGameStory);
			game.setMusic(MusicName::EvilIntro);
		};
		return Button<Game&>(center, width, height, function);
	}();

	this->quickStartButton = [&game]()
	{
		auto function = [](Game &game, int testType, int testIndex, const std::string &mifName,
			WeatherType weatherType, WorldType worldType)
		{
			// Initialize 3D renderer.
			auto &renderer = game.getRenderer();
			const auto &options = game.getOptions();
			const bool fullGameWindow = options.getModernInterface();
			renderer.initializeWorldRendering(options.getResolutionScale(), fullGameWindow);

			// Game data instance, to be initialized further by one of the loading methods below.
			// Create a player with random data for testing.
			const auto &miscAssets = game.getMiscAssets();
			auto gameData = std::make_unique<GameData>(Player::makeRandom(
				miscAssets.getClassDefinitions(), miscAssets.getExeData()));

			// Load the selected level based on world type (writing into active game data).
			if (worldType == WorldType::City)
			{
				// There is only one "premade" city (used by the center province). All others
				// are randomly generated.
				if (mifName == ImperialMIF)
				{
					const MIFFile mif(mifName);

					// Load city into game data. Location data is loaded, too.
					gameData->loadPremadeCity(mif, weatherType, miscAssets,
						game.getTextureManager(), renderer);
				}
				else
				{
					// Pick a random location based on the .MIF name, excluding the
					// center province.
					Random random;
					const int localCityID = [&mifName, &random]()
					{
						if (mifName == RandomCity)
						{
							return random.next(8);
						}
						else if (mifName == RandomTown)
						{
							return 8 + random.next(8);
						}
						else if (mifName == RandomVillage)
						{
							return 16 + random.next(16);
						}
						else
						{
							throw std::runtime_error("Bad .MIF name \"" + mifName + "\".");
						}
					}();

					const int provinceID = random.next(8);

					// Load city into game data. Location data is loaded, too.
					gameData->loadCity(localCityID, provinceID, weatherType, miscAssets,
						game.getTextureManager(), renderer);
				}
			}
			else if (worldType == WorldType::Interior)
			{
				if (testType != TestType_Dungeon)
				{
					const MIFFile mif(mifName);
					const Player &player = gameData->getPlayer();

					// Set some interior location data for testing, depending on whether
					// it's a main quest dungeon.
					const Location location = [&game, &player, testType, testIndex]()
					{
						if (testType == TestType_MainQuest)
						{
							// Fetch from a global function.
							return getMainQuestLocationFromIndex(game, testIndex);
						}
						else
						{
							Random random;
							const int localCityID = random.next(32);
							const int provinceID = random.next(8);
							return Location::makeCity(localCityID, provinceID);
						}
					}();

					gameData->loadInterior(mif, location, game.getTextureManager(), renderer);
				}
				else
				{
					// Pick a random dungeon based on the dungeon type.
					Random random;
					const int provinceID = random.next(8);
					const bool isArtifactDungeon = false;

					if (mifName == RandomNamedDungeon)
					{
						const int localDungeonID = 2 + random.next(14);
						gameData->loadNamedDungeon(localDungeonID, provinceID, isArtifactDungeon,
							miscAssets, game.getTextureManager(), renderer);
					}
					else if (mifName == RandomWildDungeon)
					{
						const int wildBlockX = random.next(RMDFile::WIDTH);
						const int wildBlockY = random.next(RMDFile::DEPTH);
						gameData->loadWildernessDungeon(provinceID, wildBlockX, wildBlockY,
							miscAssets.getCityDataFile(), game.getTextureManager(), renderer);
					}
					else
					{
						DebugCrash("Unrecognized dungeon type \"" + mifName + "\".");
					}
				}
			}
			else if (worldType == WorldType::Wilderness)
			{
				// Pick a random location and province.
				Random random;
				const int localCityID = random.next(32);
				const int provinceID = random.next(8);

				// Pick random wilderness chunks between WILD005.RMD and WILD070.RMD.
				const int rmdTR = 5 + random.next(66);
				const int rmdTL = 5 + random.next(66);
				const int rmdBR = 5 + random.next(66);
				const int rmdBL = 5 + random.next(66);
				DebugMention(std::string("Wilderness IDs:\n") +
					"- Top right: " + std::to_string(rmdTR) + "\n" +
					"- Top left: " + std::to_string(rmdTL) + "\n" +
					"- Bottom right: " + std::to_string(rmdBR) + "\n" +
					"- Bottom left: " + std::to_string(rmdBL));

				// Load wilderness into game data. Location data is loaded, too.
				gameData->loadWilderness(localCityID, provinceID, rmdTR, rmdTL, rmdBR, rmdBL,
					weatherType, miscAssets, game.getTextureManager(), renderer);
			}
			else
			{
				DebugCrash("Unrecognized world type \"" + 
					std::to_string(static_cast<int>(worldType)) + "\".");
			}

			// Set clock to 5:45am.
			gameData->getClock() = Clock(5, 45, 0);

			// Set the game data before constructing the game world panel.
			game.setGameData(std::move(gameData));

			// -- temp, set night lights on while the hardcoded start time is early morning. --
			// - To do: day/night event is only done twice a day, so this needs to be
			//   coupled with world/clock creation instead (as part of "construction").
			renderer.setNightLightsActive(true);

			// To do: organize this code somewhere (GameData perhaps).
			// - Maybe GameWorldPanel::tick() can check newMusicName vs. old each frame.
			const MusicName musicName = [&mifName, worldType, weatherType]()
			{
				if ((worldType == WorldType::City) ||
					(worldType == WorldType::Wilderness))
				{
					// Get weather-associated music.
					const std::unordered_map<WeatherType, MusicName> WeatherMusics =
					{
						{ WeatherType::Clear, MusicName::SunnyDay },
						{ WeatherType::Overcast, MusicName::Overcast },
						{ WeatherType::Rain, MusicName::Raining },
						{ WeatherType::Snow, MusicName::Snowing }
					};

					return WeatherMusics.at(weatherType);
				}
				else
				{
					// Interior. See if it's a town interior or a dungeon.
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
						const std::vector<MusicName> TavernMusics =
						{
							MusicName::Square,
							MusicName::Tavern
						};

						Random random;
						return TavernMusics.at(random.next(
							static_cast<int>(TavernMusics.size())));
					}
					else if (isTemple)
					{
						return MusicName::Temple;
					}
					else
					{
						// Dungeon.
						const std::vector<MusicName> DungeonMusics =
						{
							MusicName::Dungeon1,
							MusicName::Dungeon2,
							MusicName::Dungeon3,
							MusicName::Dungeon4,
							MusicName::Dungeon5
						};

						Random random;
						return DungeonMusics.at(random.next(
							static_cast<int>(DungeonMusics.size())));
					}
				}
			}();

			// Initialize game world panel.
			game.setPanel<GameWorldPanel>(game);
			game.setMusic(musicName);
		};
		return Button<Game&, int, int, const std::string&, WeatherType, WorldType>(function);
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
			assert(panel.testType == TestType_Interior);

			// Interior range.
			const auto &interior = InteriorLocations.at(panel.testIndex);
			const int minIndex = interior.second.first;
			const int maxIndex = interior.second.second;

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
			assert(panel.testType == TestType_Interior);

			// Interior range.
			const auto &interior = InteriorLocations.at(panel.testIndex);
			const int minIndex = interior.second.first;
			const int maxIndex = interior.second.second;

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
			assert((panel.testType == TestType_City) ||
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
			assert((panel.testType == TestType_City) ||
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
	assert(!game.gameDataIsActive());
}

std::string MainMenuPanel::getSelectedTestName() const
{
	if (this->testType == TestType_MainQuest)
	{
		// Decide how to get the main quest dungeon name.
		if (this->testIndex == 0)
		{
			// Start dungeon.
			return "START.MIF";
		}
		else if (this->testIndex == (MainQuestLocationCount - 1))
		{
			// Final dungeon.
			return "IMPPAL.MIF";
		}
		else
		{
			// Generate the location from the executable data, fetching data from a
			// global function.
			auto &game = this->getGame();
			const Location location = getMainQuestLocationFromIndex(game, testIndex);

			// Calculate the .MIF name from the dungeon seed.
			const auto &miscAssets = game.getMiscAssets();
			const auto &cityData = miscAssets.getCityDataFile();
			const uint32_t dungeonSeed = cityData.getDungeonSeed(
				location.localDungeonID, location.provinceID);
			const std::string mifName = cityData.getMainQuestDungeonMifName(dungeonSeed);
			return mifName;
		}
	}
	else if (this->testType == TestType_Interior)
	{
		const auto &interior = InteriorLocations.at(this->testIndex);
		return interior.first + std::to_string(this->testIndex2) + ".MIF";
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

WeatherType MainMenuPanel::getSelectedTestWeatherType() const
{
	return Weathers.at(this->testWeather);
}

WorldType MainMenuPanel::getSelectedTestWorldType() const
{
	if ((this->testType == TestType_MainQuest) ||
		(this->testType == TestType_Interior) ||
		(this->testType == TestType_Dungeon))
	{
		return WorldType::Interior;
	}
	else if (this->testType == TestType_City)
	{
		return WorldType::City;
	}
	else
	{
		return WorldType::Wilderness;
	}
}

std::pair<SDL_Texture*, CursorAlignment> MainMenuPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default), renderer);
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
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
			this->getSelectedTestWeatherType(),
			this->getSelectedTestWorldType());
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
				this->getSelectedTestWeatherType(),
				this->getSelectedTestWorldType());
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

void MainMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Set palette.
	auto &textureManager = this->getGame().getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw main menu.
	const auto &mainMenu = textureManager.getTexture(
		TextureFile::fromName(TextureName::MainMenu),
		PaletteFile::fromName(PaletteName::BuiltIn), renderer);
	renderer.drawOriginal(mainMenu.get());

	// Draw test buttons.
	const auto &arrows = textureManager.getTexture(
		TextureFile::fromName(TextureName::UpDown),
		PaletteFile::fromName(PaletteName::CharSheet), renderer);
	renderer.drawOriginal(arrows.get(), this->testTypeUpButton.getX(),
		this->testTypeUpButton.getY());
	renderer.drawOriginal(arrows.get(), this->testIndexUpButton.getX(),
		this->testIndexUpButton.getY());

	if (this->testType == TestType_Interior)
	{
		renderer.drawOriginal(arrows.get(), this->testIndex2UpButton.getX(),
			this->testIndex2UpButton.getY());
	}
	else if ((this->testType == TestType_City) || (this->testType == TestType_Wilderness))
	{
		renderer.drawOriginal(arrows.get(), this->testWeatherUpButton.getX(),
			this->testWeatherUpButton.getY());
	}

	const Texture testButton(Texture::generate(
		Texture::PatternType::Custom1, TestButtonRect.getWidth(), 
		TestButtonRect.getHeight(), textureManager, renderer));
	renderer.drawOriginal(testButton.get(), 
		TestButtonRect.getLeft(), TestButtonRect.getTop(),
		testButton.getWidth(), testButton.getHeight());

	// Draw test text.
	const RichTextString testButtonText(
		"Test",
		FontName::Arena,
		Color::White,
		TextAlignment::Center,
		this->getGame().getFontManager());

	const Int2 testButtonTextBoxPoint(
		TestButtonRect.getLeft() + (TestButtonRect.getWidth() / 2),
		TestButtonRect.getTop() + (TestButtonRect.getHeight() / 2));
	const TextBox testButtonTextBox(testButtonTextBoxPoint, testButtonText, renderer);

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
		this->getGame().getFontManager());

	const int testTypeTextBoxX = this->testTypeUpButton.getX() -
		testTypeText.getDimensions().x - 2;
	const int testTypeTextBoxY = this->testTypeUpButton.getY() +
		(testTypeText.getDimensions().y / 2);
	const TextBox testTypeTextBox(testTypeTextBoxX, testTypeTextBoxY, testTypeText, renderer);

	renderer.drawOriginal(testTypeTextBox.getTexture(),
		testTypeTextBox.getX(), testTypeTextBox.getY());

	const RichTextString testNameText(
		"Test location: " + this->getSelectedTestName(),
		testTypeText.getFontName(),
		testTypeText.getColor(),
		testTypeText.getAlignment(),
		this->getGame().getFontManager());
	const int testNameTextBoxX = this->testIndexUpButton.getX() -
		testNameText.getDimensions().x - 2;
	const int testNameTextBoxY = this->testIndexUpButton.getY() +
		(testNameText.getDimensions().y / 2);
	const TextBox testNameTextBox(testNameTextBoxX, testNameTextBoxY, testNameText, renderer);
	renderer.drawOriginal(testNameTextBox.getTexture(),
		testNameTextBox.getX(), testNameTextBox.getY());

	// Draw weather text if applicable.
	if ((this->testType == TestType_City) || (this->testType == TestType_Wilderness))
	{
		const std::string weatherName = [this]()
		{
			const WeatherType weatherType = this->getSelectedTestWeatherType();

			if (weatherType == WeatherType::Clear)
			{
				return "Clear";
			}
			else if (weatherType == WeatherType::Overcast)
			{
				return "Overcast";
			}
			else if (weatherType == WeatherType::Rain)
			{
				return "Rain";
			}
			else if (weatherType == WeatherType::Snow)
			{
				return "Snow";
			}
			else
			{
				throw std::runtime_error("Bad weather type.");
			}
		}();

		const RichTextString testWeatherText(
			"Test weather: " + weatherName,
			testTypeText.getFontName(),
			testTypeText.getColor(),
			testTypeText.getAlignment(),
			this->getGame().getFontManager());

		const int testWeatherTextBoxX = this->testWeatherUpButton.getX() -
			testWeatherText.getDimensions().x - 2;
		const int testWeatherTextBoxY = this->testWeatherUpButton.getY() +
			(testWeatherText.getDimensions().y / 2);
		const TextBox testWeatherTextBox(
			testWeatherTextBoxX, testWeatherTextBoxY, testWeatherText, renderer);

		renderer.drawOriginal(testWeatherTextBox.getTexture(),
			testWeatherTextBox.getX(), testWeatherTextBox.getY());
	}
}
