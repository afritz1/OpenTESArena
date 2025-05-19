#include <map>
#include <vector>

#include "CharacterPanel.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "CommonUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/FontLibrary.h"

#include "components/debug/Debug.h"

CharacterPanel::CharacterPanel(Game &game)
	: Panel(game) { }

CharacterPanel::~CharacterPanel()
{
	auto &inputManager = this->getGame().inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, false);
}

bool CharacterPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const TextBoxInitInfo playerNameTextBoxInitInfo = CharacterSheetUiView::getPlayerNameTextBoxInitInfo(fontLibrary);
	const std::string playerNameText = CharacterSheetUiModel::getPlayerName(game);
	if (!this->nameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const TextBoxInitInfo playerRaceTextBoxInitInfo = CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(fontLibrary);
	const std::string playerRaceText = CharacterSheetUiModel::getPlayerRaceName(game);
	if (!this->raceTextBox.init(playerRaceTextBoxInitInfo, playerRaceText, renderer))
	{
		DebugLogError("Couldn't init player race text box.");
		return false;
	}

	const TextBoxInitInfo playerClassTextBoxInitInfo = CharacterSheetUiView::getPlayerClassTextBoxInitInfo(fontLibrary);
	const std::string playerClassText = CharacterSheetUiModel::getPlayerClassName(game);
	if (!this->classTextBox.init(playerClassTextBoxInitInfo, playerClassText, renderer))
	{
		DebugLogError("Couldn't init player class text box.");
		return false;
	}

	const PrimaryAttributes &playerAttributes = CharacterSheetUiModel::getPlayerAttributes(game);
	const Buffer<TextBoxInitInfo> playerAttributesTextBoxInitInfos = CharacterSheetUiView::getPlayerAttributeTextBoxInitInfos(fontLibrary);
	const BufferView<const PrimaryAttribute> playerAttributesView = playerAttributes.getView();
	for (int i = 0; i < playerAttributesView.getCount(); i++)
	{
		const PrimaryAttribute &attribute = playerAttributesView.get(i);
		const int attributeValue = attribute.maxValue;
		const std::string attributeValueText = std::to_string(attributeValue);
		const TextBoxInitInfo &attributeTextBoxInitInfo = playerAttributesTextBoxInitInfos[i];
		if (!this->attributeTextBoxes[i].init(attributeTextBoxInitInfo, attributeValueText, renderer))
		{
			DebugLogError("Couldn't init player attribute " + std::string(attribute.name) + " text box.");
			return false;
		}
	}

	const TextBoxInitInfo playerExperienceTextBoxInitInfo = CharacterSheetUiView::getPlayerExperienceTextBoxInitInfo(fontLibrary);
	const std::string playerExperienceText = CharacterSheetUiModel::getPlayerExperience(game);
	if (!this->experienceTextBox.init(playerExperienceTextBoxInitInfo, playerExperienceText, renderer))
	{
		DebugLogError("Couldn't init player experience text box.");
		return false;
	}

	const TextBoxInitInfo playerLevelTextBoxInitInfo = CharacterSheetUiView::getPlayerLevelTextBoxInitInfo(fontLibrary);
	const std::string playerLevelText = CharacterSheetUiModel::getPlayerLevel(game);
	if (!this->levelTextBox.init(playerLevelTextBoxInitInfo, playerLevelText, renderer))
	{
		DebugLogError("Couldn't init player level text box.");
		return false;
	}

	const TextBoxInitInfo playerHealthTextBoxInitInfo = CharacterSheetUiView::getPlayerHealthTextBoxInitInfo(fontLibrary);
	const std::string playerHealthText = CharacterSheetUiModel::getPlayerHealth(game);
	if (!this->healthTextBox.init(playerHealthTextBoxInitInfo, playerHealthText, renderer))
	{
		DebugLogError("Couldn't init player health text box.");
		return false;
	}

	const TextBoxInitInfo playerStaminaTextBoxInitInfo = CharacterSheetUiView::getPlayerStaminaTextBoxInitInfo(fontLibrary);
	const std::string playerStaminaText = CharacterSheetUiModel::getPlayerStamina(game);
	if (!this->staminaTextBox.init(playerStaminaTextBoxInitInfo, playerStaminaText, renderer))
	{
		DebugLogError("Couldn't init player stamina text box.");
		return false;
	}

	const TextBoxInitInfo playerSpellPointsTextBoxInitInfo = CharacterSheetUiView::getPlayerSpellPointsTextBoxInitInfo(fontLibrary);
	const std::string playerSpellPointsText = CharacterSheetUiModel::getPlayerSpellPoints(game);
	if (!this->spellPointsTextBox.init(playerSpellPointsTextBoxInitInfo, playerSpellPointsText, renderer))
	{
		DebugLogError("Couldn't init player spell points text box.");
		return false;
	}

	const TextBoxInitInfo playerGoldTextBoxInitInfo = CharacterSheetUiView::getPlayerGoldTextBoxInitInfo(fontLibrary);
	const std::string playerGoldText = CharacterSheetUiModel::getPlayerGold(game);
	if (!this->goldTextBox.init(playerGoldTextBoxInitInfo, playerGoldText, renderer))
	{
		DebugLogError("Couldn't init player gold text box.");
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

	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, true);

	auto doneInputActionFunc = CharacterSheetUiController::onDoneInputAction;
	this->addInputActionListener(InputActionName::Back, doneInputActionFunc);
	this->addInputActionListener(InputActionName::CharacterSheet, doneInputActionFunc);

	auto &textureManager = game.textureManager;
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

	const Rect &playerNameTextBoxRect = this->nameTextBox.getRect();
	this->addDrawCall(
		this->nameTextBox.getTextureID(),
		playerNameTextBoxRect.getTopLeft(),
		playerNameTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerRaceTextBoxRect = this->raceTextBox.getRect();
	this->addDrawCall(
		this->raceTextBox.getTextureID(),
		playerRaceTextBoxRect.getTopLeft(),
		playerRaceTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerClassTextBoxRect = this->classTextBox.getRect();
	this->addDrawCall(
		this->classTextBox.getTextureID(),
		playerClassTextBoxRect.getTopLeft(),
		playerClassTextBoxRect.getSize(),
		PivotType::TopLeft);

	for (TextBox &playerAttributeTextBox : this->attributeTextBoxes)
	{
		const Rect &playerAttributeTextBoxRect = playerAttributeTextBox.getRect();
		this->addDrawCall(
			playerAttributeTextBox.getTextureID(),
			playerAttributeTextBoxRect.getTopLeft(),
			playerAttributeTextBoxRect.getSize(),
			PivotType::TopLeft);
	}

	const Rect &playerExperienceTextBoxRect = this->experienceTextBox.getRect();
	this->addDrawCall(
		this->experienceTextBox.getTextureID(),
		playerExperienceTextBoxRect.getTopLeft(),
		playerExperienceTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerLevelTextBoxRect = this->levelTextBox.getRect();
	this->addDrawCall(
		this->levelTextBox.getTextureID(),
		playerLevelTextBoxRect.getTopLeft(),
		playerLevelTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerHealthTextBoxRect = this->healthTextBox.getRect();
	this->addDrawCall(
		this->healthTextBox.getTextureID(),
		playerHealthTextBoxRect.getTopLeft(),
		playerHealthTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerStaminaTextBoxRect = this->staminaTextBox.getRect();
	this->addDrawCall(
		this->staminaTextBox.getTextureID(),
		playerStaminaTextBoxRect.getTopLeft(),
		playerStaminaTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerSpellPointsTextBoxRect = this->spellPointsTextBox.getRect();
	this->addDrawCall(
		this->spellPointsTextBox.getTextureID(),
		playerSpellPointsTextBoxRect.getTopLeft(),
		playerSpellPointsTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerGoldTextBoxRect = this->goldTextBox.getRect();
	this->addDrawCall(
		this->goldTextBox.getTextureID(),
		playerGoldTextBoxRect.getTopLeft(),
		playerGoldTextBoxRect.getSize(),
		PivotType::TopLeft);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
