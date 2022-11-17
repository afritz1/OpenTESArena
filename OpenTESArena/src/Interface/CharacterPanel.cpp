#include "SDL.h"

#include "CharacterPanel.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "CommonUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../UI/CursorData.h"

#include "components/debug/Debug.h"

CharacterPanel::CharacterPanel(Game &game)
	: Panel(game) { }

CharacterPanel::~CharacterPanel()
{
	auto &inputManager = this->getGame().getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, false);
}

bool CharacterPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	const std::string playerNameText = CharacterSheetUiModel::getPlayerName(game);
	const TextBox::InitInfo playerNameTextBoxInitInfo =
		CharacterSheetUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->playerNameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const std::string playerRaceText = CharacterSheetUiModel::getPlayerRaceName(game);
	const TextBox::InitInfo playerRaceTextBoxInitInfo =
		CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(playerRaceText, fontLibrary);
	if (!this->playerRaceTextBox.init(playerRaceTextBoxInitInfo, playerRaceText, renderer))
	{
		DebugLogError("Couldn't init player race text box.");
		return false;
	}

	const std::string playerClassText = CharacterSheetUiModel::getPlayerClassName(game);
	const TextBox::InitInfo playerClassTextBoxInitInfo =
		CharacterSheetUiView::getPlayerClassTextBoxInitInfo(playerClassText, fontLibrary);
	if (!this->playerClassTextBox.init(playerClassTextBoxInitInfo, playerClassText, renderer))
	{
		DebugLogError("Couldn't init player class text box.");
		return false;
	}

	const std::string playerStrengthText = CharacterSheetUiModel::getPlayerStrengthText(game);
	const TextBox::InitInfo playerStrengthTextBoxInitInfo =
		CharacterSheetUiView::getPlayerStrengthTextBoxInitInfo(playerStrengthText, fontLibrary);
	if (!this->playerStrengthTextBox.init(playerStrengthTextBoxInitInfo, playerStrengthText, renderer))
	{
		DebugLogError("Couldn't init player strength text box.");
		return false;
	}

	const std::string playerIntelligenceText = CharacterSheetUiModel::getPlayerIntelligenceText(game);
	const TextBox::InitInfo playerIntelligenceTextBoxInitInfo =
		CharacterSheetUiView::getPlayerIntelligenceTextBoxInitInfo(playerIntelligenceText, fontLibrary);
	if (!this->playerIntelligenceTextBox.init(playerIntelligenceTextBoxInitInfo, playerIntelligenceText, renderer))
	{
		DebugLogError("Couldn't init player intelligence text box.");
		return false;
	}

	const std::string playerWillpowerText = CharacterSheetUiModel::getPlayerWillpowerText(game);
	const TextBox::InitInfo playerWillpowerTextBoxInitInfo =
		CharacterSheetUiView::getPlayerWillpowerTextBoxInitInfo(playerWillpowerText, fontLibrary);
	if (!this->playerWillpowerTextBox.init(playerWillpowerTextBoxInitInfo, playerWillpowerText, renderer))
	{
		DebugLogError("Couldn't init player willpower text box.");
		return false;
	}

	const std::string playerAgilityText = CharacterSheetUiModel::getPlayerAgilityText(game);
	const TextBox::InitInfo playerAgilityTextBoxInitInfo =
		CharacterSheetUiView::getPlayerAgilityTextBoxInitInfo(playerAgilityText, fontLibrary);
	if (!this->playerAgilityTextBox.init(playerAgilityTextBoxInitInfo, playerAgilityText, renderer))
	{
		DebugLogError("Couldn't init player agility text box.");
		return false;
	}

	const std::string playerSpeedText = CharacterSheetUiModel::getPlayerSpeedText(game);
	const TextBox::InitInfo playerSpeedTextBoxInitInfo =
		CharacterSheetUiView::getPlayerSpeedTextBoxInitInfo(playerSpeedText, fontLibrary);
	if (!this->playerSpeedTextBox.init(playerSpeedTextBoxInitInfo, playerSpeedText, renderer))
	{
		DebugLogError("Couldn't init player speed text box.");
		return false;
	}

	const std::string playerEnduranceText = CharacterSheetUiModel::getPlayerEnduranceText(game);
	const TextBox::InitInfo playerEnduranceTextBoxInitInfo =
		CharacterSheetUiView::getPlayerEnduranceTextBoxInitInfo(playerEnduranceText, fontLibrary);
	if (!this->playerEnduranceTextBox.init(playerEnduranceTextBoxInitInfo, playerEnduranceText, renderer))
	{
		DebugLogError("Couldn't init player endurance text box.");
		return false;
	}

	const std::string playerPersonalityText = CharacterSheetUiModel::getPlayerPersonalityText(game);
	const TextBox::InitInfo playerPersonalityTextBoxInitInfo =
		CharacterSheetUiView::getPlayerPersonalityTextBoxInitInfo(playerPersonalityText, fontLibrary);
	if (!this->playerPersonalityTextBox.init(playerPersonalityTextBoxInitInfo, playerPersonalityText, renderer))
	{
		DebugLogError("Couldn't init player personality text box.");
		return false;
	}

	const std::string playerLuckText = CharacterSheetUiModel::getPlayerLuckText(game);
	const TextBox::InitInfo playerLuckTextBoxInitInfo =
		CharacterSheetUiView::getPlayerLuckTextBoxInitInfo(playerLuckText, fontLibrary);
	if (!this->playerLuckTextBox.init(playerLuckTextBoxInitInfo, playerLuckText, renderer))
	{
		DebugLogError("Couldn't init player luck text box.");
		return false;
	}

	this->doneButton = Button<Game&>(
		CharacterSheetUiView::DoneButtonCenterPoint,
		CharacterSheetUiView::DoneButtonWidth,
		CharacterSheetUiView::DoneButtonHeight,
		CharacterSheetUiController::onDoneButtonSelected);
	this->nextPageButton = Button<Game&>(
		CharacterSheetUiView::NextPageButtonX,
		CharacterSheetUiView::NextPageButtonY,
		CharacterSheetUiView::NextPageButtonWidth,
		CharacterSheetUiView::NextPageButtonHeight,
		CharacterSheetUiController::onNextPageButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->doneButton.getRect(),
		[this, &game]() { this->doneButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->nextPageButton.getRect(),
		[this, &game]() { this->nextPageButton.click(game); });

	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, true);

	auto doneInputActionFunc = CharacterSheetUiController::onDoneInputAction;
	this->addInputActionListener(InputActionName::Back, doneInputActionFunc);
	this->addInputActionListener(InputActionName::CharacterSheet, doneInputActionFunc);

	auto &textureManager = game.getTextureManager();
	const UiTextureID bodyTextureID = CharacterSheetUiView::allocBodyTexture(game);
	const UiTextureID pantsTextureID = CharacterSheetUiView::allocPantsTexture(game);
	const UiTextureID headTextureID = CharacterSheetUiView::allocHeadTexture(game);
	const UiTextureID shirtTextureID = CharacterSheetUiView::allocShirtTexture(game);
	const UiTextureID statsBgTextureID = CharacterSheetUiView::allocStatsBgTexture(textureManager, renderer);
	const UiTextureID nextPageTextureID = CharacterSheetUiView::allocNextPageTexture(textureManager, renderer);
	this->bodyTextureRef.init(bodyTextureID, renderer);
	this->pantsTextureRef.init(pantsTextureID, renderer);
	this->headTextureRef.init(headTextureID, renderer);
	this->shirtTextureRef.init(shirtTextureID, renderer);
	this->statsBgTextureRef.init(statsBgTextureID, renderer);
	this->nextPageTextureRef.init(nextPageTextureID, renderer);

	const Int2 bodyTextureDims = *renderer.tryGetUiTextureDims(bodyTextureID);
	const Int2 pantsTextureDims = *renderer.tryGetUiTextureDims(pantsTextureID);
	const Int2 headTextureDims = *renderer.tryGetUiTextureDims(headTextureID);
	const Int2 shirtTextureDims = *renderer.tryGetUiTextureDims(shirtTextureID);
	const Int2 statsBgTextureDims = *renderer.tryGetUiTextureDims(statsBgTextureID);
	const Int2 nextPageTextureDims = *renderer.tryGetUiTextureDims(nextPageTextureID);

	this->addDrawCall(
		bodyTextureID,
		CharacterSheetUiView::getBodyOffset(game),
		bodyTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		pantsTextureID,
		CharacterSheetUiView::getPantsOffset(game),
		pantsTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		headTextureID,
		CharacterSheetUiView::getHeadOffset(game),
		headTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		shirtTextureID,
		CharacterSheetUiView::getShirtOffset(game),
		shirtTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		statsBgTextureID,
		Int2::Zero,
		statsBgTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		nextPageTextureID,
		CharacterSheetUiView::getNextPageOffset(),
		nextPageTextureDims,
		PivotType::TopLeft);

	const Rect &playerNameTextBoxRect = this->playerNameTextBox.getRect();
	this->addDrawCall(
		this->playerNameTextBox.getTextureID(),
		playerNameTextBoxRect.getTopLeft(),
		Int2(playerNameTextBoxRect.getWidth(), playerNameTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect &playerRaceTextBoxRect = this->playerRaceTextBox.getRect();
	this->addDrawCall(
		this->playerRaceTextBox.getTextureID(),
		playerRaceTextBoxRect.getTopLeft(),
		Int2(playerRaceTextBoxRect.getWidth(), playerRaceTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerClassTextBoxRect = this->playerClassTextBox.getRect();
	this->addDrawCall(
		this->playerClassTextBox.getTextureID(),
		playerClassTextBoxRect.getTopLeft(),
		Int2(playerClassTextBoxRect.getWidth(), playerClassTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerStrengthTextBoxRect = this->playerStrengthTextBox.getRect();
	this->addDrawCall(
		this->playerStrengthTextBox.getTextureID(),
		playerStrengthTextBoxRect.getTopLeft(),
		Int2(playerStrengthTextBoxRect.getWidth(), playerStrengthTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerIntelligenceTextBoxRect = this->playerIntelligenceTextBox.getRect();
	this->addDrawCall(
		this->playerIntelligenceTextBox.getTextureID(),
		playerIntelligenceTextBoxRect.getTopLeft(),
		Int2(playerIntelligenceTextBoxRect.getWidth(), playerIntelligenceTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerWillpowerTextBoxRect = this->playerWillpowerTextBox.getRect();
	this->addDrawCall(
		this->playerWillpowerTextBox.getTextureID(),
		playerWillpowerTextBoxRect.getTopLeft(),
		Int2(playerWillpowerTextBoxRect.getWidth(), playerWillpowerTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerAgilityTextBoxRect = this->playerAgilityTextBox.getRect();
	this->addDrawCall(
		this->playerAgilityTextBox.getTextureID(),
		playerAgilityTextBoxRect.getTopLeft(),
		Int2(playerAgilityTextBoxRect.getWidth(), playerAgilityTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerSpeedTextBoxRect = this->playerSpeedTextBox.getRect();
	this->addDrawCall(
		this->playerSpeedTextBox.getTextureID(),
		playerSpeedTextBoxRect.getTopLeft(),
		Int2(playerSpeedTextBoxRect.getWidth(), playerSpeedTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerEnduranceTextBoxRect = this->playerEnduranceTextBox.getRect();
	this->addDrawCall(
		this->playerEnduranceTextBox.getTextureID(),
		playerEnduranceTextBoxRect.getTopLeft(),
		Int2(playerEnduranceTextBoxRect.getWidth(), playerEnduranceTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerPersonalityTextBoxRect = this->playerPersonalityTextBox.getRect();
	this->addDrawCall(
		this->playerPersonalityTextBox.getTextureID(),
		playerPersonalityTextBoxRect.getTopLeft(),
		Int2(playerPersonalityTextBoxRect.getWidth(), playerPersonalityTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect& playerLuckTextBoxRect = this->playerLuckTextBox.getRect();
	this->addDrawCall(
		this->playerLuckTextBox.getTextureID(),
		playerLuckTextBoxRect.getTopLeft(),
		Int2(playerLuckTextBoxRect.getWidth(), playerLuckTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
