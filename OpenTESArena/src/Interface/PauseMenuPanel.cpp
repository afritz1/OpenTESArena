#include "CommonUiView.h"
#include "GameWorldPanel.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "PauseMenuPanel.h"
#include "PauseMenuUiController.h"
#include "PauseMenuUiModel.h"
#include "PauseMenuUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/CursorData.h"
#include "../UI/Surface.h"
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

	this->newButton = Button<Game&>(PauseMenuUiView::getNewGameButtonRect(), PauseMenuUiController::onNewGameButtonSelected);
	this->loadButton = Button<Game&>(PauseMenuUiView::getLoadButtonRect(), PauseMenuUiController::onLoadButtonSelected);
	this->saveButton = Button<Game&>(PauseMenuUiView::getSaveButtonRect(), PauseMenuUiController::onSaveButtonSelected);
	this->exitButton = Button<Game&>(PauseMenuUiView::getExitButtonRect(), PauseMenuUiController::onExitButtonSelected);
	this->resumeButton = Button<Game&>(PauseMenuUiView::getResumeButtonRect(), PauseMenuUiController::onResumeButtonSelected);
	this->optionsButton = Button<Game&>(PauseMenuUiView::getOptionsButtonRect(), PauseMenuUiController::onOptionsButtonSelected);
	this->soundUpButton = Button<Game&, PauseMenuPanel&>(
		PauseMenuUiView::getSoundUpButtonRect(), PauseMenuUiController::onSoundUpButtonSelected);
	this->soundDownButton = Button<Game&, PauseMenuPanel&>(
		PauseMenuUiView::getSoundDownButtonRect(), PauseMenuUiController::onSoundDownButtonSelected);
	this->musicUpButton = Button<Game&, PauseMenuPanel&>(
		PauseMenuUiView::getMusicUpButtonRect(), PauseMenuUiController::onMusicUpButtonSelected);
	this->musicDownButton = Button<Game&, PauseMenuPanel&>(
		PauseMenuUiView::getMusicDownButtonRect(), PauseMenuUiController::onMusicDownButtonSelected);

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

	auto &textureManager = game.getTextureManager();
	const UiTextureID backgroundTextureID = PauseMenuUiView::allocBackgroundTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);
	this->addDrawCall(
		this->backgroundTextureRef.get(),
		Int2::Zero,
		Int2(this->backgroundTextureRef.getWidth(), this->backgroundTextureRef.getHeight()),
		PivotType::TopLeft);

	const UiTextureID gameWorldInterfaceTextureID =
		GameWorldUiView::allocGameWorldInterfaceTexture(textureManager, renderer);
	this->gameWorldInterfaceTextureRef.init(gameWorldInterfaceTextureID, renderer);

	this->addDrawCall(
		this->gameWorldInterfaceTextureRef.get(),
		GameWorldUiView::getGameWorldInterfacePosition(),
		Int2(this->gameWorldInterfaceTextureRef.getWidth(), this->gameWorldInterfaceTextureRef.getHeight()),
		PivotType::Bottom);

	constexpr GameWorldUiView::StatusGradientType gradientType = GameWorldUiView::StatusGradientType::Default;
	const UiTextureID statusGradientTextureID =
		GameWorldUiView::allocStatusGradientTexture(gradientType, textureManager, renderer);
	this->statusGradientTextureRef.init(statusGradientTextureID, renderer);

	const Rect portraitRect = GameWorldUiView::getPlayerPortraitRect();
	this->addDrawCall(
		this->statusGradientTextureRef.get(),
		portraitRect.getTopLeft(),
		Int2(this->statusGradientTextureRef.getWidth(), this->statusGradientTextureRef.getHeight()),
		PivotType::TopLeft);

	const auto &player = game.getPlayer();
	const UiTextureID playerPortraitTextureID = GameWorldUiView::allocPlayerPortraitTexture(
		player.isMale(), player.getRaceID(), player.getPortraitID(), textureManager, renderer);
	this->playerPortraitTextureRef.init(playerPortraitTextureID, renderer);
	this->addDrawCall(
		this->playerPortraitTextureRef.get(),
		portraitRect.getTopLeft(),
		Int2(this->playerPortraitTextureRef.getWidth(), this->playerPortraitTextureRef.getHeight()),
		PivotType::TopLeft);

	const UiTextureID noMagicTextureID = GameWorldUiView::allocNoMagicTexture(textureManager, renderer);
	this->noMagicTextureRef.init(noMagicTextureID, renderer);

	const auto &charClassLibrary = game.getCharacterClassLibrary();
	const auto &charClassDef = charClassLibrary.getDefinition(player.getCharacterClassDefID());
	if (!charClassDef.canCastMagic())
	{
		this->addDrawCall(
			this->noMagicTextureRef.get(),
			GameWorldUiView::getNoMagicTexturePosition(),
			Int2(this->noMagicTextureRef.getWidth(), this->noMagicTextureRef.getHeight()),
			PivotType::TopLeft);
	}

	// Cover up the detail slider with a new options background.
	const UiTextureID optionsButtonTextureID = PauseMenuUiView::allocOptionsButtonTexture(textureManager, renderer);
	this->optionsButtonTextureRef.init(optionsButtonTextureID, renderer);

	const Rect &optionsButtonRect = this->optionsButton.getRect();
	this->addDrawCall(
		this->optionsButtonTextureRef.get(),
		optionsButtonRect.getTopLeft(),
		Int2(optionsButtonRect.getWidth(), optionsButtonRect.getHeight()),
		PivotType::TopLeft);

	const Rect &playerNameRect = this->playerNameTextBox.getRect();
	this->addDrawCall(
		this->playerNameTextBox.getTextureID(),
		playerNameRect.getTopLeft(),
		Int2(playerNameRect.getWidth(), playerNameRect.getHeight()),
		PivotType::TopLeft);

	const Rect &musicVolumeRect = this->musicTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->musicTextBox.getTextureID(); },
		musicVolumeRect.getCenter(),
		Int2(musicVolumeRect.getWidth(), musicVolumeRect.getHeight()),
		PivotType::Middle);

	const Rect &soundVolumeRect = this->soundTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->soundTextBox.getTextureID(); },
		soundVolumeRect.getCenter(),
		Int2(soundVolumeRect.getWidth(), soundVolumeRect.getHeight()),
		PivotType::Middle);

	const Rect &optionsTextRect = this->optionsTextBox.getRect();
	this->addDrawCall(
		this->optionsTextBox.getTextureID(),
		optionsTextRect.getTopLeft(),
		Int2(optionsTextRect.getWidth(), optionsTextRect.getHeight()),
		PivotType::TopLeft);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::TopLeft);

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
