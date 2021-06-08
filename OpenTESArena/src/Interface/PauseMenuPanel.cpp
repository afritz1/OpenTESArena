#include "SDL.h"

#include "GameWorldPanel.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "PauseMenuPanel.h"
#include "PauseMenuUiController.h"
#include "PauseMenuUiModel.h"
#include "PauseMenuUiView.h"
#include "../Game/Game.h"
#include "../Media/PortraitFile.h"

PauseMenuPanel::PauseMenuPanel(Game &game)
	: Panel(game) { }

bool PauseMenuPanel::init()
{
	auto &game = this->getGame();

	this->playerNameTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			GameWorldUiModel::getPlayerNameText(game),
			GameWorldUiView::PlayerNameFontName,
			GameWorldUiView::PlayerNameTextColor,
			GameWorldUiView::PlayerNameTextAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			GameWorldUiView::PlayerNameTextBoxX,
			GameWorldUiView::PlayerNameTextBoxY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->musicTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			PauseMenuUiModel::getMusicVolumeText(game),
			PauseMenuUiView::VolumeFontName,
			PauseMenuUiView::VolumeColor,
			PauseMenuUiView::VolumeTextAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			PauseMenuUiView::MusicTextBoxCenterPoint,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->soundTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			PauseMenuUiModel::getSoundVolumeText(game),
			PauseMenuUiView::VolumeFontName,
			PauseMenuUiView::VolumeColor,
			PauseMenuUiView::VolumeTextAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			PauseMenuUiView::SoundTextBoxCenterPoint,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

	this->optionsTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			PauseMenuUiModel::getOptionsButtonText(game),
			PauseMenuUiView::OptionsButtonFontName,
			PauseMenuUiView::OptionsButtonTextColor,
			PauseMenuUiView::OptionsButtonTextAlignment,
			fontLibrary);

		const TextBox::ShadowData shadowData(
			PauseMenuUiView::OptionsButtonTextShadowColor,
			PauseMenuUiView::OptionsButtonTextShadowOffset);
		return std::make_unique<TextBox>(
			PauseMenuUiView::OptionsTextBoxCenterPoint,
			richText,
			&shadowData,
			fontLibrary,
			game.getRenderer());
	}();

	this->newButton = Button<Game&>(
		PauseMenuUiView::NewGameButtonX,
		PauseMenuUiView::NewGameButtonY,
		PauseMenuUiView::NewGameButtonWidth,
		PauseMenuUiView::NewGameButtonHeight,
		PauseMenuUiController::onNewGameButtonSelected);
	this->loadButton = Button<Game&>(
		PauseMenuUiView::LoadButtonX,
		PauseMenuUiView::LoadButtonY,
		PauseMenuUiView::LoadButtonWidth,
		PauseMenuUiView::LoadButtonHeight,
		PauseMenuUiController::onLoadButtonSelected);
	this->saveButton = Button<Game&>(
		PauseMenuUiView::SaveButtonX,
		PauseMenuUiView::SaveButtonY,
		PauseMenuUiView::SaveButtonWidth,
		PauseMenuUiView::SaveButtonHeight,
		PauseMenuUiController::onSaveButtonSelected);
	this->exitButton = Button<Game&>(
		PauseMenuUiView::ExitButtonX,
		PauseMenuUiView::ExitButtonY,
		PauseMenuUiView::ExitButtonWidth,
		PauseMenuUiView::ExitButtonHeight,
		PauseMenuUiController::onExitButtonSelected);
	this->resumeButton = Button<Game&>(
		PauseMenuUiView::ResumeButtonX,
		PauseMenuUiView::ResumeButtonY,
		PauseMenuUiView::ResumeButtonWidth,
		PauseMenuUiView::ResumeButtonHeight,
		PauseMenuUiController::onResumeButtonSelected);
	this->optionsButton = Button<Game&>(
		PauseMenuUiView::OptionsButtonX,
		PauseMenuUiView::OptionsButtonY,
		PauseMenuUiView::OptionsButtonWidth,
		PauseMenuUiView::OptionsButtonHeight,
		PauseMenuUiController::onOptionsButtonSelected);
	this->soundUpButton = Button<Game&, PauseMenuPanel&>(
		PauseMenuUiView::SoundUpButtonX,
		PauseMenuUiView::SoundUpButtonY,
		PauseMenuUiView::SoundUpButtonWidth,
		PauseMenuUiView::SoundUpButtonHeight,
		PauseMenuUiController::onSoundUpButtonSelected);
	this->soundDownButton = Button<Game&, PauseMenuPanel&>(
		PauseMenuUiView::SoundDownButtonX,
		PauseMenuUiView::SoundDownButtonY,
		PauseMenuUiView::SoundDownButtonWidth,
		PauseMenuUiView::SoundDownButtonHeight,
		PauseMenuUiController::onSoundDownButtonSelected);
	this->musicUpButton = Button<Game&, PauseMenuPanel&>(
		PauseMenuUiView::MusicUpButtonX,
		PauseMenuUiView::MusicUpButtonY,
		PauseMenuUiView::MusicUpButtonWidth,
		PauseMenuUiView::MusicUpButtonHeight,
		PauseMenuUiController::onMusicUpButtonSelected);
	this->musicDownButton = Button<Game&, PauseMenuPanel&>(
		PauseMenuUiView::MusicDownButtonX,
		PauseMenuUiView::MusicDownButtonY,
		PauseMenuUiView::MusicDownButtonWidth,
		PauseMenuUiView::MusicDownButtonHeight,
		PauseMenuUiController::onMusicDownButtonSelected);

	// Cover up the detail slider with a new options background.
	this->optionsButtonTexture = TextureUtils::generate(
		PauseMenuUiView::OptionsButtonPatternType,
		this->optionsButton.getWidth(),
		this->optionsButton.getHeight(),
		game.getTextureManager(),
		game.getRenderer());

	return true;
}

