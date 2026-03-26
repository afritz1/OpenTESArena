#include "CharacterEquipmentUiState.h"
#include "CharacterSheetUiMVC.h"
#include "CharacterUiState.h"
#include "GameWorldUiState.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"

namespace
{
	std::string GetPrimaryAttributeTextBoxElementName(const char *attributeName)
	{
		char elementName[64];
		std::snprintf(elementName, sizeof(elementName), "Character%sTextBox", attributeName);
		return std::string(elementName);
	}

	std::string GetDerivedAttributeTextBoxElementName(const char *attributeName)
	{
		return GetPrimaryAttributeTextBoxElementName(attributeName);
	}
}

CharacterUiState::CharacterUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->bodyTextureID = -1;
	this->pantsTextureID = -1;
	this->headTextureID = -1;
	this->shirtTextureID = -1;
}

void CharacterUiState::init(Game &game)
{
	this->game = &game;

	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	this->bodyTextureID = CharacterSheetUiView::allocBodyTexture(game);
	this->pantsTextureID = CharacterSheetUiView::allocPantsTexture(game);
	this->headTextureID = CharacterSheetUiView::allocHeadTexture(game);
	this->shirtTextureID = CharacterSheetUiView::allocShirtTexture(game);
}

void CharacterUiState::freeTextures(Renderer &renderer)
{
	if (this->bodyTextureID >= 0)
	{
		renderer.freeUiTexture(this->bodyTextureID);
		this->bodyTextureID = -1;
	}

	if (this->pantsTextureID >= 0)
	{
		renderer.freeUiTexture(this->pantsTextureID);
		this->pantsTextureID = -1;
	}

	if (this->headTextureID >= 0)
	{
		renderer.freeUiTexture(this->headTextureID);
		this->headTextureID = -1;
	}

	if (this->shirtTextureID >= 0)
	{
		renderer.freeUiTexture(this->shirtTextureID);
		this->shirtTextureID = -1;
	}
}

