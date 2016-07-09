#include <cassert>

#include "SDL.h"

#include "PauseMenuPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "LoadGamePanel.h"
#include "MainMenuPanel.h"
#include "OptionsPanel.h"
#include "TextBox.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"

PauseMenuPanel::PauseMenuPanel(GameState *gameState)
	: Panel(gameState)
{
	this->playerNameTextBox = [gameState]()
	{
		int x = 17;
		int y = 154;
		Color color(215, 121, 8);
		std::string text = gameState->getGameData()->getPlayer().getFirstName();
		auto fontName = FontName::Char;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			fontName,
			gameState->getTextureManager(),
			gameState->getRenderer()));
	}();

	this->loadButton = []()
	{
		int x = 65;
		int y = 118;
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> loadPanel(new LoadGamePanel(gameState));
			gameState->setPanel(std::move(loadPanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, 64, 29, function));
	}();

	this->exitButton = []()
	{
		int x = 193;
		int y = 118;
		auto function = [](GameState *gameState)
		{
			SDL_Event evt;
			evt.quit.type = SDL_QUIT;
			evt.quit.timestamp = 0;
			SDL_PushEvent(&evt);
		};
		return std::unique_ptr<Button>(new Button(x, y, 64, 29, function));
	}();

	this->newButton = []()
	{
		int x = 0;
		int y = 118;
		auto function = [](GameState *gameState)
		{
			gameState->setGameData(nullptr);

			std::unique_ptr<Panel> mainMenuPanel(new MainMenuPanel(gameState));
			gameState->setPanel(std::move(mainMenuPanel));
			gameState->setMusic(MusicName::PercIntro);
		};
		return std::unique_ptr<Button>(new Button(x, y, 65, 29, function));
	}();

	this->saveButton = []()
	{
		int x = 129;
		int y = 118;
		auto function = [](GameState *gameState)
		{
			// SaveGamePanel...
			//std::unique_ptr<Panel> optionsPanel(new OptionsPanel(gameState));
			//gameState->setPanel(std::move(optionsPanel));
		};
		return std::unique_ptr<Button>(new Button(x, y, 64, 29, function));
	}();

	this->resumeButton = []()
	{
		int x = 257;
		int y = 118;
		auto function = [](GameState *gameState)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(gameState));
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
	auto mouseOriginalPoint = this->getGameState()->getRenderer()
		.nativePointToOriginal(mousePosition);

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
			this->resumeButton->click(this->getGameState());
		}

		bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
			(e.button.button == SDL_BUTTON_LEFT);

		if (leftClick)
		{
			// See if any of the buttons are clicked.
			if (this->loadButton->containsPoint(mouseOriginalPoint))
			{
				this->loadButton->click(this->getGameState());
			}
			else if (this->exitButton->containsPoint(mouseOriginalPoint))
			{
				this->exitButton->click(this->getGameState());
			}
			else if (this->newButton->containsPoint(mouseOriginalPoint))
			{
				this->newButton->click(this->getGameState());
			}
			else if (this->saveButton->containsPoint(mouseOriginalPoint))
			{
				this->saveButton->click(this->getGameState());
			}
			else if (this->resumeButton->containsPoint(mouseOriginalPoint))
			{
				this->resumeButton->click(this->getGameState());
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

void PauseMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();

	// Set palette.
	auto &textureManager = this->getGameState()->getTextureManager();
	textureManager.setPalette(PaletteName::Default);

	// Draw pause background.
	auto *pauseBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::PauseBackground));
	renderer.drawToOriginal(pauseBackground);

	// Draw game world interface below the pause menu.
	auto *gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));
	int gameInterfaceHeight;
	SDL_QueryTexture(gameInterface, nullptr, nullptr, nullptr, &gameInterfaceHeight);
	renderer.drawToOriginal(gameInterface, 0, ORIGINAL_HEIGHT - gameInterfaceHeight);

	// Draw text: player's name.
	renderer.drawToOriginal(this->playerNameTextBox->getSurface(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getSurface(
		TextureFile::fromName(TextureName::SwordCursor));
	SDL_SetColorKey(cursor.getSurface(), SDL_TRUE,
		renderer.getFormattedARGB(Color::Black));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.getSurface(),
		mousePosition.getX(), mousePosition.getY(),
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
