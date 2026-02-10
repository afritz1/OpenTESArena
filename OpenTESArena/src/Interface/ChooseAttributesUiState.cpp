#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "ChooseAttributesUiState.h"
#include "ChooseRacePanel.h"
#include "../Game/Game.h"

namespace
{
	constexpr const char *DerivedAttributeUiNames[] =
	{
		"BonusDamage",
		"MaxWeight",
		"MagicDefense",
		"BonusToHit",
		"BonusToDefend",
		"BonusToHealth",
		"HealMod",
		"BonusToCharisma",
	};

	constexpr char PlayerHealthTextBoxElementName[] = "ChooseAttributesHealthTextBox";
	constexpr char PlayerStaminaTextBoxElementName[] = "ChooseAttributesStaminaTextBox";
	constexpr char PlayerSpellPointsTextBoxElementName[] = "ChooseAttributesSpellPointsTextBox";
	constexpr char BonusPointsTextBoxElementName[] = "ChooseAttributesBonusPointsTextBox";

	std::string GetPrimaryAttributeTextBoxElementName(const char *attributeName)
	{
		char elementName[64];
		std::snprintf(elementName, sizeof(elementName), "ChooseAttributes%sTextBox", attributeName);
		return std::string(elementName);
	}

	std::string GetDerivedAttributeTextBoxElementName(const char *attributeName)
	{
		return GetPrimaryAttributeTextBoxElementName(attributeName);
	}

	std::string GetPrimaryAttributeUpDownButtonElementName(const char *attributeName, bool up)
	{
		char elementName[64];
		std::snprintf(elementName, sizeof(elementName), "ChooseAttributes%s%sButton", attributeName, up ? "Up" : "Down");
		return std::string(elementName);
	}

	void OnPrimaryAttributeButtonSelected(int attributeIndex)
	{
		DebugAssert(attributeIndex >= 0);

		ChooseAttributesUiState &state = ChooseAttributesUI::state;
		if (state.attributesAreSaved)
		{
			return;
		}

		state.selectedAttributeIndex = attributeIndex;

		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		const UiElementInstanceID attributeUpDownImageElementInstID = uiManager.getElementByName("ChooseAttributesUpDownImage");
		const Int2 attributeUpDownPosition = ChooseAttributesUiView::UpDownButtonFirstTopLeftPosition + Int2(0, attributeIndex * 8);
		uiManager.setTransformPosition(attributeUpDownImageElementInstID, attributeUpDownPosition);

		// Set only this attribute's up/down buttons active to avoid overlapping buttons.
		for (int i = 0; i < PrimaryAttributes::COUNT; i++)
		{
			CharacterCreationState &charCreationState = *game.charCreationState;
			const PrimaryAttribute &attribute = charCreationState.attributes.getView()[i];
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

		ChooseAttributesUiState &state = ChooseAttributesUI::state;
		if (state.attributesAreSaved)
		{
			return;
		}

		if (state.selectedAttributeIndex != attributeIndex)
		{
			return;
		}

		Game &game = *state.game;
		CharacterCreationState &charCreationState = *game.charCreationState;
		Span<int> changedPoints = charCreationState.changedPoints;
		PrimaryAttributes &attributes = charCreationState.attributes;
		Span<PrimaryAttribute> attributesView = attributes.getView();
		PrimaryAttribute &attribute = attributesView[attributeIndex];

		if (up)
		{
			if (charCreationState.bonusPoints == 0)
			{
				return;
			}

			changedPoints[attributeIndex]++;
			charCreationState.bonusPoints--;
			attribute.maxValue++;
		}
		else
		{
			if (changedPoints[attributeIndex] == 0)
			{
				return;
			}

			changedPoints[attributeIndex]--;
			charCreationState.bonusPoints++;
			attribute.maxValue--;
		}

		UiManager &uiManager = game.uiManager;
		const std::string attributeTextBoxElementName = GetPrimaryAttributeTextBoxElementName(attribute.name);
		const UiElementInstanceID attributeTextBoxElementInstID = uiManager.getElementByName(attributeTextBoxElementName.c_str());
		const std::string newAttributeValueText = std::to_string(attribute.maxValue);
		uiManager.setTextBoxText(attributeTextBoxElementInstID, newAttributeValueText.c_str());

		const UiElementInstanceID bonusPointsTextBoxElementInstID = uiManager.getElementByName(BonusPointsTextBoxElementName);
		const std::string newBonusPointsValueText = std::to_string(charCreationState.bonusPoints);
		uiManager.setTextBoxText(bonusPointsTextBoxElementInstID, newBonusPointsValueText.c_str());

		ChooseAttributesUI::updateDerivedAttributeValues();
	}
}

ChooseAttributesUiState::ChooseAttributesUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->selectedAttributeIndex = -1;
	this->attributesAreSaved = false;
}

