#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "PauseMenuPanel.h"

#include "GameWorldPanel.h"
#include "LoadGamePanel.h"
#include "MainMenuPanel.h"
#include "OptionsPanel.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "../Entities/CharacterClass.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Vector2.h"
#include "../Media/AudioManager.h"
#include "../Media/Color.h"
#include "../Media/FontManager.h"
#include "../Media/FontName.h"
#include "../Media/MusicName.h"
#include "../Media/PaletteFile.h"
#include "../Media/PaletteName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Texture.h"

PauseMenuPanel::PauseMenuPanel(Game *game)
	: Panel(game)
{
	this->playerNameTextBox = [game]()
	{
		int x = 17;
		int y = 154;
		Color color(215, 121, 8);
		std::string text = game->getGameData().getPlayer().getFirstName();
		auto &font = game->getFontManager().getFont(FontName::Char);
		auto alignment = TextAlignment::Left;
		return std::unique_ptr<TextBox>(new TextBox(
			x,
			y,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->musicTextBox = [game]()
	{
		Int2 center(127, 96);
		Color color(12, 73, 16);
		std::string text = std::to_string(static_cast<int>(
			std::round(game->getOptions().getMusicVolume() * 100.0)));
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->soundTextBox = [game]()
	{
		Int2 center(54, 96);
		Color color(12, 73, 16);
		std::string text = std::to_string(static_cast<int>(
			std::round(game->getOptions().getSoundVolume() * 100.0)));
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->optionsTextBox = [game]()
	{
		Int2 center(234, 96);
		Color color(215, 158, 4);
		std::string text("OPTIONS");
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->optionsShadowTextBox = [this, game]()
	{
		Int2 center(233, 97);
		Color color(101, 77, 24);
		std::string text("OPTIONS");
		auto &font = game->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			game->getRenderer()));
	}();

	this->loadButton = []()
	{
		int x = 65;
		int y = 118;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> loadPanel(new LoadGamePanel(game));
			game->setPanel(std::move(loadPanel));
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 64, 29, function));
	}();

	this->exitButton = []()
	{
		int x = 193;
		int y = 118;
		auto function = [](Game *game)
		{
			SDL_Event evt;
			evt.quit.type = SDL_QUIT;
			evt.quit.timestamp = 0;
			SDL_PushEvent(&evt);
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 64, 29, function));
	}();

	this->newButton = []()
	{
		int x = 0;
		int y = 118;
		auto function = [](Game *game)
		{
			game->setGameData(nullptr);

			std::unique_ptr<Panel> mainMenuPanel(new MainMenuPanel(game));
			game->setPanel(std::move(mainMenuPanel));
			game->setMusic(MusicName::PercIntro);
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 65, 29, function));
	}();

	this->saveButton = []()
	{
		int x = 129;
		int y = 118;
		auto function = [](Game *game)
		{
			// SaveGamePanel...
			//std::unique_ptr<Panel> optionsPanel(new OptionsPanel(game));
			//game->setPanel(std::move(optionsPanel));
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 64, 29, function));
	}();

	this->resumeButton = []()
	{
		int x = 257;
		int y = 118;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(game));
			game->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 64, 29, function));
	}();

	this->optionsButton = []()
	{
		int x = 162;
		int y = 89;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> optionsPanel(new OptionsPanel(game));
			game->setPanel(std::move(optionsPanel));
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 145, 14, function));
	}();

	this->musicUpButton = [this]()
	{
		int x = 119;
		int y = 79;
		auto function = [this](Game *game)
		{
			Options &options = game->getOptions();
			options.setMusicVolume(std::min(options.getMusicVolume() + 0.050, 1.0));

			AudioManager &audioManager = game->getAudioManager();
			audioManager.setMusicVolume(options.getMusicVolume());

			// Update the music volume text.
			this->updateMusicText(options.getMusicVolume());
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 17, 9, function));
	}();

	this->musicDownButton = [this]()
	{
		int x = 119;
		int y = 104;
		auto function = [this](Game *game)
		{
			Options &options = game->getOptions();
			options.setMusicVolume(std::max(options.getMusicVolume() - 0.050, 0.0));

			AudioManager &audioManager = game->getAudioManager();
			audioManager.setMusicVolume(options.getMusicVolume());

			// Update the music volume text.
			this->updateMusicText(options.getMusicVolume());
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 17, 9, function));
	}();

	this->soundUpButton = [this]()
	{
		int x = 46;
		int y = 79;
		auto function = [this](Game *game)
		{
			Options &options = game->getOptions();
			options.setSoundVolume(std::min(options.getSoundVolume() + 0.050, 1.0));

			AudioManager &audioManager = game->getAudioManager();
			audioManager.setSoundVolume(options.getSoundVolume());

			// Update the sound volume text.
			this->updateSoundText(options.getSoundVolume());
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 17, 9, function));
	}();

	this->soundDownButton = [this]()
	{
		int x = 46;
		int y = 104;
		auto function = [this](Game *game)
		{
			Options &options = game->getOptions();
			options.setSoundVolume(std::max(options.getSoundVolume() - 0.050, 0.0));

			AudioManager &audioManager = game->getAudioManager();
			audioManager.setSoundVolume(options.getSoundVolume());

			// Update the sound volume text.
			this->updateSoundText(options.getSoundVolume());
		};
		return std::unique_ptr<Button<>>(new Button<>(x, y, 17, 9, function));
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
		auto &font = this->getGame()->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			this->getGame()->getRenderer()));
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
		auto &font = this->getGame()->getFontManager().getFont(FontName::Arena);
		auto alignment = TextAlignment::Center;
		return std::unique_ptr<TextBox>(new TextBox(
			center,
			color,
			text,
			font,
			alignment,
			this->getGame()->getRenderer()));
	}();
}

