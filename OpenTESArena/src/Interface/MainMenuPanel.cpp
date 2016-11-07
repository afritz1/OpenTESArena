#include <cassert>
#include <vector>

#include "SDL.h"

#include "MainMenuPanel.h"

#include "Button.h"
#include "ChooseClassCreationPanel.h"
#include "CinematicPanel.h"
#include "ImageSequencePanel.h"
#include "LoadGamePanel.h"
#include "Surface.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"
#include "../Rendering/Renderer.h"

MainMenuPanel::MainMenuPanel(GameState *gameState)
	: Panel(gameState)
{
	this->loadButton = []()
	{
		Int2 center(168, 58);
		int width = 150;
		int height = 20;
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> loadPanel(new LoadGamePanel(gameState));
			gameState->setPanel(std::move(loadPanel));
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->newButton = []()
	{
		Int2 center(168, 112);
		int width = 150;
		int height = 20;
		auto function = [](GameState *gameState)
		{
			// Link together the opening scroll, intro cinematic, and character creation.
			auto changeToCharCreation = [](GameState *gameState)
			{
				std::unique_ptr<Panel> creationPanel(new ChooseClassCreationPanel(gameState));
				gameState->setPanel(std::move(creationPanel));
				gameState->setMusic(MusicName::Sheet);
			};

			auto changeToNewGameStory = [changeToCharCreation](GameState *gameState)
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
					gameState,
					paletteNames,
					textureNames,
					imageDurations,
					changeToCharCreation));

				gameState->setPanel(std::move(newGameStoryPanel));
			};

			std::unique_ptr<Panel> cinematicPanel(new CinematicPanel(
				gameState,
				TextureFile::fromName(TextureSequenceName::OpeningScroll),
				PaletteFile::fromName(PaletteName::Default),
				0.042,
				changeToNewGameStory));
			gameState->setPanel(std::move(cinematicPanel));
			gameState->setMusic(MusicName::EvilIntro);
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->exitButton = []()
	{
		Int2 center(168, 158);
		int width = 45;
		int height = 20;
		auto function = [](GameState *gameState)
		{
            SDL_Event evt;
            evt.quit.type = SDL_QUIT;
            evt.quit.timestamp = 0;
            SDL_PushEvent(&evt);
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	// The game data should not be active on the main menu.
	assert(!gameState->gameDataIsActive());
}

MainMenuPanel::~MainMenuPanel()
{

}

void MainMenuPanel::handleEvents(bool &running)
{
	// The mouse will have different behavior when clicking out of the letterbox
	// depending on each panel's implementation. It might indicate to cancel
	// whatever is happening, or it might do nothing. Get the SDL mouse position
	// in the native window and see if it's within the letterbox area in order to
	// check which behavior to do.

	// If within the letterbox, subtract the letterbox origin from the mouse position
	// and downscale it by the draw scale because interface dimensions should be
	// constructed within the original sizes. Maybe there could be a
	// Panel::nativePointToOriginalInt2(nativePoint) method?

	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(mousePosition);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool escapePressed = (e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_ESCAPE);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);

		if (applicationExit || escapePressed)
		{
			running = false;
		}
		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->getGameState()->resizeWindow(width, height);
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		bool loadHotkeyPressed = (e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_l);
		bool newHotkeyPressed = (e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_s);
		bool exitHotkeyPressed = (e.type == SDL_KEYDOWN) && (e.key.keysym.sym == SDLK_e);

		bool loadClicked = leftClick && this->loadButton->containsPoint(mouseOriginalPoint);
		bool newClicked = leftClick && this->newButton->containsPoint(mouseOriginalPoint);
		bool exitClicked = leftClick && this->exitButton->containsPoint(mouseOriginalPoint);

		if (loadHotkeyPressed || loadClicked)
		{
			this->loadButton->click(this->getGameState());
		}
		else if (newHotkeyPressed || newClicked)
		{
			this->newButton->click(this->getGameState());
		}
		else if (exitHotkeyPressed || exitClicked)
		{
			this->exitButton->click(this->getGameState());
		}
	}
}

void MainMenuPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void MainMenuPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void MainMenuPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void MainMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw main menu.
	auto *mainMenu = textureManager.getTexture(
		TextureFile::fromName(TextureName::MainMenu), 
		PaletteFile::fromName(PaletteName::BuiltIn));
	renderer.drawToOriginal(mainMenu);

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	auto *cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor,
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor->w * this->getCursorScale()),
		static_cast<int>(cursor->h * this->getCursorScale()));
}