void ChooseAttributesUiState::init(Game &game)
{
	this->game = &game;
	this->headTextureIDs.clear();
	this->selectedAttributeIndex = 0;
	this->attributesAreSaved = false;
}

void ChooseAttributesUI::create(Game &game)
{
	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.portraitIndex = 0;
	charCreationState.clearChangedPoints();

	ArenaRandom &arenaRandom = game.arenaRandom;
	ChooseAttributesUI::populateBaseAttributesRandomly(charCreationState, arenaRandom);

	Random &random = game.random;
	const PrimaryAttributes &primaryAttributes = charCreationState.attributes;
	charCreationState.derivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(primaryAttributes);
	charCreationState.maxHealth = ArenaPlayerUtils::calculateMaxHealthPoints(charCreationState.classDefID, random);
	charCreationState.maxStamina = ArenaPlayerUtils::calculateMaxStamina(primaryAttributes.strength.maxValue, primaryAttributes.endurance.maxValue);
	charCreationState.maxSpellPoints = ArenaPlayerUtils::calculateMaxSpellPoints(charCreationState.classDefID, primaryAttributes.intelligence.maxValue);
	charCreationState.gold = ArenaPlayerUtils::calculateStartingGold(random);
	charCreationState.bonusPoints = ChooseAttributesUiModel::rollClassic(ChooseAttributesUiModel::BonusPointsRandomMax, arenaRandom);

	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseAttributesUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const std::string playerNameText = CharacterCreationUiModel::getPlayerName(game);
	const UiElementInstanceID playerNameTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesPlayerNameTextBox");
	uiManager.setTextBoxText(playerNameTextBoxElementInstID, playerNameText.c_str());

	const std::string playerRaceText = CharacterCreationUiModel::getPlayerRaceName(game);
	const UiElementInstanceID playerRaceTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesPlayerRaceTextBox");
	uiManager.setTextBoxText(playerRaceTextBoxElementInstID, playerRaceText.c_str());

	const std::string playerClassText = CharacterCreationUiModel::getPlayerClassName(game);
	const UiElementInstanceID playerClassTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesPlayerClassTextBox");
	uiManager.setTextBoxText(playerClassTextBoxElementInstID, playerClassText.c_str());

	const Span<const PrimaryAttribute> playerAttributesView = primaryAttributes.getView();
	for (const PrimaryAttribute &attribute : playerAttributesView)
	{
		const std::string attributeElementName = GetPrimaryAttributeTextBoxElementName(attribute.name);
		const UiElementInstanceID attributeTextBoxElementInstID = uiManager.getElementByName(attributeElementName.c_str());

		const std::string attributeValueText = std::to_string(attribute.maxValue);
		uiManager.setTextBoxText(attributeTextBoxElementInstID, attributeValueText.c_str());
	}

	Span<const int> playerDerivedAttributesView = charCreationState.derivedAttributes.getView();
	for (int i = 0; i < playerDerivedAttributesView.getCount(); i++)
	{
		DebugAssertIndex(DerivedAttributeUiNames, i);
		const std::string derivedAttributeElementName = GetDerivedAttributeTextBoxElementName(DerivedAttributeUiNames[i]);
		const UiElementInstanceID derivedAttributeTextBoxElementInstID = uiManager.getElementByName(derivedAttributeElementName.c_str());

		const int derivedAttributeValue = playerDerivedAttributesView[i];
		const std::string derivedAttributeValueText = DerivedAttributes::isModifier(i) ? CharacterSheetUiModel::getDerivedAttributeDisplayString(derivedAttributeValue) : std::to_string(derivedAttributeValue);
		uiManager.setTextBoxText(derivedAttributeTextBoxElementInstID, derivedAttributeValueText.c_str());
	}

	const std::string playerExperienceText = CharacterCreationUiModel::getPlayerExperience(game);
	const UiElementInstanceID experienceTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesExperienceTextBox");
	uiManager.setTextBoxText(experienceTextBoxElementInstID, playerExperienceText.c_str());

	const std::string playerLevelText = CharacterCreationUiModel::getPlayerLevel(game);
	const UiElementInstanceID levelTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesLevelTextBox");
	uiManager.setTextBoxText(levelTextBoxElementInstID, playerLevelText.c_str());

	const std::string playerHealthText = ChooseAttributesUiModel::getPlayerHealth(game);
	const UiElementInstanceID healthTextBoxElementInstID = uiManager.getElementByName(PlayerHealthTextBoxElementName);
	uiManager.setTextBoxText(healthTextBoxElementInstID, playerHealthText.c_str());

	const std::string playerStaminaText = ChooseAttributesUiModel::getPlayerStamina(game);
	const UiElementInstanceID staminaTextBoxElementInstID = uiManager.getElementByName(PlayerStaminaTextBoxElementName);
	uiManager.setTextBoxText(staminaTextBoxElementInstID, playerStaminaText.c_str());

	const std::string playerSpellPointsText = ChooseAttributesUiModel::getPlayerSpellPoints(game);
	const UiElementInstanceID spellPointsTextBoxElementInstID = uiManager.getElementByName(PlayerSpellPointsTextBoxElementName);
	uiManager.setTextBoxText(spellPointsTextBoxElementInstID, playerSpellPointsText.c_str());

	const std::string playerGoldText = ChooseAttributesUiModel::getPlayerGold(game);
	const UiElementInstanceID goldTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesGoldTextBox");
	uiManager.setTextBoxText(goldTextBoxElementInstID, playerGoldText.c_str());

	const std::string bonusPointsText = std::to_string(charCreationState.bonusPoints);
	const UiElementInstanceID bonusPointsTextBoxElementInstID = uiManager.getElementByName(BonusPointsTextBoxElementName);
	uiManager.setTextBoxText(bonusPointsTextBoxElementInstID, bonusPointsText.c_str());

	UiElementInitInfo playerBackgroundImageElementInitInfo;
	playerBackgroundImageElementInitInfo.name = "ChooseAttributesPlayerBackground";
	playerBackgroundImageElementInitInfo.drawOrder = 0;

	const TextureAsset bodyTextureAsset = ChooseAttributesUiView::getBodyTextureAsset(game);
	const TextureAsset bodyPaletteTextureAsset = CharacterSheetUiView::getPaletteTextureAsset();
	const UiTextureID playerBackgroundTextureID = uiManager.getOrAddTexture(bodyTextureAsset, bodyPaletteTextureAsset, textureManager, renderer);
	uiManager.createImage(playerBackgroundImageElementInitInfo, playerBackgroundTextureID, state.contextInstID, renderer);

	UiElementInitInfo playerHeadImageElementInitInfo;
	playerHeadImageElementInitInfo.name = "ChooseAttributesPlayerHead";
	playerHeadImageElementInitInfo.position = ChooseAttributesUiView::getHeadOffset(game);
	playerHeadImageElementInitInfo.drawOrder = 1;

	const Buffer<TextureAsset> headTextureAssets = ChooseAttributesUiView::getHeadTextureAssets(game);
	state.headTextureIDs.init(headTextureAssets.getCount());
	for (int i = 0; i < headTextureAssets.getCount(); i++)
	{
		const TextureAsset &headTextureAsset = headTextureAssets[i];
		state.headTextureIDs[i] = uiManager.getOrAddTexture(headTextureAsset, bodyPaletteTextureAsset, textureManager, renderer);
	}

	uiManager.createImage(playerHeadImageElementInitInfo, state.headTextureIDs[0], state.contextInstID, renderer);

	UiElementInitInfo playerPantsImageElementInitInfo;
	playerPantsImageElementInitInfo.name = "ChooseAttributesPlayerPants";
	playerPantsImageElementInitInfo.position = ChooseAttributesUiView::getPantsOffset(game);
	playerPantsImageElementInitInfo.drawOrder = 2;

	const TextureAsset pantsTextureAsset = ChooseAttributesUiView::getPantsTextureAsset(game);
	const UiTextureID pantsTextureID = uiManager.getOrAddTexture(pantsTextureAsset, bodyPaletteTextureAsset, textureManager, renderer);
	uiManager.createImage(playerPantsImageElementInitInfo, pantsTextureID, state.contextInstID, renderer);

	UiElementInitInfo playerShirtImageElementInitInfo;
	playerShirtImageElementInitInfo.name = "ChooseAttributesPlayerShirt";
	playerShirtImageElementInitInfo.position = ChooseAttributesUiView::getShirtOffset(game);
	playerShirtImageElementInitInfo.drawOrder = 3;

	const TextureAsset shirtTextureAsset = ChooseAttributesUiView::getShirtTextureAsset(game);
	const UiTextureID shirtTextureID = uiManager.getOrAddTexture(shirtTextureAsset, bodyPaletteTextureAsset, textureManager, renderer);
	uiManager.createImage(playerShirtImageElementInitInfo, shirtTextureID, state.contextInstID, renderer);

	// Default to first attribute.
	OnPrimaryAttributeButtonSelected(0);

	// @todo initial text popup (also shows when rerolling)
	
	// @todo Done button functionality (Save/Reroll) when !attributesAreSaved
	// - remember hotkeys
	
	// @todo Done button functionality when attributesAreSaved

	/*
	// Push the initial text pop-up onto the sub-panel stack.
	const std::string initialPopUpText = ChooseAttributesUiModel::getInitialText(game);
	const TextBoxInitInfo initialPopUpTextBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		initialPopUpText,
		ChooseAttributesUiView::InitialTextCenterPoint,
		ChooseAttributesUiView::InitialTextFontName,
		ChooseAttributesUiView::InitialTextColor,
		ChooseAttributesUiView::InitialTextAlignment,
		std::nullopt,
		ChooseAttributesUiView::InitialTextLineSpacing,
		fontLibrary);

	const Surface initialPopUpSurface = TextureUtils::generate(
		ChooseAttributesUiView::InitialTextPatternType,
		ChooseAttributesUiView::getDistributePointsTextBoxTextureWidth(initialPopUpTextBoxInitInfo.rect.width),
		ChooseAttributesUiView::getDistributePointsTextBoxTextureHeight(initialPopUpTextBoxInitInfo.rect.height),
		textureManager,
		renderer);

	UiTextureID initialPopUpTextureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(initialPopUpSurface, textureManager, renderer, &initialPopUpTextureID))
	{
		DebugCrash("Couldn't create initial pop-up texture.");
	}

	ScopedUiTextureRef initialPopUpTextureRef(initialPopUpTextureID, renderer);
	game.pushSubPanel<TextSubPanel>(initialPopUpTextBoxInitInfo, initialPopUpText,
		ChooseAttributesUiController::onInitialPopUpSelected, std::move(initialPopUpTextureRef),
		ChooseAttributesUiView::InitialTextureCenterPoint);
	*/
}