void PauseMenuPanel::updateMusicText(double volume)
{
	// Update the displayed music volume.
	this->musicTextBox = [this, volume]()
	{
		// @todo: this should pull from PauseMenuUiModel
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
			PauseMenuUiView::MusicTextBoxCenterPoint,
			richText,
			fontLibrary,
			this->getGame().getRenderer());
	}();
}

void PauseMenuPanel::updateSoundText(double volume)
{
	// Update the displayed sound volume.
	this->soundTextBox = [this, volume]()
	{
		// @todo: this should pull from PauseMenuUiModel
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
			PauseMenuUiView::SoundTextBoxCenterPoint,
			richText,
			fontLibrary,
			this->getGame().getRenderer());
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
		auto &game = this->getGame();
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = game.getRenderer().nativeToOriginal(mousePosition);

		auto &options = game.getOptions();
		auto &audioManager = game.getAudioManager();

		// See if any of the buttons are clicked.
		// (This code is getting kind of bad now. Maybe use a vector?)
		if (this->loadButton.contains(mouseOriginalPoint))
		{
			this->loadButton.click(game);
		}
		else if (this->exitButton.contains(mouseOriginalPoint))
		{
			this->exitButton.click(game);
		}
		else if (this->newButton.contains(mouseOriginalPoint))
		{
			this->newButton.click(game);
		}
		else if (this->saveButton.contains(mouseOriginalPoint))
		{
			this->saveButton.click(game);
		}
		else if (this->resumeButton.contains(mouseOriginalPoint))
		{
			this->resumeButton.click(game);
		}
		else if (this->optionsButton.contains(mouseOriginalPoint))
		{
			this->optionsButton.click(game);
		}
		else if (this->musicUpButton.contains(mouseOriginalPoint))
		{
			this->musicUpButton.click(game, *this);
		}
		else if (this->musicDownButton.contains(mouseOriginalPoint))
		{
			this->musicDownButton.click(game, *this);
		}
		else if (this->soundUpButton.contains(mouseOriginalPoint))
		{
			this->soundUpButton.click(game, *this);
		}
		else if (this->soundDownButton.contains(mouseOriginalPoint))
		{
			this->soundDownButton.click(game, *this);
		}
	}
}

void PauseMenuPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw pause background.
	auto &game = this->getGame();
	auto &textureManager = game.getTextureManager();
	const TextureAssetReference backgroundPaletteTextureAssetRef = PauseMenuUiView::getBackgroundPaletteTextureAssetRef();
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundPaletteTextureAssetRef);
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get pause background palette ID for \"" + backgroundPaletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference backgroundTextureAssetRef = PauseMenuUiView::getBackgroundTextureAssetRef();
	const std::optional<TextureBuilderID> backgroundTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get pause background texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*backgroundTextureBuilderID, *backgroundPaletteID, textureManager);

	// Draw game world interface below the pause menu.
	const PaletteID gameWorldInterfacePaletteID = *backgroundPaletteID;
	const TextureBuilderID gameWorldInterfaceTextureBuilderID = GameWorldUiView::getGameWorldInterfaceTextureBuilderID(textureManager);
	const TextureBuilder &gameWorldInterfaceTextureBuilder = textureManager.getTextureBuilderHandle(gameWorldInterfaceTextureBuilderID);
	const Int2 gameWorldInterfacePosition = GameWorldUiView::getGameWorldInterfacePosition(gameWorldInterfaceTextureBuilder.getHeight());
	renderer.drawOriginal(gameWorldInterfaceTextureBuilderID, gameWorldInterfacePaletteID,
		gameWorldInterfacePosition.x, gameWorldInterfacePosition.y, textureManager);

	// Draw player portrait.
	const auto &player = game.getGameState().getPlayer();
	const PaletteID portraitPaletteID = gameWorldInterfacePaletteID;
	const std::string &headsFilename = PortraitFile::getHeads(player.isMale(), player.getRaceID(), true);
	const TextureBuilderID portraitTextureBuilderID =
		GameWorldUiView::getPlayerPortraitTextureBuilderID(game, headsFilename, player.getPortraitID());

	const PaletteID statusPaletteID = portraitPaletteID;
	const TextureBuilderID statusTextureBuilderID = GameWorldUiView::getStatusGradientTextureBuilderID(game, 0);

	renderer.drawOriginal(statusTextureBuilderID, statusPaletteID,
		GameWorldUiView::PlayerPortraitX, GameWorldUiView::PlayerPortraitY, textureManager);
	renderer.drawOriginal(portraitTextureBuilderID, portraitPaletteID,
		GameWorldUiView::PlayerPortraitX, GameWorldUiView::PlayerPortraitY, textureManager);

	// If the player's class can't use magic, show the darkened spell icon.
	const auto &charClassLibrary = game.getCharacterClassLibrary();
	const auto &charClassDef = charClassLibrary.getDefinition(player.getCharacterClassDefID());
	if (!charClassDef.canCastMagic())
	{
		const PaletteID nonMagicIconPaletteID = gameWorldInterfacePaletteID;
		const TextureBuilderID nonMagicIconTextureBuilderID = GameWorldUiView::getNoSpellTextureBuilderID(game);
		const Int2 nonMagicIconPosition = GameWorldUiView::getNoMagicTexturePosition();
		renderer.drawOriginal(nonMagicIconTextureBuilderID, nonMagicIconPaletteID,
			nonMagicIconPosition.x, nonMagicIconPosition.y, textureManager);
	}

	renderer.drawOriginal(this->optionsButtonTexture, this->optionsButton.getX(), this->optionsButton.getY());

	// Draw text: player's name, music volume, sound volume, options.
	renderer.drawOriginal(this->playerNameTextBox->getTexture(), this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
	renderer.drawOriginal(this->musicTextBox->getTexture(), this->musicTextBox->getX(), this->musicTextBox->getY());
	renderer.drawOriginal(this->soundTextBox->getTexture(), this->soundTextBox->getX(), this->soundTextBox->getY());
	renderer.drawOriginal(this->optionsTextBox->getTexture(), this->optionsTextBox->getX() - 1, this->optionsTextBox->getY());
}
