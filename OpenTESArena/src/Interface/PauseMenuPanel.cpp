#include <algorithm>
#include <cmath>

#include "SDL.h"

#include "CursorAlignment.h"
#include "GameWorldPanel.h"
#include "LoadSavePanel.h"
#include "MainMenuPanel.h"
#include "OptionsPanel.h"
#include "PauseMenuPanel.h"
#include "RichTextString.h"
#include "TextAlignment.h"
#include "TextBox.h"
#include "Texture.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/Player.h"
#include "../Game/GameData.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Constants.h"
#include "../Math/Vector2.h"
#include "../Media/AudioManager.h"
#include "../Media/Color.h"
#include "../Media/FontLibrary.h"
#include "../Media/FontName.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

PauseMenuPanel::PauseMenuPanel(Game &game)
	: Panel(game)
{
	this->playerNameTextBox = [&game]()
	{
		const int x = 17;
		const int y = 154;

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			game.getGameData().getPlayer().getFirstName(),
			FontName::Char,
			Color(215, 121, 8),
			TextAlignment::Left,
			fontLibrary);

		return std::make_unique<TextBox>(x, y, richText, fontLibrary, game.getRenderer());
	}();

	this->musicTextBox = [&game]()
	{
		const Int2 center(127, 96);

		const std::string text = std::to_string(static_cast<int>(
			std::round(game.getOptions().getAudio_MusicVolume() * 100.0)));

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::Arena,
			Color(12, 73, 16),
			TextAlignment::Center,
			fontLibrary);

		return std::make_unique<TextBox>(center, richText, fontLibrary, game.getRenderer());
	}();

	this->soundTextBox = [&game]()
	{
		const Int2 center(54, 96);

		const std::string text = std::to_string(static_cast<int>(
			std::round(game.getOptions().getAudio_SoundVolume() * 100.0)));

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			text,
			FontName::Arena,
			Color(12, 73, 16),
			TextAlignment::Center,
			fontLibrary);

		return std::make_unique<TextBox>(center, richText, fontLibrary, game.getRenderer());
	}();

	this->optionsTextBox = [&game]()
	{
		const Int2 center(234, 95);

		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			"OPTIONS",
			FontName::Arena,
			Color(215, 158, 4),
			TextAlignment::Center,
			fontLibrary);

		const TextBox::ShadowData shadowData(Color(101, 77, 24), Int2(-1, 1));
		return std::make_unique<TextBox>(
			center, richText, &shadowData, fontLibrary, game.getRenderer());
	}();

	this->loadButton = []()
	{
		const int x = 65;
		const int y = 118;
		auto function = [](Game &game)
		{
			game.setPanel<LoadSavePanel>(game, LoadSavePanel::Type::Load);
		};
		return Button<Game&>(x, y, 64, 29, function);
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
		return Button<>(x, y, 64, 29, function);
	}();

	this->newButton = []()
	{
		const int x = 0;
		const int y = 118;
		auto function = [](Game &game)
		{
			game.setGameData(nullptr);
			game.setPanel<MainMenuPanel>(game);

			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
				MusicDefinition::Type::MainMenu, game.getRandom());

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing main menu music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);
		};

		return Button<Game&>(x, y, 65, 29, function);
	}();

	this->saveButton = []()
	{
		const int x = 129;
		const int y = 118;
		auto function = [](Game &game)
		{
			// SaveGamePanel...
			//auto optionsPanel = std::make_unique<OptionsPanel>(game);
			//game.setPanel(std::move(optionsPanel));
		};
		return Button<Game&>(x, y, 64, 29, function);
	}();

	this->resumeButton = []()
	{
		const int x = 257;
		const int y = 118;
		auto function = [](Game &game)
		{
			game.setPanel<GameWorldPanel>(game);
		};
		return Button<Game&>(x, y, 64, 29, function);
	}();

	this->optionsButton = []()
	{
		const int x = 162;
		const int y = 88;
		auto function = [](Game &game)
		{
			game.setPanel<OptionsPanel>(game);
		};
		return Button<Game&>(x, y, 145, 15, function);
	}();

	this->musicUpButton = []()
	{
		const int x = 119;
		const int y = 79;
		auto function = [](Options &options, AudioManager &audioManager,
			PauseMenuPanel &panel)
		{
			options.setAudio_MusicVolume(std::min(options.getAudio_MusicVolume() + 0.050, 1.0));
			audioManager.setMusicVolume(options.getAudio_MusicVolume());

			// Update the music volume text.
			panel.updateMusicText(options.getAudio_MusicVolume());
		};
		return Button<Options&, AudioManager&, PauseMenuPanel&>(x, y, 17, 9, function);
	}();

	this->musicDownButton = []()
	{
		const int x = 119;
		const int y = 104;
		auto function = [](Options &options, AudioManager &audioManager,
			PauseMenuPanel &panel)
		{
			const double newVolume = [&options]()
			{
				const double volume = std::max(options.getAudio_MusicVolume() - 0.050, 0.0);

				// Clamp very small values to zero to avoid precision issues with tiny numbers.
				return volume < Constants::Epsilon ? 0.0 : volume;
			}();

			options.setAudio_MusicVolume(newVolume);
			audioManager.setMusicVolume(options.getAudio_MusicVolume());

			// Update the music volume text.
			panel.updateMusicText(options.getAudio_MusicVolume());
		};
		return Button<Options&, AudioManager&, PauseMenuPanel&>(x, y, 17, 9, function);
	}();

	this->soundUpButton = []()
	{
		const int x = 46;
		const int y = 79;
		auto function = [](Options &options, AudioManager &audioManager,
			PauseMenuPanel &panel)
		{
			options.setAudio_SoundVolume(std::min(options.getAudio_SoundVolume() + 0.050, 1.0));
			audioManager.setSoundVolume(options.getAudio_SoundVolume());

			// Update the sound volume text.
			panel.updateSoundText(options.getAudio_SoundVolume());
		};
		return Button<Options&, AudioManager&, PauseMenuPanel&>(x, y, 17, 9, function);
	}();

	this->soundDownButton = []()
	{
		const int x = 46;
		const int y = 104;
		auto function = [](Options &options, AudioManager &audioManager,
			PauseMenuPanel &panel)
		{
			const double newVolume = [&options]()
			{
				const double volume = std::max(options.getAudio_SoundVolume() - 0.050, 0.0);

				// Clamp very small values to zero to avoid precision issues with tiny numbers.
				return volume < Constants::Epsilon ? 0.0 : volume;
			}();

			options.setAudio_SoundVolume(newVolume);
			audioManager.setSoundVolume(options.getAudio_SoundVolume());

			// Update the sound volume text.
			panel.updateSoundText(options.getAudio_SoundVolume());
		};
		return Button<Options&, AudioManager&, PauseMenuPanel&>(x, y, 17, 9, function);
	}();
}