void ChooseAttributesUI::destroy()
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, game.inputManager, game.renderer);
		state.contextInstID = -1;
	}

	state.attributesAreSaved = false;
	state.selectedAttributeIndex = -1;
}

void ChooseAttributesUI::update(double dt)
{
	// Do nothing.
	static_cast<void>(dt);
}

void ChooseAttributesUI::populateBaseAttributesRandomly(CharacterCreationState &charCreationState, ArenaRandom &random)
{
	charCreationState.populateBaseAttributes();

	Span<PrimaryAttribute> attributes = charCreationState.attributes.getView();
	for (int i = 0; i < PrimaryAttributes::COUNT; i++)
	{
		PrimaryAttribute &attribute = attributes[i];
		const int addedValue = ChooseAttributesUiModel::rollClassic(ChooseAttributesUiModel::PrimaryAttributeRandomMax, random);
		attribute.maxValue += addedValue;
	}
}

void ChooseAttributesUI::updateDerivedAttributeValues()
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const PrimaryAttributes &primaryAttributes = charCreationState.attributes;
	charCreationState.maxStamina = ArenaPlayerUtils::calculateMaxStamina(primaryAttributes.strength.maxValue, primaryAttributes.endurance.maxValue);
	charCreationState.maxSpellPoints = ArenaPlayerUtils::calculateMaxSpellPoints(charCreationState.classDefID, primaryAttributes.intelligence.maxValue);
	charCreationState.derivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(primaryAttributes);

	const Span<const int> derivedAttributesView = charCreationState.derivedAttributes.getView();
	for (int i = 0; i < DerivedAttributes::COUNT; i++)
	{
		const int derivedAttributeValue = derivedAttributesView[i];
		const std::string derivedAttributeDisplayString = DerivedAttributes::isModifier(i) ? CharacterSheetUiModel::getDerivedAttributeDisplayString(derivedAttributeValue) : std::to_string(derivedAttributeValue);

		DebugAssertIndex(DerivedAttributeUiNames, i);
		const std::string derivedAttributeElementName = GetDerivedAttributeTextBoxElementName(DerivedAttributeUiNames[i]);
		const UiElementInstanceID derivedAttributeTextBoxElementInstID = uiManager.getElementByName(derivedAttributeElementName.c_str());
		uiManager.setTextBoxText(derivedAttributeTextBoxElementInstID, derivedAttributeDisplayString.c_str());
	}

	const UiElementInstanceID healthTextBoxElementInstID = uiManager.getElementByName(PlayerHealthTextBoxElementName);
	const std::string healthValueText = ChooseAttributesUiModel::getPlayerHealth(game);
	uiManager.setTextBoxText(healthTextBoxElementInstID, healthValueText.c_str());

	const UiElementInstanceID staminaTextBoxElementInstID = uiManager.getElementByName(PlayerStaminaTextBoxElementName);
	const std::string staminaValueText = ChooseAttributesUiModel::getPlayerStamina(game);
	uiManager.setTextBoxText(staminaTextBoxElementInstID, staminaValueText.c_str());

	const UiElementInstanceID spellPointsTextBoxElementInstID = uiManager.getElementByName(PlayerSpellPointsTextBoxElementName);
	const std::string spellPointsValueText = ChooseAttributesUiModel::getPlayerSpellPoints(game);
	uiManager.setTextBoxText(spellPointsTextBoxElementInstID, spellPointsValueText.c_str());
}

