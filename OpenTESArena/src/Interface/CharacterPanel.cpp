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

	const DerivedAttributes playerDerivedAttributes = CharacterSheetUiModel::getPlayerDerivedAttributes(game);
	const Buffer<TextBoxInitInfo> playerDerivedAttributesTextBoxInitInfos = CharacterSheetUiView::getPlayerDerivedAttributeTextBoxInitInfos(fontLibrary);
	BufferView<const int> playerDerivedAttributesView = playerDerivedAttributes.getView();
	for (int i = 0; i < playerDerivedAttributesView.getCount(); i++)
	{
		const int derivedAttributeValue = playerDerivedAttributesView.get(i);
		const std::string derivedAttributeValueText = DerivedAttributes::isModifier(i) ?
			CharacterSheetUiModel::getDerivedAttributeDisplayString(derivedAttributeValue) : std::to_string(derivedAttributeValue);
		const TextBoxInitInfo &derivedAttributeTextBoxInitInfo = playerDerivedAttributesTextBoxInitInfos[i];
		if (!this->derivedAttributeTextBoxes[i].init(derivedAttributeTextBoxInitInfo, derivedAttributeValueText, renderer))
		{
			DebugLogErrorFormat("Couldn't init derived player attribute %d text box.", i);
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

	TextureManager &textureManager = game.textureManager;
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

	UiDrawCallInitInfo bodyDrawCallInitInfo;
	bodyDrawCallInitInfo.textureID = bodyTextureID;
	bodyDrawCallInitInfo.position = CharacterSheetUiView::getBodyOffset(game);
	bodyDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(bodyTextureID);
	this->addDrawCall(bodyDrawCallInitInfo);

	UiDrawCallInitInfo pantsDrawCallInitInfo;
	pantsDrawCallInitInfo.textureID = pantsTextureID;
	pantsDrawCallInitInfo.position = CharacterSheetUiView::getPantsOffset(game);
	pantsDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(pantsTextureID);
	this->addDrawCall(pantsDrawCallInitInfo);

	UiDrawCallInitInfo headDrawCallInitInfo;
	headDrawCallInitInfo.textureID = headTextureID;
	headDrawCallInitInfo.position = CharacterSheetUiView::getHeadOffset(game);
	headDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(headTextureID);
	this->addDrawCall(headDrawCallInitInfo);

	UiDrawCallInitInfo shirtDrawCallInitInfo;
	shirtDrawCallInitInfo.textureID = shirtTextureID;
	shirtDrawCallInitInfo.position = CharacterSheetUiView::getShirtOffset(game);
	shirtDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(shirtTextureID);
	this->addDrawCall(shirtDrawCallInitInfo);

	UiDrawCallInitInfo statsBgDrawCallInitInfo;
	statsBgDrawCallInitInfo.textureID = statsBgTextureID;
	statsBgDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(statsBgTextureID);
	this->addDrawCall(statsBgDrawCallInitInfo);

	UiDrawCallInitInfo nextPageDrawCallInitInfo;
	nextPageDrawCallInitInfo.textureID = nextPageTextureID;
	nextPageDrawCallInitInfo.position = CharacterSheetUiView::getNextPageOffset();
	nextPageDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(nextPageTextureID);
	this->addDrawCall(nextPageDrawCallInitInfo);

	const Rect playerNameTextBoxRect = this->nameTextBox.getRect();
	UiDrawCallInitInfo playerNameDrawCallInitInfo;
	playerNameDrawCallInitInfo.textureID = this->nameTextBox.getTextureID();
	playerNameDrawCallInitInfo.position = playerNameTextBoxRect.getTopLeft();
	playerNameDrawCallInitInfo.size = playerNameTextBoxRect.getSize();
	this->addDrawCall(playerNameDrawCallInitInfo);

	const Rect playerRaceTextBoxRect = this->raceTextBox.getRect();
	UiDrawCallInitInfo playerRaceDrawCallInitInfo;
	playerRaceDrawCallInitInfo.textureID = this->raceTextBox.getTextureID();
	playerRaceDrawCallInitInfo.position = playerRaceTextBoxRect.getTopLeft();
	playerRaceDrawCallInitInfo.size = playerRaceTextBoxRect.getSize();
	this->addDrawCall(playerRaceDrawCallInitInfo);

	const Rect playerClassTextBoxRect = this->classTextBox.getRect();
	UiDrawCallInitInfo playerClassDrawCallInitInfo;
	playerClassDrawCallInitInfo.textureID = this->classTextBox.getTextureID();
	playerClassDrawCallInitInfo.position = playerClassTextBoxRect.getTopLeft();
	playerClassDrawCallInitInfo.size = playerClassTextBoxRect.getSize();
	this->addDrawCall(playerClassDrawCallInitInfo);

	for (TextBox &primaryAttributeTextBox : this->attributeTextBoxes)
	{
		const Rect primaryAttributeTextBoxRect = primaryAttributeTextBox.getRect();
		UiDrawCallInitInfo primaryAttributeDrawCallInitInfo;
		primaryAttributeDrawCallInitInfo.textureID = primaryAttributeTextBox.getTextureID();
		primaryAttributeDrawCallInitInfo.position = primaryAttributeTextBoxRect.getTopLeft();
		primaryAttributeDrawCallInitInfo.size = primaryAttributeTextBoxRect.getSize();
		this->addDrawCall(primaryAttributeDrawCallInitInfo);
	}

	for (TextBox &derivedAttributeTextBox : this->derivedAttributeTextBoxes)
	{
		const Rect derivedAttributeTextBoxRect = derivedAttributeTextBox.getRect();
		UiDrawCallInitInfo derivedAttributeDrawCallInitInfo;
		derivedAttributeDrawCallInitInfo.textureID = derivedAttributeTextBox.getTextureID();
		derivedAttributeDrawCallInitInfo.position = derivedAttributeTextBoxRect.getTopLeft();
		derivedAttributeDrawCallInitInfo.size = derivedAttributeTextBoxRect.getSize();
		this->addDrawCall(derivedAttributeDrawCallInitInfo);
	}

	const Rect playerExperienceTextBoxRect = this->experienceTextBox.getRect();
	UiDrawCallInitInfo experienceDrawCallInitInfo;
	experienceDrawCallInitInfo.textureID = this->experienceTextBox.getTextureID();
	experienceDrawCallInitInfo.position = playerExperienceTextBoxRect.getTopLeft();
	experienceDrawCallInitInfo.size = playerExperienceTextBoxRect.getSize();
	this->addDrawCall(experienceDrawCallInitInfo);

	const Rect playerLevelTextBoxRect = this->levelTextBox.getRect();
	UiDrawCallInitInfo levelDrawCallInitInfo;
	levelDrawCallInitInfo.textureID = this->levelTextBox.getTextureID();
	levelDrawCallInitInfo.position = playerLevelTextBoxRect.getTopLeft();
	levelDrawCallInitInfo.size = playerLevelTextBoxRect.getSize();
	this->addDrawCall(levelDrawCallInitInfo);

	const Rect playerHealthTextBoxRect = this->healthTextBox.getRect();
	UiDrawCallInitInfo playerHealthDrawCallInitInfo;
	playerHealthDrawCallInitInfo.textureID = this->healthTextBox.getTextureID();
	playerHealthDrawCallInitInfo.position = playerHealthTextBoxRect.getTopLeft();
	playerHealthDrawCallInitInfo.size = playerHealthTextBoxRect.getSize();
	this->addDrawCall(playerHealthDrawCallInitInfo);

	const Rect playerStaminaTextBoxRect = this->staminaTextBox.getRect();
	UiDrawCallInitInfo playerStaminaDrawCallInitInfo;
	playerStaminaDrawCallInitInfo.textureID = this->staminaTextBox.getTextureID();
	playerStaminaDrawCallInitInfo.position = playerStaminaTextBoxRect.getTopLeft();
	playerStaminaDrawCallInitInfo.size = playerStaminaTextBoxRect.getSize();
	this->addDrawCall(playerStaminaDrawCallInitInfo);

	const Rect playerSpellPointsTextBoxRect = this->spellPointsTextBox.getRect();
	UiDrawCallInitInfo playerSpellPointsDrawCallInitInfo;
	playerSpellPointsDrawCallInitInfo.textureID = this->spellPointsTextBox.getTextureID();
	playerSpellPointsDrawCallInitInfo.position = playerSpellPointsTextBoxRect.getTopLeft();
	playerSpellPointsDrawCallInitInfo.size = playerSpellPointsTextBoxRect.getSize();
	this->addDrawCall(playerSpellPointsDrawCallInitInfo);

	const Rect playerGoldTextBoxRect = this->goldTextBox.getRect();
	UiDrawCallInitInfo playerGoldDrawCallInitInfo;
	playerGoldDrawCallInitInfo.textureID = this->goldTextBox.getTextureID();
	playerGoldDrawCallInitInfo.position = playerGoldTextBoxRect.getTopLeft();
	playerGoldDrawCallInitInfo.size = playerGoldTextBoxRect.getSize();
	this->addDrawCall(playerGoldDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
