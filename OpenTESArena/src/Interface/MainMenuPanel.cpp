#include <cassert>
#include <map>
#include <vector>

#include "SDL.h"

#include "ChooseClassCreationPanel.h"
#include "CinematicPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "ImageSequencePanel.h"
#include "LoadGamePanel.h"
#include "MainMenuPanel.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Assets/MiscAssets.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
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
#include "../Utilities/String.h"
#include "../World/ClimateType.h"
#include "../World/WeatherType.h"
#include "../World/WorldType.h"

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
			std::unique_ptr<Panel> loadPanel(new LoadGamePanel(game));
			game.setPanel(std::move(loadPanel));
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
				std::unique_ptr<Panel> creationPanel(new ChooseClassCreationPanel(game));
				game.setPanel(std::move(creationPanel));
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

				std::unique_ptr<Panel> newGameStoryPanel(new ImageSequencePanel(
					game,
					paletteNames,
					textureNames,
					imageDurations,
					changeToCharCreation));

				game.setPanel(std::move(newGameStoryPanel));
			};

			std::unique_ptr<Panel> cinematicPanel(new CinematicPanel(
				game,
				PaletteFile::fromName(PaletteName::Default),
				TextureFile::fromName(TextureSequenceName::OpeningScroll),
				0.042,
				changeToNewGameStory));
			game.setPanel(std::move(cinematicPanel));
			game.setMusic(MusicName::EvilIntro);
		};
		return Button<Game&>(center, width, height, function);
	}();

	this->fastStartButton = [&game]()
	{
		auto function = [](Game &game)
		{
			// Initialize 3D renderer.
			auto &renderer = game.getRenderer();
			const auto &options = game.getOptions();
			const bool fullGameWindow = options.getModernInterface();
			renderer.initializeWorldRendering(options.getResolutionScale(), fullGameWindow);

			// Generate a random player character for the game data.
			std::unique_ptr<GameData> gameData = GameData::createRandomPlayer(
				game.getMiscAssets().getClassDefinitions(), game.getTextureManager(), renderer);

			// Overwrite game level with a .MIF file.
			//const std::string mifName("START.MIF");
			//const std::string mifName("39699021.MIF");
			const std::string mifName("IMPERIAL.MIF");
			const MIFFile mif(mifName);

			// These values determine some traits of the quickstart city.
			const ClimateType climateType = ClimateType::Temperate;
			const WeatherType weatherType = WeatherType::Clear;
			const WorldType worldType = WorldType::City;

			const std::string infName = [&mif, climateType, weatherType, worldType]()
			{
				if (worldType == WorldType::Interior)
				{
					return String::toUppercase(
						mif.getLevels().at(mif.getStartingLevelIndex()).info);
				}
				else
				{
					// Exterior location, get the biome-associated pattern.
					const std::string biomeLetter = [climateType]()
					{
						if (climateType == ClimateType::Temperate)
						{
							return "T";
						}
						else if (climateType == ClimateType::Desert)
						{
							return "D";
						}
						else
						{
							return "M";
						}
					}();

					const std::string locationLetter = [worldType]()
					{
						if (worldType == WorldType::City)
						{
							return "C";
						}
						else
						{
							return "W";
						}
					}();

					const std::string weatherLetter = [weatherType]()
					{
						if ((weatherType == WeatherType::Clear) ||
							(weatherType == WeatherType::Overcast))
						{
							return "N";
						}
						else if (weatherType == WeatherType::Rain)
						{
							return "R";
						}
						else if (weatherType == WeatherType::Snow)
						{
							return "S";
						}
						else
						{
							// Not sure what this letter represents.
							return "W";
						}
					}();

					return biomeLetter + locationLetter + weatherLetter + ".INF";
				}
			}();

			const INFFile inf(infName);

			auto &player = gameData->getPlayer();
			Double3 playerPosition = player.getPosition();
			GameData::loadFromMIF(mif, inf, worldType, weatherType, playerPosition,
				gameData->getWorldData(), game.getTextureManager(), renderer);

			player.teleport(playerPosition);

			// Set the game data before constructing the game world panel.
			game.setGameData(std::move(gameData));
			
			// Set weather-relative fog distance.
			const double fogDistance = [weatherType]()
			{
				// Just some arbitrary values.
				if (weatherType == WeatherType::Clear)
				{
					return 75.0;
				}
				else if (weatherType == WeatherType::Overcast)
				{
					return 25.0;
				}
				else if (weatherType == WeatherType::Rain)
				{
					return 45.0;
				}
				else
				{
					return 15.0;
				}
			}();

			renderer.setFogDistance(fogDistance);

			// To do: organize this code somewhere (GameData perhaps).
			// - Maybe GameWorldPanel::tick() can check newMusicName vs. old each frame.
			const MusicName musicName = [&mifName, worldType, weatherType]()
			{
				if ((worldType == WorldType::City) ||
					(worldType == WorldType::Wilderness))
				{
					// Get weather-associated music.
					const std::map<WeatherType, MusicName> WeatherMusics =
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
			std::unique_ptr<Panel> gameWorldPanel(new GameWorldPanel(game));
			game.setPanel(std::move(gameWorldPanel));
			game.setMusic(musicName);
		};
		return Button<Game&>(function);
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

	// The game data should not be active on the main menu.
	assert(!game.gameDataIsActive());
}

MainMenuPanel::~MainMenuPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> MainMenuPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame().getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
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
		// Enter the game world immediately, for testing purposes.
		this->fastStartButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->loadButton.contains(mouseOriginalPoint))
		{
			this->loadButton.click(this->getGame());
		}
		else if (this->newButton.contains(mouseOriginalPoint))
		{
			this->newButton.click(this->getGame());
		}
		else if (this->exitButton.contains(mouseOriginalPoint))
		{
			this->exitButton.click();
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
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawOriginal(mainMenu.get());
}