void CharacterUI::create(Game &game)
{
	CharacterUiState &state = CharacterUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(CharacterUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiElementInitInfo bodyImageElementInitInfo;
	bodyImageElementInitInfo.name = "CharacterBodyImage";
	bodyImageElementInitInfo.position = CharacterSheetUiView::getBodyOffset(game);
	bodyImageElementInitInfo.drawOrder = 1;
	uiManager.createImage(bodyImageElementInitInfo, state.bodyTextureID, state.contextInstID, renderer);

	UiElementInitInfo pantsImageElementInitInfo;
	pantsImageElementInitInfo.name = "CharacterPantsImage";
	pantsImageElementInitInfo.position = CharacterSheetUiView::getPantsOffset(game);
	pantsImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(pantsImageElementInitInfo, state.pantsTextureID, state.contextInstID, renderer);

	UiElementInitInfo headImageElementInitInfo;
	headImageElementInitInfo.name = "CharacterHeadImage";
	headImageElementInitInfo.position = CharacterSheetUiView::getHeadOffset(game);
	headImageElementInitInfo.drawOrder = 3;
	uiManager.createImage(headImageElementInitInfo, state.headTextureID, state.contextInstID, renderer);

	UiElementInitInfo shirtImageElementInitInfo;
	shirtImageElementInitInfo.name = "CharacterShirtImage";
	shirtImageElementInitInfo.position = CharacterSheetUiView::getShirtOffset(game);
	shirtImageElementInitInfo.drawOrder = 4;
	uiManager.createImage(shirtImageElementInitInfo, state.shirtTextureID, state.contextInstID, renderer);

	const std::string playerNameText = CharacterSheetUiModel::getPlayerName(game);
	const UiElementInstanceID playerNameTextBoxElementInstID = uiManager.getElementByName("CharacterNameTextBox");
	uiManager.setTextBoxText(playerNameTextBoxElementInstID, playerNameText.c_str());

	const std::string playerRaceText = CharacterSheetUiModel::getPlayerRaceName(game);
	const UiElementInstanceID playerRaceTextBoxElementInstID = uiManager.getElementByName("CharacterRaceTextBox");
	uiManager.setTextBoxText(playerRaceTextBoxElementInstID, playerRaceText.c_str());

	const std::string playerClassText = CharacterSheetUiModel::getPlayerClassName(game);
	const UiElementInstanceID playerClassTextBoxElementInstID = uiManager.getElementByName("CharacterClassTextBox");
	uiManager.setTextBoxText(playerClassTextBoxElementInstID, playerClassText.c_str());

	const std::string playerExperienceText = CharacterSheetUiModel::getPlayerExperience(game);
	const UiElementInstanceID experienceTextBoxElementInstID = uiManager.getElementByName("CharacterExperienceTextBox");
	uiManager.setTextBoxText(experienceTextBoxElementInstID, playerExperienceText.c_str());

	const std::string playerLevelText = CharacterSheetUiModel::getPlayerLevel(game);
	const UiElementInstanceID levelTextBoxElementInstID = uiManager.getElementByName("CharacterLevelTextBox");
	uiManager.setTextBoxText(levelTextBoxElementInstID, playerLevelText.c_str());

	const std::string playerGoldText = CharacterSheetUiModel::getPlayerGold(game);
	const UiElementInstanceID goldTextBoxElementInstID = uiManager.getElementByName("CharacterGoldTextBox");
	uiManager.setTextBoxText(goldTextBoxElementInstID, playerGoldText.c_str());

	const Player &player = game.player;
	Span<const PrimaryAttribute> playerAttributesView = player.primaryAttributes.getView();
	for (const PrimaryAttribute &attribute : playerAttributesView)
	{
		const std::string attributeElementName = GetPrimaryAttributeTextBoxElementName(attribute.name);
		const UiElementInstanceID attributeTextBoxElementInstID = uiManager.getElementByName(attributeElementName.c_str());

		const std::string attributeValueText = std::to_string(attribute.maxValue);
		uiManager.setTextBoxText(attributeTextBoxElementInstID, attributeValueText.c_str());
	}

	const DerivedAttributes derivedAttributes = CharacterSheetUiModel::getPlayerDerivedAttributes(game);
	const Span<const int> derivedAttributesView = derivedAttributes.getView();
	for (int i = 0; i < derivedAttributesView.getCount(); i++)
	{
		const int derivedAttributeValue = derivedAttributesView[i];
		const std::string derivedAttributeDisplayString = DerivedAttributes::isModifier(i) ? CharacterSheetUiModel::getDerivedAttributeDisplayString(derivedAttributeValue) : std::to_string(derivedAttributeValue);

		DebugAssertIndex(CharacterSheetUiModel::DerivedAttributeUiNames, i);
		const std::string derivedAttributeElementName = GetDerivedAttributeTextBoxElementName(CharacterSheetUiModel::DerivedAttributeUiNames[i]);
		const UiElementInstanceID derivedAttributeTextBoxElementInstID = uiManager.getElementByName(derivedAttributeElementName.c_str());
		uiManager.setTextBoxText(derivedAttributeTextBoxElementInstID, derivedAttributeDisplayString.c_str());
	}

	const UiElementInstanceID healthTextBoxElementInstID = uiManager.getElementByName("CharacterHealthTextBox");
	const std::string healthValueText = CharacterSheetUiModel::getPlayerHealth(game);
	uiManager.setTextBoxText(healthTextBoxElementInstID, healthValueText.c_str());

	const UiElementInstanceID staminaTextBoxElementInstID = uiManager.getElementByName("CharacterStaminaTextBox");
	const std::string staminaValueText = CharacterSheetUiModel::getPlayerStamina(game);
	uiManager.setTextBoxText(staminaTextBoxElementInstID, staminaValueText.c_str());

	const UiElementInstanceID spellPointsTextBoxElementInstID = uiManager.getElementByName("CharacterSpellPointsTextBox");
	const std::string spellPointsValueText = CharacterSheetUiModel::getPlayerSpellPoints(game);
	uiManager.setTextBoxText(spellPointsTextBoxElementInstID, spellPointsValueText.c_str());

	game.setCursorOverride(std::nullopt);

	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, true);
}

void CharacterUI::destroy()
{
	CharacterUiState &state = CharacterUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	state.freeTextures(renderer);

	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, false);
}

void CharacterUI::update(double dt)
{
	// Do nothing.
}

void CharacterUI::onDoneButtonSelected(MouseButtonType mouseButtonType)
{
	CharacterUiState &state = CharacterUI::state;
	Game &game = *state.game;
	game.setNextContext(GameWorldUI::ContextName);
}

void CharacterUI::onNextPageButtonSelected(MouseButtonType mouseButtonType)
{
	CharacterUiState &state = CharacterUI::state;
	Game &game = *state.game;
	game.setNextContext(CharacterEquipmentUI::ContextName);
}

void CharacterUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		CharacterUI::onDoneButtonSelected(MouseButtonType::Left);
	}
}