void ChooseAttributesUI::onStrengthButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(0);
}

void ChooseAttributesUI::onIntelligenceButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(1);
}

void ChooseAttributesUI::onWillpowerButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(2);
}

void ChooseAttributesUI::onAgilityButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(3);
}

void ChooseAttributesUI::onSpeedButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(4);
}

void ChooseAttributesUI::onEnduranceButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(5);
}

void ChooseAttributesUI::onPersonalityButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(6);
}

void ChooseAttributesUI::onLuckButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonSelected(7);
}

void ChooseAttributesUI::onStrengthUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(0, true);
}

void ChooseAttributesUI::onStrengthDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(0, false);
}

void ChooseAttributesUI::onIntelligenceUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(1, true);
}

void ChooseAttributesUI::onIntelligenceDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(1, false);
}

void ChooseAttributesUI::onWillpowerUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(2, true);
}

void ChooseAttributesUI::onWillpowerDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(2, false);
}

void ChooseAttributesUI::onAgilityUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(3, true);
}

void ChooseAttributesUI::onAgilityDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(3, false);
}

void ChooseAttributesUI::onSpeedUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(4, true);
}

void ChooseAttributesUI::onSpeedDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(4, false);
}

void ChooseAttributesUI::onEnduranceUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(5, true);
}

