#include "CharacterSheetUiMVC.h"
#include "GameWorldUiState.h"
#include "LevelUpUiMVC.h"
#include "LevelUpUiState.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../UI/FontLibrary.h"
#include "../UI/UiPivotType.h"

namespace
{
	constexpr char ContextName_RemainingPointsPopUp[] = "LevelUpRemainingPointsPopUp";

	constexpr char PlayerHealthTextBoxElementName[] = "LevelUpHealthTextBox";
	constexpr char PlayerStaminaTextBoxElementName[] = "LevelUpStaminaTextBox";
	constexpr char PlayerSpellPointsTextBoxElementName[] = "LevelUpSpellPointsTextBox";
	constexpr char BonusPointsTextBoxElementName[] = "LevelUpBonusPointsTextBox";

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;

	std::string GetPrimaryAttributeTextBoxElementName(const char *attributeName)
	{
		char elementName[64];
		std::snprintf(elementName, sizeof(elementName), "LevelUp%sTextBox", attributeName);
		return std::string(elementName);
	}

	std::string GetDerivedAttributeTextBoxElementName(const char *attributeName)
	{
		return GetPrimaryAttributeTextBoxElementName(attributeName);
	}

	std::string GetPrimaryAttributeUpDownButtonElementName(const char *attributeName, bool up)
	{
		char elementName[64];
		std::snprintf(elementName, sizeof(elementName), "LevelUp%s%sButton", attributeName, up ? "Up" : "Down");
		return std::string(elementName);
	}

	void OnPrimaryAttributeButtonSelected(int attributeIndex)
	{
		DebugAssert(attributeIndex >= 0);

		LevelUpUiState &state = LevelUpUI::state;
		state.selectedAttributeIndex = attributeIndex;

		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		const UiElementInstanceID attributeUpDownImageElementInstID = uiManager.getElementByName("LevelUpUpDownImage");
		const Int2 attributeUpDownPosition = LevelUpUiView::UpDownButtonFirstTopLeftPosition + Int2(0, attributeIndex * 8);
		uiManager.setTransformPosition(attributeUpDownImageElementInstID, attributeUpDownPosition);

		// Set only this attribute's up/down buttons active to avoid overlapping buttons.
		const PrimaryAttributes &primaryAttributes = game.charLevelUpState->attributes;
		const Span<const PrimaryAttribute> primaryAttributesView = primaryAttributes.getView();
		for (int i = 0; i < PrimaryAttributes::COUNT; i++)
		{
			const PrimaryAttribute &attribute = primaryAttributesView[i];
			const std::string attributeUpButtonElementName = GetPrimaryAttributeUpDownButtonElementName(attribute.name, true);
			const std::string attributeDownButtonElementName = GetPrimaryAttributeUpDownButtonElementName(attribute.name, false);
			const UiElementInstanceID attributeUpButtonElementInstID = uiManager.getElementByName(attributeUpButtonElementName.c_str());
			const UiElementInstanceID attributeDownButtonElementInstID = uiManager.getElementByName(attributeDownButtonElementName.c_str());

			const bool shouldSetButtonActive = i == attributeIndex;
			uiManager.setElementActive(attributeUpButtonElementInstID, shouldSetButtonActive);
			uiManager.setElementActive(attributeDownButtonElementInstID, shouldSetButtonActive);
		}
	}

	void OnPrimaryAttributeButtonUpDownSelected(int attributeIndex, bool up)
	{
		DebugAssert(attributeIndex >= 0);

		LevelUpUiState &state = LevelUpUI::state;
		if (state.selectedAttributeIndex != attributeIndex)
		{
			return;
		}

		Game &game = *state.game;
		CharacterLevelUpState &charLevelUpState = *game.charLevelUpState;
		Span<int> changedPoints = charLevelUpState.changedPoints;
		PrimaryAttributes &attributes = charLevelUpState.attributes;
		Span<PrimaryAttribute> attributesView = attributes.getView();
		PrimaryAttribute &attribute = attributesView[attributeIndex];

		if (up)
		{
			if (charLevelUpState.bonusPoints == 0)
			{
				return;
			}

			changedPoints[attributeIndex]++;
			charLevelUpState.bonusPoints--;
			attribute.maxValue++;
		}
		else
		{
			if (changedPoints[attributeIndex] == 0)
			{
				return;
			}

			changedPoints[attributeIndex]--;
			charLevelUpState.bonusPoints++;
			attribute.maxValue--;
		}

		UiManager &uiManager = game.uiManager;
		const std::string attributeTextBoxElementName = GetPrimaryAttributeTextBoxElementName(attribute.name);
		const UiElementInstanceID attributeTextBoxElementInstID = uiManager.getElementByName(attributeTextBoxElementName.c_str());
		const std::string newAttributeValueText = std::to_string(attribute.maxValue);
		uiManager.setTextBoxText(attributeTextBoxElementInstID, newAttributeValueText.c_str());

		const UiElementInstanceID bonusPointsTextBoxElementInstID = uiManager.getElementByName(BonusPointsTextBoxElementName);
		const std::string newBonusPointsValueText = std::to_string(charLevelUpState.bonusPoints);
		uiManager.setTextBoxText(bonusPointsTextBoxElementInstID, newBonusPointsValueText.c_str());

		LevelUpUI::updateDerivedAttributes();
	}
}