void PauseMenuPanel::handleEvent(const SDL_Event &e)
{
	bool escapePressed = (e.type == SDL_KEYDOWN) &&
		(e.key.keysym.sym == SDLK_ESCAPE);

	if (escapePressed)
	{
		this->resumeButton->click(this->getGame());
	}

	bool leftClick = (e.type == SDL_MOUSEBUTTONDOWN) &&
		(e.button.button == SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = this->getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		// See if any of the buttons are clicked.
		// (This code is getting kind of bad now. Maybe use a vector?)
		if (this->loadButton->contains(mouseOriginalPoint))
		{
			this->loadButton->click(this->getGame());
		}
		else if (this->exitButton->contains(mouseOriginalPoint))
		{
			this->exitButton->click(this->getGame());
		}
		else if (this->newButton->contains(mouseOriginalPoint))
		{
			this->newButton->click(this->getGame());
		}
		else if (this->saveButton->contains(mouseOriginalPoint))
		{
			this->saveButton->click(this->getGame());
		}
		else if (this->resumeButton->contains(mouseOriginalPoint))
		{
			this->resumeButton->click(this->getGame());
		}
		else if (this->optionsButton->contains(mouseOriginalPoint))
		{
			this->optionsButton->click(this->getGame());
		}
		else if (this->musicUpButton->contains(mouseOriginalPoint))
		{
			this->musicUpButton->click(this->getGame());
		}
		else if (this->musicDownButton->contains(mouseOriginalPoint))
		{
			this->musicDownButton->click(this->getGame());
		}
		else if (this->soundUpButton->contains(mouseOriginalPoint))
		{
			this->soundUpButton->click(this->getGame());
		}
		else if (this->soundDownButton->contains(mouseOriginalPoint))
		{
			this->soundDownButton->click(this->getGame());
		}
	}
}

void PauseMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clearNative();
	renderer.clearOriginal();

	// Set palette.
	auto &textureManager = this->getGame()->getTextureManager();
	textureManager.setPalette(PaletteFile::fromName(PaletteName::Default));

	// Draw pause background.
	const auto &pauseBackground = textureManager.getTexture(
		TextureFile::fromName(TextureName::PauseBackground));
	renderer.drawToOriginal(pauseBackground.get());

	// Draw game world interface below the pause menu.
	const auto &gameInterface = textureManager.getTexture(
		TextureFile::fromName(TextureName::GameWorldInterface));
	renderer.drawToOriginal(gameInterface.get(), 0,
		Renderer::ORIGINAL_HEIGHT - gameInterface.getHeight());

	// Draw player portrait.
	const auto &player = this->getGame()->getGameData().getPlayer();
	const auto &headsFilename = PortraitFile::getHeads(
		player.getGenderName(), player.getRaceName(), true);
	const auto &portrait = textureManager.getTextures(headsFilename)
		.at(player.getPortraitID());
	const auto &status = textureManager.getTextures(
		TextureFile::fromName(TextureName::StatusGradients)).at(0);
	renderer.drawToOriginal(status.get(), 14, 166);
	renderer.drawToOriginal(portrait.get(), 14, 166);

	// If the player's class can't use magic, show the darkened spell icon.
	if (!player.getCharacterClass().canCastMagic())
	{
		const auto &nonMagicIcon = textureManager.getTexture(
			TextureFile::fromName(TextureName::NoSpell));
		renderer.drawToOriginal(nonMagicIcon.get(), 91, 177);
	}

	// Cover up the detail slider with a new options background.
	Color optionsBackground(85, 85, 97);
	Color optionsLightBorder(125, 125, 145);
	Color optionsDarkBorder(40, 40, 48);
	renderer.fillOriginalRect(optionsBackground, 162, 89, 145, 14);
	renderer.drawOriginalLine(optionsLightBorder, 162, 89, 306, 89);
	renderer.drawOriginalLine(optionsLightBorder, 306, 89, 306, 102);
	renderer.drawOriginalLine(optionsDarkBorder, 162, 102, 305, 102);
	renderer.drawOriginalLine(optionsDarkBorder, 162, 90, 162, 102);
	renderer.drawOriginalPixel(optionsBackground, 162, 89);
	renderer.drawOriginalPixel(optionsBackground, 306, 102);

	// Draw text: player's name, music volume, sound volume, options.
	renderer.drawToOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawToOriginal(this->musicTextBox->getTexture(),
		this->musicTextBox->getX(), this->musicTextBox->getY());
	renderer.drawToOriginal(this->soundTextBox->getTexture(),
		this->soundTextBox->getX(), this->soundTextBox->getY());
	renderer.drawToOriginal(this->optionsShadowTextBox->getTexture(),
		this->optionsShadowTextBox->getX(), this->optionsShadowTextBox->getY());
	renderer.drawToOriginal(this->optionsTextBox->getTexture(),
		this->optionsTextBox->getX(), this->optionsTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();

	// Draw cursor.
	const auto &cursor = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor));
	auto mousePosition = this->getMousePosition();
	renderer.drawToNative(cursor.get(),
		mousePosition.x, mousePosition.y,
		static_cast<int>(cursor.getWidth() * this->getCursorScale()),
		static_cast<int>(cursor.getHeight() * this->getCursorScale()));
}
