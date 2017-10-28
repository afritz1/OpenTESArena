#include <algorithm>
#include <cassert>
#include <cmath>

#include "SDL.h"

#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "LoadGamePanel.h"
#include "MainMenuPanel.h"
#include "OptionsPanel.h"
#include "PauseMenuPanel.h"
#include "RichTextString.h"
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
		const int x = 17;
		const int y = 154;

		const RichTextString richText(
			game->getGameData().getPlayer().getFirstName(),
			FontName::Char,
			Color(215, 121, 8),
			TextAlignment::Left,
			game->getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			x, y, richText, game->getRenderer()));
	}();

	this->musicTextBox = [game]()
	{
		const Int2 center(127, 96);

		const std::string text = std::to_string(static_cast<int>(
			std::round(game->getOptions().getMusicVolume() * 100.0)));

		const RichTextString richText(
			text,
			FontName::Arena,
			Color(12, 73, 16),
			TextAlignment::Center,
			game->getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			center, richText, game->getRenderer()));
	}();

	this->soundTextBox = [game]()
	{
		const Int2 center(54, 96);

		const std::string text = std::to_string(static_cast<int>(
			std::round(game->getOptions().getSoundVolume() * 100.0)));

		const RichTextString richText(
			text,
			FontName::Arena,
			Color(12, 73, 16),
			TextAlignment::Center,
			game->getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			center, richText, game->getRenderer()));
	}();

	this->optionsTextBox = [game]()
	{
		const Int2 center(234, 96);

		const RichTextString richText(
			"OPTIONS",
			FontName::Arena,
			Color(215, 158, 4),
			TextAlignment::Center,
			game->getFontManager());

		const Color shadowColor(101, 77, 24);

		return std::unique_ptr<TextBox>(new TextBox(
			center, richText, shadowColor, game->getRenderer()));
	}();

	this->loadButton = []()
	{
		const int x = 65;
		const int y = 118;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> loadPanel(new LoadGamePanel(game));
			game->setPanel(std::move(loadPanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(x, y, 64, 29, function));
	}();

	this->exitButton = []()
	{
		const int x = 193;
		const int y = 118;
		auto function = []()
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
		const int x = 0;
		const int y = 118;
		auto function = [](Game *game)
		{
			game->setGameData(nullptr);

			std::unique_ptr<Panel> mainMenuPanel(new MainMenuPanel(game));
			game->setPanel(std::move(mainMenuPanel));
			game->setMusic(MusicName::PercIntro);
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(x, y, 65, 29, function));
	}();

	this->saveButton = []()
	{
		const int x = 129;
		const int y = 118;
		auto function = [](Game *game)
		{
			// SaveGamePanel...
			//std::unique_ptr<Panel> optionsPanel(new OptionsPanel(game));
			//game->setPanel(std::move(optionsPanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(x, y, 64, 29, function));
	}();

	this->resumeButton = []()
	{
		const int x = 257;
		const int y = 118;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> gamePanel(new GameWorldPanel(game));
			game->setPanel(std::move(gamePanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(x, y, 64, 29, function));
	}();

	this->optionsButton = []()
	{
		const int x = 162;
		const int y = 89;
		auto function = [](Game *game)
		{
			std::unique_ptr<Panel> optionsPanel(new OptionsPanel(game));
			game->setPanel(std::move(optionsPanel));
		};
		return std::unique_ptr<Button<Game*>>(
			new Button<Game*>(x, y, 145, 14, function));
	}();

	this->musicUpButton = []()
	{
		const int x = 119;
		const int y = 79;
		auto function = [](Options &options, AudioManager &audioManager,
			PauseMenuPanel *panel)
		{
			options.setMusicVolume(std::min(options.getMusicVolume() + 0.050, 1.0));
			audioManager.setMusicVolume(options.getMusicVolume());

			// Update the music volume text.
			panel->updateMusicText(options.getMusicVolume());
		};
		return std::unique_ptr<Button<Options&, AudioManager&, PauseMenuPanel*>>(
			new Button<Options&, AudioManager&, PauseMenuPanel*>(x, y, 17, 9, function));
	}();

	this->musicDownButton = []()
	{
		const int x = 119;
		const int y = 104;
		auto function = [](Options &options, AudioManager &audioManager,
			PauseMenuPanel *panel)
		{
			options.setMusicVolume(std::max(options.getMusicVolume() - 0.050, 0.0));
			audioManager.setMusicVolume(options.getMusicVolume());

			// Update the music volume text.
			panel->updateMusicText(options.getMusicVolume());
		};
		return std::unique_ptr<Button<Options&, AudioManager&, PauseMenuPanel*>>(
			new Button<Options&, AudioManager&, PauseMenuPanel*>(x, y, 17, 9, function));
	}();

	this->soundUpButton = []()
	{
		const int x = 46;
		const int y = 79;
		auto function = [](Options &options, AudioManager &audioManager,
			PauseMenuPanel *panel)
		{
			options.setSoundVolume(std::min(options.getSoundVolume() + 0.050, 1.0));
			audioManager.setSoundVolume(options.getSoundVolume());

			// Update the sound volume text.
			panel->updateSoundText(options.getSoundVolume());
		};
		return std::unique_ptr<Button<Options&, AudioManager&, PauseMenuPanel*>>(
			new Button<Options&, AudioManager&, PauseMenuPanel*>(x, y, 17, 9, function));
	}();

	this->soundDownButton = []()
	{
		const int x = 46;
		const int y = 104;
		auto function = [](Options &options, AudioManager &audioManager,
			PauseMenuPanel *panel)
		{
			options.setSoundVolume(std::max(options.getSoundVolume() - 0.050, 0.0));
			audioManager.setSoundVolume(options.getSoundVolume());

			// Update the sound volume text.
			panel->updateSoundText(options.getSoundVolume());
		};
		return std::unique_ptr<Button<Options&, AudioManager&, PauseMenuPanel*>>(
			new Button<Options&, AudioManager&, PauseMenuPanel*>(x, y, 17, 9, function));
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
		const Int2 center(127, 96);
		const int displayedVolume = static_cast<int>(std::round(volume * 100.0));

		const RichTextString &oldRichText = this->musicTextBox->getRichText();

		const RichTextString richText(
			std::to_string(displayedVolume),
			oldRichText.getFontName(),
			oldRichText.getColor(),
			oldRichText.getAlignment(),
			this->getGame()->getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			center, richText, this->getGame()->getRenderer()));
	}();
}

void PauseMenuPanel::updateSoundText(double volume)
{
	// Update the displayed sound volume.
	this->soundTextBox = [this, volume]()
	{
		const Int2 center(54, 96);
		const int displayedVolume = static_cast<int>(std::round(volume * 100.0));

		const RichTextString &oldRichText = this->soundTextBox->getRichText();
		
		const RichTextString richText(
			std::to_string(displayedVolume),
			oldRichText.getFontName(),
			oldRichText.getColor(),
			oldRichText.getAlignment(),
			this->getGame()->getFontManager());

		return std::unique_ptr<TextBox>(new TextBox(
			center, richText, this->getGame()->getRenderer()));
	}();
}

std::pair<SDL_Texture*, CursorAlignment> PauseMenuPanel::getCurrentCursor() const
{
	auto &textureManager = this->getGame()->getTextureManager();
	const auto &texture = textureManager.getTexture(
		TextureFile::fromName(TextureName::SwordCursor),
		PaletteFile::fromName(PaletteName::Default));
	return std::make_pair(texture.get(), CursorAlignment::TopLeft);
}

void PauseMenuPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame()->getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->resumeButton->click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame()->getRenderer()
			.nativePointToOriginal(mousePosition);

		auto &options = this->getGame()->getOptions();
		auto &audioManager = this->getGame()->getAudioManager();

		// See if any of the buttons are clicked.
		// (This code is getting kind of bad now. Maybe use a vector?)
		if (this->loadButton->contains(mouseOriginalPoint))
		{
			this->loadButton->click(this->getGame());
		}
		else if (this->exitButton->contains(mouseOriginalPoint))
		{
			this->exitButton->click();
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
			this->musicUpButton->click(options, audioManager, this);
		}
		else if (this->musicDownButton->contains(mouseOriginalPoint))
		{
			this->musicDownButton->click(options, audioManager, this);
		}
		else if (this->soundUpButton->contains(mouseOriginalPoint))
		{
			this->soundUpButton->click(options, audioManager, this);
		}
		else if (this->soundDownButton->contains(mouseOriginalPoint))
		{
			this->soundDownButton->click(options, audioManager, this);
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
		player.getGenderName(), player.getRaceID(), true);
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
	Texture optionsBackground(Texture::generate(Texture::PatternType::Custom1,
		this->optionsButton->getWidth(), this->optionsButton->getHeight(),
		textureManager, renderer));
	renderer.drawToOriginal(optionsBackground.get(), this->optionsButton->getX(),
		this->optionsButton->getY());

	// Draw text: player's name, music volume, sound volume, options.
	renderer.drawToOriginal(this->playerNameTextBox->getTexture(),
		this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawToOriginal(this->musicTextBox->getTexture(),
		this->musicTextBox->getX(), this->musicTextBox->getY());
	renderer.drawToOriginal(this->soundTextBox->getTexture(),
		this->soundTextBox->getX(), this->soundTextBox->getY());
	renderer.drawToOriginal(this->optionsTextBox->getShadowTexture(),
		this->optionsTextBox->getX() - 1, this->optionsTextBox->getY() + 1);
	renderer.drawToOriginal(this->optionsTextBox->getTexture(),
		this->optionsTextBox->getX(), this->optionsTextBox->getY());

	// Scale the original frame buffer onto the native one.
	renderer.drawOriginalToNative();
}