LevelUpUiState::LevelUpUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->remainingPointsPopUpContextInstID = -1;
	this->selectedAttributeIndex = -1;
}

void LevelUpUiState::init(Game &game)
{
	this->game = &game;
	this->selectedAttributeIndex = 0;
}

void LevelUpUI::create(Game &game)
{
	LevelUpUiState &state = LevelUpUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(LevelUpUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const std::string playerNameText = CharacterSheetUiModel::getPlayerName(game);
	const UiElementInstanceID playerNameTextBoxElementInstID = uiManager.getElementByName("LevelUpPlayerNameTextBox");
	uiManager.setTextBoxText(playerNameTextBoxElementInstID, playerNameText.c_str());

	const std::string playerRaceText = CharacterSheetUiModel::getPlayerRaceName(game);
	const UiElementInstanceID playerRaceTextBoxElementInstID = uiManager.getElementByName("LevelUpPlayerRaceTextBox");
	uiManager.setTextBoxText(playerRaceTextBoxElementInstID, playerRaceText.c_str());

	const std::string playerClassText = CharacterSheetUiModel::getPlayerClassName(game);
	const UiElementInstanceID playerClassTextBoxElementInstID = uiManager.getElementByName("LevelUpPlayerClassTextBox");
	uiManager.setTextBoxText(playerClassTextBoxElementInstID, playerClassText.c_str());

	const std::string playerExperienceText = CharacterSheetUiModel::getPlayerExperience(game);
	const UiElementInstanceID experienceTextBoxElementInstID = uiManager.getElementByName("LevelUpExperienceTextBox");
	uiManager.setTextBoxText(experienceTextBoxElementInstID, playerExperienceText.c_str());

	const std::string playerLevelText = CharacterSheetUiModel::getPlayerLevel(game);
	const UiElementInstanceID levelTextBoxElementInstID = uiManager.getElementByName("LevelUpLevelTextBox");
	uiManager.setTextBoxText(levelTextBoxElementInstID, playerLevelText.c_str());

	LevelUpUI::updatePrimaryAttributes();
	LevelUpUI::updateDerivedAttributes();
	LevelUpUI::updateGold();
	LevelUpUI::updateBonusPoints();

	const CharacterEquipmentPresentationState equipmentPresentationState = CharacterSheetUiView::getEquipmentPresentationState(game);

	UiElementInitInfo playerBackgroundImageElementInitInfo;
	playerBackgroundImageElementInitInfo.name = "LevelUpPlayerBackground";
	playerBackgroundImageElementInitInfo.position = equipmentPresentationState.bodyPosition;
	playerBackgroundImageElementInitInfo.drawOrder = 0;
	uiManager.createImage(playerBackgroundImageElementInitInfo, equipmentPresentationState.bodyTextureID, state.contextInstID, renderer);

	UiElementInitInfo playerHeadImageElementInitInfo;
	playerHeadImageElementInitInfo.name = "LevelUpPlayerHead";
	playerHeadImageElementInitInfo.position = equipmentPresentationState.headPosition;
	playerHeadImageElementInitInfo.drawOrder = 1;
	uiManager.createImage(playerHeadImageElementInitInfo, equipmentPresentationState.headTextureID, state.contextInstID, renderer);

	UiElementInitInfo playerPantsImageElementInitInfo;
	playerPantsImageElementInitInfo.name = "LevelUpPlayerPants";
	playerPantsImageElementInitInfo.position = equipmentPresentationState.pantsPosition;
	playerPantsImageElementInitInfo.drawOrder = 2;
	uiManager.createImage(playerPantsImageElementInitInfo, equipmentPresentationState.pantsTextureID, state.contextInstID, renderer);

	UiElementInitInfo playerShirtImageElementInitInfo;
	playerShirtImageElementInitInfo.name = "LevelUpPlayerShirt";
	playerShirtImageElementInitInfo.position = equipmentPresentationState.shirtPosition;
	playerShirtImageElementInitInfo.drawOrder = 3;
	uiManager.createImage(playerShirtImageElementInitInfo, equipmentPresentationState.shirtTextureID, state.contextInstID, renderer);

	// Remaining points popup.
	UiContextInitInfo remainingPointsPopUpContextInitInfo;
	remainingPointsPopUpContextInitInfo.name = ContextName_RemainingPointsPopUp;
	remainingPointsPopUpContextInitInfo.drawOrder = 1;
	state.remainingPointsPopUpContextInstID = uiManager.createContext(remainingPointsPopUpContextInitInfo);

	UiElementInitInfo remainingPointsPopUpTextBoxElementInitInfo;
	remainingPointsPopUpTextBoxElementInitInfo.name = "LevelUpRemainingPointsPopUpTextBox";
	remainingPointsPopUpTextBoxElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
	remainingPointsPopUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	remainingPointsPopUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo remainingPointsPopUpTextBoxInitInfo;
	remainingPointsPopUpTextBoxInitInfo.text = LevelUpUiModel::getBonusPointsRemainingText(game);
	remainingPointsPopUpTextBoxInitInfo.fontName = LevelUpUiView::AppearanceTextFontName;
	remainingPointsPopUpTextBoxInitInfo.defaultColor = LevelUpUiView::AppearanceTextColor;
	remainingPointsPopUpTextBoxInitInfo.alignment = LevelUpUiView::AppearanceTextAlignment;
	remainingPointsPopUpTextBoxInitInfo.lineSpacing = LevelUpUiView::AppearanceTextLineSpacing;
	const UiElementInstanceID remainingPointsPopUpTextBoxElementInstID = uiManager.createTextBox(remainingPointsPopUpTextBoxElementInitInfo, remainingPointsPopUpTextBoxInitInfo, state.remainingPointsPopUpContextInstID, renderer);
	const Rect remainingPointsPopUpTextBoxRect = uiManager.getTransformGlobalRect(remainingPointsPopUpTextBoxElementInstID);

	UiElementInitInfo remainingPointsPopUpImageElementInitInfo;
	remainingPointsPopUpImageElementInitInfo.name = "LevelUpRemainingPointsPopUpImage";
	remainingPointsPopUpImageElementInitInfo.position = LevelUpUiView::AppearanceTextCenterPoint;
	remainingPointsPopUpImageElementInitInfo.pivotType = UiPivotType::Middle;
	remainingPointsPopUpImageElementInitInfo.drawOrder = 0;

	const int remainingPointsPopUpImageTextureWidth = LevelUpUiView::getRemainingPointsTextBoxTextureWidth(remainingPointsPopUpTextBoxRect.width);
	const int remainingPointsPopUpImageTextureHeight = LevelUpUiView::getRemainingPointsTextBoxTextureWidth(remainingPointsPopUpTextBoxRect.height);
	const UiTextureID remainingPointsPopUpImageTextureID = uiManager.getOrAddTexture(LevelUpUiView::AppearanceTextPatternType, remainingPointsPopUpImageTextureWidth, remainingPointsPopUpImageTextureHeight, textureManager, renderer);
	uiManager.createImage(remainingPointsPopUpImageElementInitInfo, remainingPointsPopUpImageTextureID, state.remainingPointsPopUpContextInstID, renderer);

	UiElementInitInfo remainingPointsPopUpBackButtonElementInitInfo;
	remainingPointsPopUpBackButtonElementInitInfo.name = "LevelUpRemainingPointsPopUpBackButton";
	remainingPointsPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	remainingPointsPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	remainingPointsPopUpBackButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo remainingPointsPopUpBackButtonInitInfo;
	remainingPointsPopUpBackButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	remainingPointsPopUpBackButtonInitInfo.callback = LevelUpUI::onRemainingPointsPopUpBackButtonSelected;
	uiManager.createButton(remainingPointsPopUpBackButtonElementInitInfo, remainingPointsPopUpBackButtonInitInfo, state.remainingPointsPopUpContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, LevelUpUI::onRemainingPointsPopUpBackInputAction, ContextName_RemainingPointsPopUp, inputManager);

	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, true);

	uiManager.setContextEnabled(state.remainingPointsPopUpContextInstID, false);

	// Default to first attribute.
	OnPrimaryAttributeButtonSelected(0);
}

