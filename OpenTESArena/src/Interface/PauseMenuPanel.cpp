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
	const FontLibrary &fontLibrary = FontLibrary::getInstance();

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

	TextureManager &textureManager = game.textureManager;
	const UiTextureID backgroundTextureID = PauseMenuUiView::allocBackgroundTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);

	UiDrawCallInitInfo bgDrawCallInitInfo;
	bgDrawCallInitInfo.textureID = this->backgroundTextureRef.get();
	bgDrawCallInitInfo.size = this->backgroundTextureRef.getDimensions();
	this->addDrawCall(bgDrawCallInitInfo);

	const UiTextureID gameWorldInterfaceTextureID = GameWorldUiView::allocGameWorldInterfaceTexture(textureManager, renderer);
	this->gameWorldInterfaceTextureRef.init(gameWorldInterfaceTextureID, renderer);

	UiDrawCallInitInfo gameWorldInterfaceDrawCallInitInfo;
	gameWorldInterfaceDrawCallInitInfo.textureID = this->gameWorldInterfaceTextureRef.get();
	gameWorldInterfaceDrawCallInitInfo.position = GameWorldUiView::getGameWorldInterfacePosition();
	gameWorldInterfaceDrawCallInitInfo.size = this->gameWorldInterfaceTextureRef.getDimensions();
	gameWorldInterfaceDrawCallInitInfo.pivotType = UiPivotType::Bottom;
	this->addDrawCall(gameWorldInterfaceDrawCallInitInfo);

	constexpr GameWorldUiView::StatusGradientType gradientType = GameWorldUiView::StatusGradientType::Default;
	const UiTextureID statusGradientTextureID = GameWorldUiView::allocStatusGradientTexture(gradientType, textureManager, renderer);
	this->statusGradientTextureRef.init(statusGradientTextureID, renderer);

	const Rect portraitRect = GameWorldUiView::getPlayerPortraitRect();
	UiDrawCallInitInfo statusGradientDrawCallInitInfo;
	statusGradientDrawCallInitInfo.textureID = this->statusGradientTextureRef.get();
	statusGradientDrawCallInitInfo.position = portraitRect.getTopLeft();
	statusGradientDrawCallInitInfo.size = this->statusGradientTextureRef.getDimensions();
	this->addDrawCall(statusGradientDrawCallInitInfo);

	const Player &player = game.player;
	const UiTextureID playerPortraitTextureID = GameWorldUiView::allocPlayerPortraitTexture(player.male, player.raceID, player.portraitID, textureManager, renderer);
	this->playerPortraitTextureRef.init(playerPortraitTextureID, renderer);

	UiDrawCallInitInfo playerPortraitDrawCallInitInfo;
	playerPortraitDrawCallInitInfo.textureID = this->playerPortraitTextureRef.get();
	playerPortraitDrawCallInitInfo.position = portraitRect.getTopLeft();
	playerPortraitDrawCallInitInfo.size = this->playerPortraitTextureRef.getDimensions();
	this->addDrawCall(playerPortraitDrawCallInitInfo);

	const UiTextureID healthTextureID = GameWorldUiView::allocHealthBarTexture(textureManager, renderer);
	const UiTextureID staminaTextureID = GameWorldUiView::allocStaminaBarTexture(textureManager, renderer);
	const UiTextureID spellPointsTextureID = GameWorldUiView::allocSpellPointsBarTexture(textureManager, renderer);
	this->healthBarTextureRef.init(healthTextureID, renderer);
	this->staminaBarTextureRef.init(staminaTextureID, renderer);
	this->spellPointsBarTextureRef.init(spellPointsTextureID, renderer);

	constexpr UiPivotType statusBarPivotType = GameWorldUiView::StatusBarPivotType;

	UiDrawCallInitInfo healthBarDrawCallInitInfo;
	healthBarDrawCallInitInfo.textureID = this->healthBarTextureRef.get();
	healthBarDrawCallInitInfo.position = GameWorldUiView::HealthBarRect.getBottomLeft();
	healthBarDrawCallInitInfo.sizeFunc = [&game]()
	{
		const Player &player = game.player;
		const Rect barRect = GameWorldUiView::HealthBarRect;
		return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentHealth, player.maxHealth));
	};

	healthBarDrawCallInitInfo.pivotType = statusBarPivotType;
	this->addDrawCall(healthBarDrawCallInitInfo);

	UiDrawCallInitInfo staminaBarDrawCallInitInfo;
	staminaBarDrawCallInitInfo.textureID = this->staminaBarTextureRef.get();
	staminaBarDrawCallInitInfo.position = GameWorldUiView::StaminaBarRect.getBottomLeft();
	staminaBarDrawCallInitInfo.sizeFunc = [&game]()
	{
		const Player &player = game.player;
		const Rect barRect = GameWorldUiView::StaminaBarRect;
		return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentStamina, player.maxStamina));
	};

	staminaBarDrawCallInitInfo.pivotType = statusBarPivotType;
	this->addDrawCall(staminaBarDrawCallInitInfo);

	UiDrawCallInitInfo spellPointsBarDrawCallInitInfo;
	spellPointsBarDrawCallInitInfo.textureID = this->spellPointsBarTextureRef.get();
	spellPointsBarDrawCallInitInfo.position = GameWorldUiView::SpellPointsBarRect.getBottomLeft();
	spellPointsBarDrawCallInitInfo.sizeFunc = [&game]()
	{
		const Player &player = game.player;
		const Rect barRect = GameWorldUiView::SpellPointsBarRect;
		return Int2(barRect.width, GameWorldUiView::getStatusBarCurrentHeight(barRect.height, player.currentSpellPoints, player.maxSpellPoints));
	};

	spellPointsBarDrawCallInitInfo.pivotType = statusBarPivotType;
	this->addDrawCall(spellPointsBarDrawCallInitInfo);

	const UiTextureID noMagicTextureID = GameWorldUiView::allocNoMagicTexture(textureManager, renderer);
	this->noMagicTextureRef.init(noMagicTextureID, renderer);

	const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(player.charClassDefID);
	if (!charClassDef.castsMagic)
	{
		UiDrawCallInitInfo noMagicDrawCallInitInfo;
		noMagicDrawCallInitInfo.textureID = this->noMagicTextureRef.get();
		noMagicDrawCallInitInfo.position = GameWorldUiView::getNoMagicTexturePosition();
		noMagicDrawCallInitInfo.size = this->noMagicTextureRef.getDimensions();
		this->addDrawCall(noMagicDrawCallInitInfo);
	}

	const Rect playerNameTextBoxRect = this->playerNameTextBox.getRect();
	UiDrawCallInitInfo playerNameDrawCallInitInfo;
	playerNameDrawCallInitInfo.textureID = this->playerNameTextBox.getTextureID();
	playerNameDrawCallInitInfo.position = playerNameTextBoxRect.getTopLeft();
	playerNameDrawCallInitInfo.size = playerNameTextBoxRect.getSize();
	this->addDrawCall(playerNameDrawCallInitInfo);

	// Cover up the detail slider with a new options background.
	const UiTextureID optionsButtonTextureID = PauseMenuUiView::allocOptionsButtonTexture(textureManager, renderer);
	this->optionsButtonTextureRef.init(optionsButtonTextureID, renderer);

	const Rect optionsButtonRect = this->optionsButton.getRect();
	UiDrawCallInitInfo optionsButtonDrawCallInitInfo;
	optionsButtonDrawCallInitInfo.textureID = this->optionsButtonTextureRef.get();
	optionsButtonDrawCallInitInfo.position = optionsButtonRect.getTopLeft();
	optionsButtonDrawCallInitInfo.size = optionsButtonRect.getSize();
	this->addDrawCall(optionsButtonDrawCallInitInfo);

	const Rect musicVolumeRect = this->musicTextBox.getRect();
	UiDrawCallInitInfo musicVolumeTextDrawCallInitInfo;
	musicVolumeTextDrawCallInitInfo.textureFunc = [this]() { return this->musicTextBox.getTextureID(); };
	musicVolumeTextDrawCallInitInfo.position = musicVolumeRect.getCenter();
	musicVolumeTextDrawCallInitInfo.size = musicVolumeRect.getSize();
	musicVolumeTextDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(musicVolumeTextDrawCallInitInfo);

	const Rect soundVolumeRect = this->soundTextBox.getRect();
	UiDrawCallInitInfo soundVolumeTextDrawCallInitInfo;
	soundVolumeTextDrawCallInitInfo.textureFunc = [this]() { return this->soundTextBox.getTextureID(); };
	soundVolumeTextDrawCallInitInfo.position = soundVolumeRect.getCenter();
	soundVolumeTextDrawCallInitInfo.size = soundVolumeRect.getSize();
	soundVolumeTextDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(soundVolumeTextDrawCallInitInfo);

	const Rect optionsTextRect = this->optionsTextBox.getRect();
	UiDrawCallInitInfo optionsTextDrawCallInitInfo;
	optionsTextDrawCallInitInfo.textureID = this->optionsTextBox.getTextureID();
	optionsTextDrawCallInitInfo.position = optionsTextRect.getTopLeft();
	optionsTextDrawCallInitInfo.size = optionsTextRect.getSize();
	this->addDrawCall(optionsTextDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), UiPivotType::TopLeft);

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
