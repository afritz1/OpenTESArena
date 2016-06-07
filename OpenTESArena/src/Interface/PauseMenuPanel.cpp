#include <cassert>
#include <iostream>

#include <SDL2/SDL.h>

#include "PauseMenuPanel.h"

#include "Button.h"
#include "GameWorldPanel.h"
#include "LoadGamePanel.h"
#include "MainMenuPanel.h"
#include "OptionsPanel.h"
#include "TextBox.h"
#include "../Game/GameData.h"
#include "../Game/GameState.h"
#include "../Math/Int2.h"
#include "../Media/Color.h"
#include "../Media/MusicName.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"

PauseMenuPanel::PauseMenuPanel(GameState *gameState)
	: Panel(gameState)
{
	this->titleTextBox = nullptr;
	this->loadTextBox = nullptr;
	this->mainMenuTextBox = nullptr;
	this->optionsTextBox = nullptr;
	this->resumeTextBox = nullptr;
	this->loadButton = nullptr;
	this->mainMenuButton = nullptr;
	this->optionsButton = nullptr;
	this->resumeButton = nullptr;

	this->titleTextBox = [gameState]()
	{
		auto center = Int2(160, 70);
		auto color = Color::White;
		std::string text = "Paused";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->loadTextBox = [gameState]()
	{
		auto center = Int2(215, 100);
		auto color = Color::White;
		std::string text = "Load Game";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->mainMenuTextBox = [gameState]()
	{
		auto center = Int2(215, 125);
		auto color = Color::White;
		std::string text = "Main Menu";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->optionsTextBox = [gameState]()
	{
		auto center = Int2(105, 125);
		auto color = Color::White;
		std::string text = "Options";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	this->resumeTextBox = [gameState]()
	{
		auto center = Int2(105, 100);
		auto color = Color::White;
		std::string text = "Resume";
		auto fontName = FontName::A;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager()));
	}();

	// Disable the load button for now until functionality is added in the GameState
	// that tells whether a game is active, and thus which panel to return to.
	this->loadButton = [gameState]()
	{
		auto center = Int2(215, 100);
		auto function = [gameState]()
		{
			//auto loadPanel = std::unique_ptr<Panel>(new LoadGamePanel(gameState));
			//gameState->setPanel(std::move(loadPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 100, 20, function));
	}();

	this->mainMenuButton = [gameState]()
	{
		auto center = Int2(215, 125);
		auto function = [gameState]()
		{
			gameState->setGameData(nullptr);

			auto mainMenuPanel = std::unique_ptr<Panel>(new MainMenuPanel(gameState));
			gameState->setMusic(MusicName::PercIntro);
			gameState->setPanel(std::move(mainMenuPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 100, 20, function));
	}();

	this->optionsButton = [gameState]()
	{
		auto center = Int2(105, 125);
		auto function = [gameState]()
		{
			// Change to options panel once that class is programmed.
			auto optionsPanel = std::unique_ptr<Panel>(new OptionsPanel(gameState));
			gameState->setPanel(std::move(optionsPanel));
		};
		return std::unique_ptr<Button>(new Button(center, 100, 20, function));
	}();

	this->resumeButton = [gameState]()
	{
		auto center = Int2(105, 100);
		auto function = [gameState]()
		{
			auto gamePanel = std::unique_ptr<Panel>(new GameWorldPanel(gameState));
			gameState->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button>(new Button(center, 100, 20, function));
	}();

	// Temporary button backgrounds.
	this->loadButton->fill(Color::Black);
	this->mainMenuButton->fill(Color::Black);
	this->optionsButton->fill(Color::Black);
	this->resumeButton->fill(Color::Black);

	assert(this->titleTextBox.get() != nullptr);
	assert(this->loadTextBox.get() != nullptr);
	assert(this->mainMenuTextBox.get() != nullptr);
	assert(this->resumeTextBox.get() != nullptr);
	assert(this->optionsTextBox.get() != nullptr);
	assert(this->loadButton.get() != nullptr);
	assert(this->mainMenuButton.get() != nullptr);
	assert(this->resumeButton.get() != nullptr);
	assert(this->optionsButton.get() != nullptr);
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
			else if (this->mainMenuButton->containsPoint(mouseOriginalPoint))
			{
				this->mainMenuButton->click();
			}
			else if (this->optionsButton->containsPoint(mouseOriginalPoint))
			{
				this->optionsButton->click();
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

	// Draw temporary background.
	SDL_FillRect(dst, letterbox, SDL_MapRGB(dst->format, 36, 48, 36));

	// Draw buttons: load, main menu, options, resume.
	this->drawScaledToNative(*this->loadButton.get(), dst);
	this->drawScaledToNative(*this->mainMenuButton.get(), dst);
	this->drawScaledToNative(*this->optionsButton.get(), dst);
	this->drawScaledToNative(*this->resumeButton.get(), dst);

	// Draw text: title, load, main menu, options, resume.
	this->drawScaledToNative(*this->titleTextBox.get(), dst);
	this->drawScaledToNative(*this->loadTextBox.get(), dst);
	this->drawScaledToNative(*this->mainMenuTextBox.get(), dst);
	this->drawScaledToNative(*this->optionsTextBox.get(), dst);
	this->drawScaledToNative(*this->resumeTextBox.get(), dst);

	// Draw cursor.
	const auto &cursor = this->getGameState()->getTextureManager()
		.getSurface(TextureFile::fromName(TextureName::SwordCursor));
	this->drawCursor(cursor, dst);
}