void LevelUpUI::destroy()
{
	LevelUpUiState &state = LevelUpUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	if (state.remainingPointsPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.remainingPointsPopUpContextInstID, inputManager, renderer);
		state.remainingPointsPopUpContextInstID = -1;
	}

	state.selectedAttributeIndex = -1;

	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, false);
}

void LevelUpUI::update(double dt)
{
	// Do nothing.
	static_cast<void>(dt);
}

void LevelUpUI::updatePrimaryAttributes()
{
	LevelUpUiState &state = LevelUpUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const CharacterLevelUpState &charLevelUpState = *game.charLevelUpState;
	Span<const PrimaryAttribute> playerAttributesView = charLevelUpState.attributes.getView();
	for (const PrimaryAttribute &attribute : playerAttributesView)
	{
		const std::string attributeElementName = GetPrimaryAttributeTextBoxElementName(attribute.name);
		const UiElementInstanceID attributeTextBoxElementInstID = uiManager.getElementByName(attributeElementName.c_str());

		const std::string attributeValueText = std::to_string(attribute.maxValue);
		uiManager.setTextBoxText(attributeTextBoxElementInstID, attributeValueText.c_str());
	}
}

void LevelUpUI::updateDerivedAttributes()
{
	LevelUpUiState &state = LevelUpUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	CharacterLevelUpState &charLevelUpState = *game.charLevelUpState;
	const PrimaryAttributes &primaryAttributes = charLevelUpState.attributes;
	const Player &player = game.player;
	charLevelUpState.maxStamina = ArenaPlayerUtils::calculateMaxStamina(primaryAttributes.strength.maxValue, primaryAttributes.endurance.maxValue);
	charLevelUpState.maxSpellPoints = ArenaPlayerUtils::calculateMaxSpellPoints(player.charClassDefID, primaryAttributes.intelligence.maxValue);
	charLevelUpState.derivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(primaryAttributes);

	const Span<const int> derivedAttributesView = charLevelUpState.derivedAttributes.getView();
	for (int i = 0; i < DerivedAttributes::COUNT; i++)
	{
		const int derivedAttributeValue = derivedAttributesView[i];
		const std::string derivedAttributeDisplayString = DerivedAttributes::isModifier(i) ? CharacterSheetUiModel::getDerivedAttributeDisplayString(derivedAttributeValue) : std::to_string(derivedAttributeValue);

		DebugAssertIndex(CharacterSheetUiModel::DerivedAttributeUiNames, i);
		const std::string derivedAttributeElementName = GetDerivedAttributeTextBoxElementName(CharacterSheetUiModel::DerivedAttributeUiNames[i]);
		const UiElementInstanceID derivedAttributeTextBoxElementInstID = uiManager.getElementByName(derivedAttributeElementName.c_str());
		uiManager.setTextBoxText(derivedAttributeTextBoxElementInstID, derivedAttributeDisplayString.c_str());
	}

	const UiElementInstanceID healthTextBoxElementInstID = uiManager.getElementByName(PlayerHealthTextBoxElementName);
	const std::string healthValueText = LevelUpUiModel::getPlayerHealth(game);
	uiManager.setTextBoxText(healthTextBoxElementInstID, healthValueText.c_str());

	const UiElementInstanceID staminaTextBoxElementInstID = uiManager.getElementByName(PlayerStaminaTextBoxElementName);
	const std::string staminaValueText = LevelUpUiModel::getPlayerStamina(game);
	uiManager.setTextBoxText(staminaTextBoxElementInstID, staminaValueText.c_str());

	const UiElementInstanceID spellPointsTextBoxElementInstID = uiManager.getElementByName(PlayerSpellPointsTextBoxElementName);
	const std::string spellPointsValueText = LevelUpUiModel::getPlayerSpellPoints(game);
	uiManager.setTextBoxText(spellPointsTextBoxElementInstID, spellPointsValueText.c_str());
}

