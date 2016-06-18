#include <cassert>
#include <iostream>

#include "SDL.h"

#include "PauseMenuPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "LoadGamePanel.h"
#include "MainMenuPanel.h"
#include "OptionsPanel.h"
#include "TextBox.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

PauseMenuPanel::PauseMenuPanel(GameState *gameState)
	: Panel(gameState)
{
	this->loadButton = [gameState]()
	{
		int x = 65;
		int y = 118;
		auto function = [gameState]()
		{
			auto loadPanel = std::unique_ptr<Panel>(new LoadGamePanel(gameState));
			gameState->setPanel(std::move(loadPanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, 64, 29, function));
	}();

	this->exitButton = [gameState]()
	{
		int x = 193;
		int y = 118;
		auto function = [gameState]()
		{
			SDL_Event evt;
			evt.quit.type = SDL_QUIT;
			evt.quit.timestamp = 0;
			SDL_PushEvent(&evt);
		};
		return std::unique_ptr<Button>(new Button(x, y, 64, 29, function));
	}();

	this->newButton = [gameState]()
	{
		int x = 0;
		int y = 118;
		auto function = [gameState]()
		{
			gameState->setGameData(nullptr);

			auto mainMenuPanel = std::unique_ptr<Panel>(new MainMenuPanel(gameState));
			gameState->setMusic(MusicName::PercIntro);
			gameState->setPanel(std::move(mainMenuPanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, 65, 29, function));
	}();

	this->saveButton = [gameState]()
	{
		int x = 129;
		int y = 118;
		auto function = [gameState]()
		{
			// SaveGamePanel...
			//auto optionsPanel = std::unique_ptr<Panel>(new OptionsPanel(gameState));
			//gameState->setPanel(std::move(optionsPanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, 64, 29, function));
	}();

	this->resumeButton = [gameState]()
	{
		int x = 257;
		int y = 118;
		auto function = [gameState]()
		{
			auto gamePanel = std::unique_ptr<Panel>(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, 64, 29, function));
	}();
}

PauseMenuPanel::~PauseMenuPanel()
{

}

void PauseMenuPanel::handleEvents(bool &running)
{
	auto mousePosition = this->getMousePosition();
	auto mouseOriginalPoint = this->nativePointToOriginal(mousePosition);

	SDL_Event e;
	while (SDL_PollEvent(&e) != 0)
	{
		bool applicationExit = (e.type == SDL_QUIT);
		bool resized = (e.type == SDL_WINDOWEVENT) &&
			(e.window.event == SDL_WINDOWEVENT_RESIZED);
		bool escapePressed = (e.type == SDL_KEYDOWN) &&
			(e.key.keysym.sym == SDLK_ESCAPE);

		if (applicationExit)
		{
			running = false;
		}
		if (resized)
		{
			int width = e.window.data1;
			int height = e.window.data2;
			this->getGameState()->resizeWindow(width, height);
		}
		if (escapePressed)
		{
			this->resumeButton->click();
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			// See if any of the buttons are clicked.
			if (this->loadButton->containsPoint(mouseOriginalPoint))
			{
				this->loadButton->click();
			}
			else if (this->exitButton->containsPoint(mouseOriginalPoint))
			{
				this->exitButton->click();
			}
			else if (this->newButton->containsPoint(mouseOriginalPoint))
			{
				this->newButton->click();
			}
			else if (this->saveButton->containsPoint(mouseOriginalPoint))
			{
				this->saveButton->click();
			}
			else if (this->resumeButton->containsPoint(mouseOriginalPoint))
			{
				this->resumeButton->click();
			}
		}
	}
}

void PauseMenuPanel::handleMouse(double dt)
{
	static_cast<void>(dt);
}

void PauseMenuPanel::handleKeyboard(double dt)
{
	static_cast<void>(dt);
}

void PauseMenuPanel::tick(double dt, bool &running)
{
	static_cast<void>(dt);

	this->handleEvents(running);
}

void PauseMenuPanel::render(SDL_Surface *dst, const SDL_Rect *letterbox)
{
	// Clear full screen.
	this->clearScreen(dst);

	// Draw pause background.
	const auto &pauseBackground = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::PauseBackground));
	this->drawScaledToNative(pauseBackground, dst);

	// Draw game world interface below the pause menu.
	const auto &gameInterface = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::GameWorldInterface));
	this->drawScaledToNative(gameInterface,
		(ORIGINAL_WIDTH / 2) - (gameInterface.getWidth() / 2),
		ORIGINAL_HEIGHT - gameInterface.getHeight(),
		gameInterface.getWidth(),
		gameInterface.getHeight(),
		dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
