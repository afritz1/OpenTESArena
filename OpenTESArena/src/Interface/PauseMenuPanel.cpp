#include <algorithm>
#include <cassert>
#include <cmath>

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
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Int2.h"
#include "../Media/AudioManager.h"
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

	this->musicTextBox = [gameState]()
	{
		Int2 center(127, 96);
		Color color(12, 73, 16);
		std::string text = std::to_string(static_cast<int>(
			std::round(gameState->getOptions().getMusicVolume() * 100.0)));
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			gameState->getTextureManager(),
			gameState->getRenderer()));
	}();

	this->soundTextBox = [gameState]()
	{
		Int2 center(54, 96);
		Color color(12, 73, 16);
		std::string text = std::to_string(static_cast<int>(
			std::round(gameState->getOptions().getSoundVolume() * 100.0)));
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
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

	this->musicUpButton = [this]()
	{
		int x = 119;
		int y = 79;
		auto function = [this](GameState *gameState)
		{
			Options &options = gameState->getOptions();
			options.setMusicVolume(std::min(options.getMusicVolume() + 0.050, 1.0));

			AudioManager &audioManager = gameState->getAudioManager();
			audioManager.setMusicVolume(options.getMusicVolume());

			// Update the music volume text.
			this->updateMusicText(options.getMusicVolume());
		};
		return std::unique_ptr<Button>(new Button(x, y, 17, 9, function));
	}();

	this->musicDownButton = [this]()
	{
		int x = 119;
		int y = 104;
		auto function = [this](GameState *gameState)
		{
			Options &options = gameState->getOptions();
			options.setMusicVolume(std::max(options.getMusicVolume() - 0.050, 0.0));

			AudioManager &audioManager = gameState->getAudioManager();
			audioManager.setMusicVolume(options.getMusicVolume());

			// Update the music volume text.
			this->updateMusicText(options.getMusicVolume());
		};
		return std::unique_ptr<Button>(new Button(x, y, 17, 9, function));
	}();

	this->soundUpButton = [this]()
	{
		int x = 46;
		int y = 79;
		auto function = [this](GameState *gameState)
		{
			Options &options = gameState->getOptions();
			options.setSoundVolume(std::min(options.getSoundVolume() + 0.050, 1.0));

			AudioManager &audioManager = gameState->getAudioManager();
			audioManager.setSoundVolume(options.getSoundVolume());

			// Update the sound volume text.
			this->updateSoundText(options.getSoundVolume());
		};
		return std::unique_ptr<Button>(new Button(x, y, 17, 9, function));
	}();

	this->soundDownButton = [this]()
	{
		int x = 46;
		int y = 104;
		auto function = [this](GameState *gameState)
		{
			Options &options = gameState->getOptions();
			options.setSoundVolume(std::max(options.getSoundVolume() - 0.050, 0.0));

			AudioManager &audioManager = gameState->getAudioManager();
			audioManager.setSoundVolume(options.getSoundVolume());

			// Update the sound volume text.
			this->updateSoundText(options.getSoundVolume());
		};
		return std::unique_ptr<Button>(new Button(x, y, 17, 9, function));
	}();
}

PauseMenuPanel::~PauseMenuPanel()
{

}

void PauseMenuPanel::updateMusicText(double volume)
{
	// Update the displayed music volume.
	this->musicTextBox = [this, volume]()
	{
		int displayedVolume = static_cast<int>(std::round(volume * 100.0));

		Int2 center(127, 96);
		Color color(12, 73, 16);
		std::string text = std::to_string(displayedVolume);
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			this->getGameState()->getTextureManager(),
			this->getGameState()->getRenderer()));
	}();
}

void PauseMenuPanel::updateSoundText(double volume)
{
	// Update the displayed sound volume.
	this->soundTextBox = [this, volume]()
	{
		int displayedVolume = static_cast<int>(std::round(volume * 100.0));

		Int2 center(54, 96);
		Color color(12, 73, 16);
		std::string text = std::to_string(displayedVolume);
		auto fontName = FontName::Arena;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			fontName,
			this->getGameState()->getTextureManager(),
			this->getGameState()->getRenderer()));
	}();
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
			// (This code is getting kind of bad now. Maybe use a vector?)
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
			else if (this->musicUpButton->containsPoint(mouseOriginalPoint))
			{
				this->musicUpButton->click(this->getGameState());
			}
			else if (this->musicDownButton->containsPoint(mouseOriginalPoint))
			{
				this->musicDownButton->click(this->getGameState());
			}
			else if (this->soundUpButton->containsPoint(mouseOriginalPoint))
			{
				this->soundUpButton->click(this->getGameState());
			}
			else if (this->soundDownButton->containsPoint(mouseOriginalPoint))
			{
				this->soundDownButton->click(this->getGameState());
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

	// Draw text: player's name, music volume, sound volume.
	renderer.drawToOriginal(this->playerNameTextBox->getSurface(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawToOriginal(this->musicTextBox->getSurface(),
		this->musicTextBox->getX(), this->musicTextBox->getY());
	renderer.drawToOriginal(this->soundTextBox->getSurface(),
		this->soundTextBox->getX(), this->soundTextBox->getY());

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