void ChooseAttributesUI::onEnduranceDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(5, false);
}

void ChooseAttributesUI::onPersonalityUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(6, true);
}

void ChooseAttributesUI::onPersonalityDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(6, false);
}

void ChooseAttributesUI::onLuckUpButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(7, true);
}

void ChooseAttributesUI::onLuckDownButtonSelected(MouseButtonType mouseButtonType)
{
	OnPrimaryAttributeButtonUpDownSelected(7, false);
}

void ChooseAttributesUI::onPortraitButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	if (!state.attributesAreSaved)
	{
		return;
	}

	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	const bool shouldIncrementPortraitIndex = mouseButtonType != MouseButtonType::Right;
	const int maxPortraitIndex = state.headTextureIDs.getCount() - 1;

	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	const int oldPortraitIndex = charCreationState.portraitIndex;
	const int newPortraitIndex = shouldIncrementPortraitIndex ?
		((oldPortraitIndex == maxPortraitIndex) ? 0 : (oldPortraitIndex + 1)) :
		((oldPortraitIndex == 0) ? maxPortraitIndex : (oldPortraitIndex - 1));

	charCreationState.portraitIndex = newPortraitIndex;

	const UiElementInstanceID headImageElementInstID = uiManager.getElementByName("ChooseAttributesPlayerHead");
	const UiTextureID newHeadTextureID = state.headTextureIDs[newPortraitIndex];
	uiManager.setTransformPosition(headImageElementInstID, ChooseAttributesUiView::getHeadOffset(game));
	uiManager.setImageTexture(headImageElementInstID, newHeadTextureID);
}

void ChooseAttributesUI::onDoneButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	ChooseAttributesUiController::onDoneButtonSelected(game, charCreationState.bonusPoints, &state.attributesAreSaved);
}

void ChooseAttributesUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setPanel<ChooseRacePanel>();
	}
}
