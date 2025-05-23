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
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextRenderUtils.h"

PauseMenuPanel::PauseMenuPanel(Game &game)
	: Panel(game) { }

bool PauseMenuPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	const TextBoxInitInfo playerNameTextBoxInitInfo =
		GameWorldUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->playerNameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const std::string musicText = PauseMenuUiModel::getMusicVolumeText(game);
	const TextBoxInitInfo musicTextBoxInitInfo = PauseMenuUiView::getMusicTextBoxInitInfo(fontLibrary);
	if (!this->musicTextBox.init(musicTextBoxInitInfo, musicText, renderer))
	{
		DebugLogError("Couldn't init music volume text box.");
		return false;
	}

	const std::string soundText = PauseMenuUiModel::getSoundVolumeText(game);
	const TextBoxInitInfo soundTextBoxInitInfo = PauseMenuUiView::getSoundTextBoxInitInfo(fontLibrary);
	if (!this->soundTextBox.init(soundTextBoxInitInfo, soundText, renderer))
	{
		DebugLogError("Couldn't init sound volume text box.");
		return false;
	}

	const std::string optionsText = PauseMenuUiModel::getOptionsButtonText(game);
	const TextBoxInitInfo optionsTextBoxInitInfo = PauseMenuUiView::getOptionsTextBoxInitInfo(optionsText, fontLibrary);
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

	auto &textureManager = game.textureManager;
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

	const auto &player = game.player;
	const UiTextureID playerPortraitTextureID = GameWorldUiView::allocPlayerPortraitTexture(
		player.male, player.raceID, player.portraitID, textureManager, renderer);
	this->playerPortraitTextureRef.init(playerPortraitTextureID, renderer);
	this->addDrawCall(
		this->playerPortraitTextureRef.get(),
		portraitRect.getTopLeft(),
		Int2(this->playerPortraitTextureRef.getWidth(), this->playerPortraitTextureRef.getHeight()),
		PivotType::TopLeft);

	const UiTextureID healthTextureID = GameWorldUiView::allocHealthBarTexture(textureManager, renderer);
	this->healthBarTextureRef.init(healthTextureID, renderer);
	const UiTextureID staminaTextureID = GameWorldUiView::allocStaminaBarTexture(textureManager, renderer);
	this->staminaBarTextureRef.init(staminaTextureID, renderer);
	const UiTextureID spellPointsTextureID = GameWorldUiView::allocSpellPointsBarTexture(textureManager, renderer);
	this->spellPointsBarTextureRef.init(spellPointsTextureID, renderer);

	UiDrawCall::TextureFunc healthBarTextureFunc = [this]() { return this->healthBarTextureRef.get(); };
	UiDrawCall::TextureFunc staminaBarTextureFunc = [this]() { return this->staminaBarTextureRef.get(); };
	UiDrawCall::TextureFunc spellPointsBarTextureFunc = [this]() { return this->spellPointsBarTextureRef.get(); };
	UiDrawCall::PositionFunc healthBarPositionFunc = []() { return GameWorldUiView::HealthBarRect.getBottomLeft(); };
	UiDrawCall::PositionFunc staminaBarPositionFunc = []() { return GameWorldUiView::StaminaBarRect.getBottomLeft(); };
	UiDrawCall::PositionFunc spellPointsBarPositionFunc = []() { return GameWorldUiView::SpellPointsBarRect.getBottomLeft(); };

	UiDrawCall::SizeFunc healthBarSizeFunc = [&game]()
	{
		const Player &player = game.player;
		const Rect &barRect = GameWorldUiView::HealthBarRect;
		return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentHealth, player.maxHealth));
	};

	UiDrawCall::SizeFunc staminaBarSizeFunc = [&game]()
	{
		const Player &player = game.player;
		const Rect &barRect = GameWorldUiView::StaminaBarRect;
		return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentStamina, player.maxStamina));
	};

	UiDrawCall::SizeFunc spellPointsBarSizeFunc = [&game]()
	{
		const Player &player = game.player;
		const Rect &barRect = GameWorldUiView::SpellPointsBarRect;
		return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentSpellPoints, player.maxSpellPoints));
	};

	UiDrawCall::PivotFunc statusBarPivotFunc = []() { return GameWorldUiView::StatusBarPivotType; };
	UiDrawCall::ActiveFunc statusBarActiveFunc = []() { return true; };

	this->addDrawCall(
		healthBarTextureFunc,
		healthBarPositionFunc,
		healthBarSizeFunc,
		statusBarPivotFunc,
		statusBarActiveFunc);
	this->addDrawCall(
		staminaBarTextureFunc,
		staminaBarPositionFunc,
		staminaBarSizeFunc,
		statusBarPivotFunc,
		statusBarActiveFunc);
	this->addDrawCall(
		spellPointsBarTextureFunc,
		spellPointsBarPositionFunc,
		spellPointsBarSizeFunc,
		statusBarPivotFunc,
		statusBarActiveFunc);

	const UiTextureID noMagicTextureID = GameWorldUiView::allocNoMagicTexture(textureManager, renderer);
	this->noMagicTextureRef.init(noMagicTextureID, renderer);

	const auto &charClassLibrary = CharacterClassLibrary::getInstance();
	const auto &charClassDef = charClassLibrary.getDefinition(player.charClassDefID);
	if (!charClassDef.castsMagic)
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
		optionsButtonRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerNameRect = this->playerNameTextBox.getRect();
	this->addDrawCall(
		this->playerNameTextBox.getTextureID(),
		playerNameRect.getTopLeft(),
		playerNameRect.getSize(),
		PivotType::TopLeft);

	const Rect &musicVolumeRect = this->musicTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->musicTextBox.getTextureID(); },
		musicVolumeRect.getCenter(),
		musicVolumeRect.getSize(),
		PivotType::Middle);

	const Rect &soundVolumeRect = this->soundTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->soundTextBox.getTextureID(); },
		soundVolumeRect.getCenter(),
		soundVolumeRect.getSize(),
		PivotType::Middle);

	const Rect &optionsTextRect = this->optionsTextBox.getRect();
	this->addDrawCall(
		this->optionsTextBox.getTextureID(),
		optionsTextRect.getTopLeft(),
		optionsTextRect.getSize(),
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