void LevelUpUI::updateGold()
{
	LevelUpUiState &state = LevelUpUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const std::string playerGoldText = CharacterSheetUiModel::getPlayerGold(game);
	const UiElementInstanceID goldTextBoxElementInstID = uiManager.getElementByName("LevelUpGoldTextBox");
	uiManager.setTextBoxText(goldTextBoxElementInstID, playerGoldText.c_str());
}

void LevelUpUI::updateBonusPoints()
{
	LevelUpUiState &state = LevelUpUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	CharacterLevelUpState &charLevelUpState = *game.charLevelUpState;
	const std::string bonusPointsText = std::to_string(charLevelUpState.bonusPoints);
	const UiElementInstanceID bonusPointsTextBoxElementInstID = uiManager.getElementByName(BonusPointsTextBoxElementName);
	uiManager.setTextBoxText(bonusPointsTextBoxElementInstID, bonusPointsText.c_str());
}

void LevelUpUI::onStrengthButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(0);
}

void LevelUpUI::onIntelligenceButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(1);
}

void LevelUpUI::onWillpowerButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(2);
}

void LevelUpUI::onAgilityButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(3);
}

void LevelUpUI::onSpeedButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(4);
}

void LevelUpUI::onEnduranceButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(5);
}

void LevelUpUI::onPersonalityButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(6);
}

