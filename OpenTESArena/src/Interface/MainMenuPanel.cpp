#include <cassert>
#include <iostream>

#include "SDL.h"

#include "MainMenuPanel.h"

#include "Button.h"
#include "ChooseClassCreationPanel.h"
#include "CinematicPanel.h"
#include "LoadGamePanel.h"
#include "Surface.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Media/TextureSequenceName.h"

MainMenuPanel::MainMenuPanel(GameState *gameState)
	: Panel(gameState)
{
	this->loadButton = [gameState]()
	{
		auto center = Int2(168, 58);
		int width = 150;
		int height = 20;
		auto function = [gameState]()
		{
			auto loadPanel = std::unique_ptr<Panel>(
				new LoadGamePanel(gameState));
			gameState->setPanel(std::move(loadPanel));
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->newButton = [gameState]()
	{
		auto center = Int2(168, 112);
		int width = 150;
		int height = 20;
		auto function = [gameState]()
		{
			// Link together the opening scroll, intro cinematic, and character creation.
			auto changeToCharCreation = [gameState]()
			{
				auto creationPanel = std::unique_ptr<Panel>(
					new ChooseClassCreationPanel(gameState));
				gameState->setPanel(std::move(creationPanel));
				gameState->setMusic(MusicName::Sheet);
			};

			auto changeToNewGameStory = [gameState, changeToCharCreation]()
			{
				auto newGameStoryPanel = std::unique_ptr<Panel>(new CinematicPanel(
					gameState,
					TextureSequenceName::NewGameStory,
					5.0,
					changeToCharCreation));
				gameState->setPanel(std::move(newGameStoryPanel));
			};

			gameState->setPanel(std::unique_ptr<Panel>(new CinematicPanel(
				gameState,
				TextureSequenceName::OpeningScroll,
				CinematicPanel::DEFAULT_MOVIE_SECONDS_PER_IMAGE,
				changeToNewGameStory)));
			gameState->setMusic(MusicName::EvilIntro);
		};
		return std::unique_ptr<Button>(new Button(center, width, height, function));
	}();

	this->exitButton = []()
	{
		auto center = Int2(168, 158);
		int width = 45;
		int height = 20;
		auto function = []()
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
	auto mouseOriginalPoint = this->nativePointToOriginal(mousePosition);

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
			this->loadButton->click();
		}
		else if (newHotkeyPressed || newClicked)
		{
			this->newButton->click();
		}
		else if (exitHotkeyPressed || exitClicked)
		{
			this->exitButton->click();
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

void MainMenuPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw main menu.
	const auto &mainMenu = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::MainMenu));
	this->drawLetterbox(mainMenu, dst, letterbox);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