void PauseMenuPanel::updateMusicText(double volume)
{
	// Update the displayed music volume.
	this->musicTextBox = [this, volume]()
	{
		const Int2 center(127, 96);
		const int displayedVolume = static_cast<int>(std::round(volume * 100.0));

		const RichTextString &oldRichText = this->musicTextBox->getRichText();

		const auto &fontLibrary = this->getGame().getFontLibrary();
		const RichTextString richText(
			std::to_string(displayedVolume),
			oldRichText.getFontName(),
			oldRichText.getColor(),
			oldRichText.getAlignment(),
			fontLibrary);

		return std::make_unique<TextBox>(
			center, richText, fontLibrary, this->getGame().getRenderer());
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
		
		const auto &fontLibrary = this->getGame().getFontLibrary();
		const RichTextString richText(
			std::to_string(displayedVolume),
			oldRichText.getFontName(),
			oldRichText.getColor(),
			oldRichText.getAlignment(),
			fontLibrary);

		return std::make_unique<TextBox>(
			center, richText, fontLibrary, this->getGame().getRenderer());
	}();
}

std::optional<Panel::CursorData> PauseMenuPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void PauseMenuPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);

	if (escapePressed)
	{
		this->resumeButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		auto &options = this->getGame().getOptions();
		auto &audioManager = this->getGame().getAudioManager();

		// See if any of the buttons are clicked.
		// (This code is getting kind of bad now. Maybe use a vector?)
		if (this->loadButton.contains(mouseOriginalPoint))
		{
			this->loadButton.click(this->getGame());
		}
		else if (this->exitButton.contains(mouseOriginalPoint))
		{
			this->exitButton.click();
		}
		else if (this->newButton.contains(mouseOriginalPoint))
		{
			this->newButton.click(this->getGame());
		}
		else if (this->saveButton.contains(mouseOriginalPoint))
		{
			this->saveButton.click(this->getGame());
		}
		else if (this->resumeButton.contains(mouseOriginalPoint))
		{
			this->resumeButton.click(this->getGame());
		}
		else if (this->optionsButton.contains(mouseOriginalPoint))
		{
			this->optionsButton.click(this->getGame());
		}
		else if (this->musicUpButton.contains(mouseOriginalPoint))
		{
			this->musicUpButton.click(options, audioManager, *this);
		}
		else if (this->musicDownButton.contains(mouseOriginalPoint))
		{
			this->musicDownButton.click(options, audioManager, *this);
		}
		else if (this->soundUpButton.contains(mouseOriginalPoint))
		{
			this->soundUpButton.click(options, audioManager, *this);
		}
		else if (this->soundDownButton.contains(mouseOriginalPoint))
		{
			this->soundDownButton.click(options, audioManager, *this);
		}
	}
}

void PauseMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw pause background.
	auto &textureManager = this->getGame().getTextureManager();
	const std::string &pauseBackgroundPaletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> pauseBackgroundPaletteID =
		textureManager.tryGetPaletteID(pauseBackgroundPaletteFilename.c_str());
	if (!pauseBackgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get pause background palette ID for \"" + pauseBackgroundPaletteFilename + "\".");
		return;
	}

	const std::string &pauseBackgroundTextureFilename = ArenaTextureName::PauseBackground;
	const std::optional<TextureBuilderID> pauseBackgroundTextureBuilderID =
		textureManager.tryGetTextureBuilderID(pauseBackgroundTextureFilename.c_str());
	if (!pauseBackgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get pause background texture builder ID for \"" + pauseBackgroundTextureFilename + "\".");
		return;
	}

	renderer.drawOriginal(*pauseBackgroundTextureBuilderID, *pauseBackgroundPaletteID, textureManager);

	// Draw game world interface below the pause menu.
	const PaletteID gameWorldInterfacePaletteID = *pauseBackgroundPaletteID;
	const std::string &gameWorldInterfaceTextureFilename = ArenaTextureName::GameWorldInterface;
	const std::optional<TextureBuilderID> gameWorldInterfaceTextureBuilderID =
		textureManager.tryGetTextureBuilderID(gameWorldInterfaceTextureFilename.c_str());
	if (!gameWorldInterfaceTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get game world interface texture builder ID for \"" + gameWorldInterfaceTextureFilename + "\".");
		return;
	}

	const TextureBuilder &gameWorldInterfaceTextureBuilder =
		textureManager.getTextureBuilderHandle(*gameWorldInterfaceTextureBuilderID);
	renderer.drawOriginal(*gameWorldInterfaceTextureBuilderID, gameWorldInterfacePaletteID,
		0, ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilder.getHeight(), textureManager);

	// Draw player portrait.
	const auto &player = this->getGame().getGameData().getPlayer();
	const PaletteID portraitPaletteID = gameWorldInterfacePaletteID;
	const TextureBuilderID portraitTextureBuilderID = [this, &textureManager, &player]()
	{
		const std::string &headsFilename = PortraitFile::getHeads(player.isMale(), player.getRaceID(), true);
		const std::optional<TextureBuilderIdGroup> portraitTextureBuilderIDs =
			textureManager.tryGetTextureBuilderIDs(headsFilename.c_str());
		if (!portraitTextureBuilderIDs.has_value())
		{
			DebugCrash("Couldn't get portrait texture builder IDs for \"" + headsFilename + "\".");
		}

		return portraitTextureBuilderIDs->getID(player.getPortraitID());
	}();

	const PaletteID statusPaletteID = portraitPaletteID;
	const TextureBuilderID statusTextureBuilderID = [this, &textureManager]()
	{
		const std::string &statusFilename = ArenaTextureName::StatusGradients;
		const std::optional<TextureBuilderIdGroup> statusTextureBuilderIDs =
			textureManager.tryGetTextureBuilderIDs(statusFilename.c_str());
		if (!statusTextureBuilderIDs.has_value())
		{
			DebugCrash("Couldn't get status texture builder IDs for \"" + statusFilename + "\".");
		}

		return statusTextureBuilderIDs->getID(0);
	}();

	renderer.drawOriginal(statusTextureBuilderID, statusPaletteID, 14, 166, textureManager);
	renderer.drawOriginal(portraitTextureBuilderID, portraitPaletteID, 14, 166, textureManager);

	// If the player's class can't use magic, show the darkened spell icon.
	const auto &charClassLibrary = this->getGame().getCharacterClassLibrary();
	const auto &charClassDef = charClassLibrary.getDefinition(player.getCharacterClassDefID());
	if (!charClassDef.canCastMagic())
	{
		const PaletteID nonMagicIconPaletteID = gameWorldInterfacePaletteID;
		const std::string &nonMagicIconTextureFilename = ArenaTextureName::NoSpell;
		const std::optional<TextureBuilderID> nonMagicIconTextureBuilderID =
			textureManager.tryGetTextureBuilderID(nonMagicIconTextureFilename.c_str());
		if (!nonMagicIconTextureBuilderID.has_value())
		{
			DebugCrash("Couldn't get non-magic icon texture builder ID for \"" + nonMagicIconTextureFilename + "\".");
		}

		renderer.drawOriginal(*nonMagicIconTextureBuilderID, nonMagicIconPaletteID, 91, 177, textureManager);
	}

	// Cover up the detail slider with a new options background.
	Texture optionsBackground = TextureUtils::generate(TextureUtils::PatternType::Custom1,
		this->optionsButton.getWidth(), this->optionsButton.getHeight(), textureManager, renderer);
	renderer.drawOriginal(optionsBackground, this->optionsButton.getX(), this->optionsButton.getY());

	// Draw text: player's name, music volume, sound volume, options.
	renderer.drawOriginal(this->playerNameTextBox->getTexture(), this->playerNameTextBox->getX(),
		this->playerNameTextBox->getY());
	renderer.drawOriginal(this->musicTextBox->getTexture(), this->musicTextBox->getX(),
		this->musicTextBox->getY());
	renderer.drawOriginal(this->soundTextBox->getTexture(), this->soundTextBox->getX(),
		this->soundTextBox->getY());
	renderer.drawOriginal(this->optionsTextBox->getTexture(), this->optionsTextBox->getX() - 1,
		this->optionsTextBox->getY());
}