void LevelUpUI::onLuckButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(7);
}

void LevelUpUI::onStrengthUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(0, true);
}

void LevelUpUI::onStrengthDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(0, false);
}

void LevelUpUI::onIntelligenceUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(1, true);
}

void LevelUpUI::onIntelligenceDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(1, false);
}

void LevelUpUI::onWillpowerUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(2, true);
}

void LevelUpUI::onWillpowerDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(2, false);
}

void LevelUpUI::onAgilityUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(3, true);
}

void LevelUpUI::onAgilityDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(3, false);
}

void LevelUpUI::onSpeedUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(4, true);
}

void LevelUpUI::onSpeedDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(4, false);
}

void LevelUpUI::onEnduranceUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(5, true);
}

void LevelUpUI::onEnduranceDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(5, false);
}

void LevelUpUI::onPersonalityUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(6, true);
}

void LevelUpUI::onPersonalityDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(6, false);
}

void LevelUpUI::onLuckUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(7, true);
}

void LevelUpUI::onLuckDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(7, false);
}

void LevelUpUI::onDoneButtonSelected(MouseButtonType mouseButtonType)
{
	LevelUpUiState &state = LevelUpUI::state;
	Game &game = *state.game;
	const CharacterLevelUpState &charLevelUpState = *game.charLevelUpState;
	const bool anyBonusPointsRemaining = charLevelUpState.bonusPoints > 0;

	if (anyBonusPointsRemaining)
	{
		UiManager &uiManager = game.uiManager;
		uiManager.setContextEnabled(state.remainingPointsPopUpContextInstID, true);
	}
	else
	{
		Player &player = game.player;
		Span<const PrimaryAttribute> sourceAttributes = charLevelUpState.attributes.getView();
		Span<PrimaryAttribute> destinationAttributes = player.primaryAttributes.getView();

		for (int i = 0; i < PrimaryAttributes::COUNT; i++)
		{			
			destinationAttributes[i].maxValue = sourceAttributes[i].maxValue;
		}

		DebugLogWarning("@todo level up logic is incomplete. Grant more max health based on endurance?");
		player.maxHealth = charLevelUpState.maxHealth;
		player.maxStamina = charLevelUpState.maxStamina;
		player.maxSpellPoints = charLevelUpState.maxSpellPoints;
		game.charLevelUpState = nullptr;

		game.setNextContext(GameWorldUI::ContextName);
	}
}

void LevelUpUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		LevelUpUI::onDoneButtonSelected(MouseButtonType::Left);
	}
}

void LevelUpUI::onCharacterSheetInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		LevelUpUI::onDoneButtonSelected(MouseButtonType::Left);
	}
}

void LevelUpUI::onRemainingPointsPopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	LevelUpUiState &state = LevelUpUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
}

void LevelUpUI::onRemainingPointsPopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		LevelUpUI::onRemainingPointsPopUpBackButtonSelected(MouseButtonType::Left);
	}
}
