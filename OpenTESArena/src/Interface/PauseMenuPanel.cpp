#include "SDL.h"

#include "GameWorldPanel.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "PauseMenuPanel.h"
#include "PauseMenuUiController.h"
#include "PauseMenuUiModel.h"
#include "PauseMenuUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Media/PortraitFile.h"
#include "../UI/CursorData.h"
#include "../UI/TextRenderUtils.h"

PauseMenuPanel::PauseMenuPanel(Game &game)
	: Panel(game) { }

bool PauseMenuPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	const TextBox::InitInfo playerNameTextBoxInitInfo =
		GameWorldUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->playerNameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const std::string musicText = PauseMenuUiModel::getMusicVolumeText(game);
	const TextBox::InitInfo musicTextBoxInitInfo = PauseMenuUiView::getMusicTextBoxInitInfo(fontLibrary);
	if (!this->musicTextBox.init(musicTextBoxInitInfo, musicText, renderer))
	{
		DebugLogError("Couldn't init music volume text box.");
		return false;
	}

	const std::string soundText = PauseMenuUiModel::getSoundVolumeText(game);
	const TextBox::InitInfo soundTextBoxInitInfo = PauseMenuUiView::getSoundTextBoxInitInfo(fontLibrary);
	if (!this->soundTextBox.init(soundTextBoxInitInfo, soundText, renderer))
	{
		DebugLogError("Couldn't init sound volume text box.");
		return false;
	}

	const std::string optionsText = PauseMenuUiModel::getOptionsButtonText(game);
	const TextBox::InitInfo optionsTextBoxInitInfo = PauseMenuUiView::getOptionsTextBoxInitInfo(optionsText, fontLibrary);
	if (!this->optionsTextBox.init(optionsTextBoxInitInfo, optionsText, renderer))
	{
		DebugLogError("Couldn't init options button text box.");
		return false;
	}

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

	this->addButtonProxy(MouseButtonType::Left, this->newButton.getRect(),
		[this, &game]() { this->newButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->loadButton.getRect(),
		[this, &game]() { this->loadButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->saveButton.getRect(),
		[this, &game]() { this->saveButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->exitButton.getRect(),
		[this, &game]() { this->exitButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->resumeButton.getRect(),
		[this, &game]() { this->resumeButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->optionsButton.getRect(),
		[this, &game]() { this->optionsButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->soundUpButton.getRect(),
		[this, &game]() { this->soundUpButton.click(game, *this); });
	this->addButtonProxy(MouseButtonType::Left, this->soundDownButton.getRect(),
		[this, &game]() { this->soundDownButton.click(game, *this); });
	this->addButtonProxy(MouseButtonType::Left, this->musicUpButton.getRect(),
		[this, &game]() { this->musicUpButton.click(game, *this); });
	this->addButtonProxy(MouseButtonType::Left, this->musicDownButton.getRect(),
		[this, &game]() { this->musicDownButton.click(game, *this); });

	this->addInputActionListener(InputActionName::Back,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->resumeButton.click(game);
		}
	});

	// Cover up the detail slider with a new options background.
	this->optionsButtonTexture = TextureUtils::generate(
		PauseMenuUiView::OptionsButtonPatternType,
		this->optionsButton.getWidth(),
		this->optionsButton.getHeight(),
		game.getTextureManager(),
		renderer);

	return true;
}

void PauseMenuPanel::updateMusicText(double volume)
{
	const std::string volumeStr = PauseMenuUiModel::getVolumeString(volume);
	this->musicTextBox.setText(volumeStr);
}

void PauseMenuPanel::updateSoundText(double volume)
{
	const std::string volumeStr = PauseMenuUiModel::getVolumeString(volume);
	this->soundTextBox.setText(volumeStr);
}

std::optional<CursorData> PauseMenuPanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
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

	auto drawTextBox = [&renderer](TextBox &textBox, int xOffset)
	{
		const Rect &textBoxRect = textBox.getRect();
		renderer.drawOriginal(textBox.getTexture(), textBoxRect.getLeft() + xOffset, textBoxRect.getTop());
	};

	// Draw text: player's name, music volume, sound volume, options.
	drawTextBox(this->playerNameTextBox, 0);
	drawTextBox(this->musicTextBox, 0);
	drawTextBox(this->soundTextBox, 0);
	drawTextBox(this->optionsTextBox, -1);
}
