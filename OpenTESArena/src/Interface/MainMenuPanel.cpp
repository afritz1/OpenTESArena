#include <cassert>
#include <vector>

#include "SDL.h"

#include "MainMenuPanel.h"

#include "ChooseClassCreationPanel.h"
#include "CinematicPanel.h"
#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "ImageSequencePanel.h"
#include "LoadGamePanel.h"
#include "../Assets/INFFile.h"
#include "../Assets/MIFFile.h"
#include "../Game/Game.h"
#include "../Game/GameData.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
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

MainMenuPanel::MainMenuPanel(Game *game)
	: Panel(game)
{
	this->loadButton = []()
	{
		Int2 center(168, 58);
		int width = 150;
		int height = 20;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> loadPanel(new LoadGamePanel(game));
			game->setPanel(std::move(loadPanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(center, width, height, function));
	}();

	this->newButton = []()
	{
		Int2 center(168, 112);
		int width = 150;
		int height = 20;
		auto function = [](Game *game)
		{
			// Link together the opening scroll, intro cinematic, and character creation.
			auto changeToCharCreation = [](Game *game)
			{
				std::unique_ptr<Panel> creationPanel(new ChooseClassCreationPanel(game));
				game->setPanel(std::move(creationPanel));
				game->setMusic(MusicName::Sheet);
			};

			auto changeToNewGameStory = [changeToCharCreation](Game *game)
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

				game->setPanel(std::move(newGameStoryPanel));
			};

			std::unique_ptr<Panel> cinematicPanel(new CinematicPanel(
				game,
				PaletteFile::fromName(PaletteName::Default),
				TextureFile::fromName(TextureSequenceName::OpeningScroll),
				0.042,
				changeToNewGameStory));
			game->setPanel(std::move(cinematicPanel));
			game->setMusic(MusicName::EvilIntro);
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(center, width, height, function));
	}();

	this->fastStartButton = [game]()
	{
		auto function = [](Game *game)
		{
			// Initialize 3D renderer.
			auto &renderer = game->getRenderer();
			const auto &options = game->getOptions();
			const bool fullGameWindow = options.getPlayerInterface() == PlayerInterface::Modern;
			renderer.initializeWorldRendering(options.getResolutionScale(), fullGameWindow);

			// Generate a random player character for the game data.
			std::unique_ptr<GameData> gameData = GameData::createRandomPlayer(
				game->getTextureManager(), renderer);

			// Overwrite game level with a .MIF file.
			const MIFFile mif("START.MIF");
			//const MIFFile mif("39699021.MIF");
			const INFFile inf(String::toUppercase(
				mif.getLevels().at(mif.getStartingLevelIndex()).info));

			auto &player = gameData->getPlayer();
			Double3 playerPosition = player.getPosition();
			GameData::loadFromMIF(mif, inf, playerPosition, 
				gameData->getWorldData(), game->getTextureManager(), renderer);

			player.teleport(playerPosition);

			// Set the game data before constructing the game world panel.
			game->setGameData(std::move(gameData));

			// Initialize game world panel.
			std::unique_ptr<Panel> gameWorldPanel(new GameWorldPanel(game));
			game->setPanel(std::move(gameWorldPanel));
			game->setMusic(MusicName::Dungeon1);
		};
		return std::unique_ptr<Button<Game*>>(new Button<Game*>(function));
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
		return std::unique_ptr<Button<>>(new Button<>(center, width, height, function));
	}();

	// The game data should not be active on the main menu.
	assert(!game->gameDataIsActive());
}

MainMenuPanel::~MainMenuPanel()
{

}

std::pair<SDL_Texture*, CursorAlignment> MainMenuPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame()->getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void MainMenuPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame()->getInputManager();
	bool lPressed = inputManager.keyPressed(e, SDLK_l);
	bool sPressed = inputManager.keyPressed(e, SDLK_s);
	bool ePressed = inputManager.keyPressed(e, SDLK_e);
	bool fPressed = inputManager.keyPressed(e, SDLK_f);

	if (lPressed)
	{
		this->loadButton->click(this->getGame());
	}
	else if (sPressed)
	{
		this->newButton->click(this->getGame());
	}
	else if (ePressed)
	{
		this->exitButton->click();
	}
	else if (fPressed)
	{
		// Enter the game world immediately, for testing purposes.
		this->fastStartButton->click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		if (this->loadButton->contains(mouseOriginalPoint))
		{
			this->loadButton->click(this->getGame());
		}
		else if (this->newButton->contains(mouseOriginalPoint))
		{
			this->newButton->click(this->getGame());
		}
		else if (this->exitButton->contains(mouseOriginalPoint))
		{
			this->exitButton->click();
		}
	}
}

void MainMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw main menu.
	const auto &mainMenu = textureManager.getTexture(
		TextureFile::fromName(TextureName::MainMenu), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(mainMenu.get());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();
}
