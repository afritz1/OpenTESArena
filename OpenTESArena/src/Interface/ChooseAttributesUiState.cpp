#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "ChooseAttributesUiState.h"
#include "ChooseRacePanel.h"
#include "TextCinematicPanel.h"
#include "TextCinematicUiState.h"
#include "TextSubPanel.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Interface/CinematicLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../World/CardinalDirection.h"
#include "../WorldMap/ArenaLocationUtils.h"

namespace
{
	constexpr char ContextName_InitialPopUp[] = "ChooseAttributesInitialPopUp";
	constexpr char ContextName_SaveReroll[] = "ChooseAttributesSaveReroll";
	constexpr char ContextName_RemainingPointsPopUp[] = "ChooseAttributesRemainingPointsPopUp";
	constexpr char ContextName_PortraitPopUp[] = "ChooseAttributesPortraitPopUp";

	constexpr char PlayerHealthTextBoxElementName[] = "ChooseAttributesHealthTextBox";
	constexpr char PlayerStaminaTextBoxElementName[] = "ChooseAttributesStaminaTextBox";
	constexpr char PlayerSpellPointsTextBoxElementName[] = "ChooseAttributesSpellPointsTextBox";
	constexpr char BonusPointsTextBoxElementName[] = "ChooseAttributesBonusPointsTextBox";

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;

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

		const CharacterCreationState &charCreationState = *game.charCreationState;

		// Set only this attribute's up/down buttons active to avoid overlapping buttons.
		const Span<const PrimaryAttribute> primaryAttributes = charCreationState.attributes.getView();
		for (int i = 0; i < PrimaryAttributes::COUNT; i++)
		{
			const PrimaryAttribute &attribute = primaryAttributes[i];
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

		ChooseAttributesUI::updateDerivedAttributes();
	}
}

ChooseAttributesUiState::ChooseAttributesUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->initialPopUpContextInstID = -1;
	this->saveRerollContextInstID = -1;
	this->remainingPointsPopUpContextInstID = -1;
	this->portraitPopUpContextInstID = -1;
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
	ChooseAttributesUI::randomizeStats(charCreationState, game.random, game.arenaRandom);

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

	const std::string playerExperienceText = CharacterCreationUiModel::getPlayerExperience(game);
	const UiElementInstanceID experienceTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesExperienceTextBox");
	uiManager.setTextBoxText(experienceTextBoxElementInstID, playerExperienceText.c_str());

	const std::string playerLevelText = CharacterCreationUiModel::getPlayerLevel(game);
	const UiElementInstanceID levelTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesLevelTextBox");
	uiManager.setTextBoxText(levelTextBoxElementInstID, playerLevelText.c_str());

	ChooseAttributesUI::updatePrimaryAttributes();
	ChooseAttributesUI::updateDerivedAttributes();
	ChooseAttributesUI::updateGold();
	ChooseAttributesUI::updateBonusPoints();

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

	UiContextInitInfo initialPopUpContextInitInfo;
	initialPopUpContextInitInfo.name = ContextName_InitialPopUp;
	initialPopUpContextInitInfo.drawOrder = 1;
	state.initialPopUpContextInstID = uiManager.createContext(initialPopUpContextInitInfo);

	UiElementInitInfo initialPopUpTextBoxElementInitInfo;
	initialPopUpTextBoxElementInitInfo.name = "ChooseAttributesInitialPopUpTextBox";
	initialPopUpTextBoxElementInitInfo.position = ChooseAttributesUiView::InitialTextCenterPoint;
	initialPopUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	initialPopUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo initialPopUpTextBoxInitInfo;
	initialPopUpTextBoxInitInfo.text = ChooseAttributesUiModel::getInitialText(game);
	initialPopUpTextBoxInitInfo.fontName = ChooseAttributesUiView::InitialTextFontName;
	initialPopUpTextBoxInitInfo.defaultColor = ChooseAttributesUiView::InitialTextColor;
	initialPopUpTextBoxInitInfo.alignment = ChooseAttributesUiView::InitialTextAlignment;
	initialPopUpTextBoxInitInfo.lineSpacing = ChooseAttributesUiView::InitialTextLineSpacing;
	const UiElementInstanceID initialPopUpTextBoxElementInstID = uiManager.createTextBox(initialPopUpTextBoxElementInitInfo, initialPopUpTextBoxInitInfo, state.initialPopUpContextInstID, renderer);
	const Rect initialPopUpTextBoxRect = uiManager.getTransformGlobalRect(initialPopUpTextBoxElementInstID);

	UiElementInitInfo initialPopUpImageElementInitInfo;
	initialPopUpImageElementInitInfo.name = "ChooseAttributesInitialPopUpImage";
	initialPopUpImageElementInitInfo.position = ChooseAttributesUiView::InitialTextureCenterPoint;
	initialPopUpImageElementInitInfo.pivotType = UiPivotType::Middle;
	initialPopUpImageElementInitInfo.drawOrder = 0;

	const int initialPopUpImageTextureWidth = ChooseAttributesUiView::getDistributePointsTextBoxTextureWidth(initialPopUpTextBoxRect.width);
	const int initialPopUpImageTextureHeight = ChooseAttributesUiView::getDistributePointsTextBoxTextureHeight(initialPopUpTextBoxRect.height);
	UiTextureID initialPopUpImageTextureID = uiManager.getOrAddTexture(ChooseAttributesUiView::InitialTextPatternType, initialPopUpImageTextureWidth, initialPopUpImageTextureHeight, textureManager, renderer);
	uiManager.createImage(initialPopUpImageElementInitInfo, initialPopUpImageTextureID, state.initialPopUpContextInstID, renderer);

	UiElementInitInfo initialPopUpButtonElementInitInfo;
	initialPopUpButtonElementInitInfo.name = "ChooseAttributesInitialPopUpButton";
	initialPopUpButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	initialPopUpButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	initialPopUpButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo initialPopUpButtonInitInfo;
	initialPopUpButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	initialPopUpButtonInitInfo.callback = ChooseAttributesUI::onInitialPopUpBackButtonSelected;
	uiManager.createButton(initialPopUpButtonElementInitInfo, initialPopUpButtonInitInfo, state.initialPopUpContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseAttributesUI::onInitialPopUpBackInputAction, ContextName_InitialPopUp, inputManager);

	// Save/Reroll popup.
	const MessageBoxBackgroundProperties saveRerollBackgroundProperties = ChooseAttributesUiView::getMessageBoxBackgroundProperties();

	UiContextInitInfo saveRerollContextInitInfo;
	saveRerollContextInitInfo.name = ContextName_SaveReroll;
	saveRerollContextInitInfo.drawOrder = 1;
	state.saveRerollContextInstID = uiManager.createContext(saveRerollContextInitInfo);

	UiElementInitInfo saveRerollTitleTextBoxElementInitInfo;
	saveRerollTitleTextBoxElementInitInfo.name = "ChooseAttributesSaveRerollTitleTextBox";
	saveRerollTitleTextBoxElementInitInfo.position = ChooseAttributesUiView::MessageBoxTitleCenterPoint;
	saveRerollTitleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	saveRerollTitleTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo saveRerollTitleTextBoxInitInfo;
	saveRerollTitleTextBoxInitInfo.text = ChooseAttributesUiModel::getMessageBoxTitleText(game);
	saveRerollTitleTextBoxInitInfo.fontName = ChooseAttributesUiView::MessageBoxTitleFontName;
	saveRerollTitleTextBoxInitInfo.defaultColor = ChooseAttributesUiView::MessageBoxTitleColor;
	saveRerollTitleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	const UiElementInstanceID saveRerollTextBoxElementInstID = uiManager.createTextBox(saveRerollTitleTextBoxElementInitInfo, saveRerollTitleTextBoxInitInfo, state.saveRerollContextInstID, renderer);
	const Rect saveRerollTitleTextBoxRect = uiManager.getTransformGlobalRect(saveRerollTextBoxElementInstID);

	UiElementInitInfo saveRerollTitleImageElementInitInfo;
	saveRerollTitleImageElementInitInfo.name = "ChooseAttributesSaveRerollTitleImage";
	saveRerollTitleImageElementInitInfo.position = saveRerollTitleTextBoxRect.getCenter();
	saveRerollTitleImageElementInitInfo.pivotType = UiPivotType::Middle;
	saveRerollTitleImageElementInitInfo.drawOrder = 0;

	const int saveRerollTitleImageTextureWidth = saveRerollTitleTextBoxRect.width + saveRerollBackgroundProperties.extraTitleWidth;
	const int saveRerollTitleImageTextureHeight = *saveRerollBackgroundProperties.heightOverride;
	UiTextureID saveRerollTitleImageTextureID = uiManager.getOrAddTexture(saveRerollBackgroundProperties.patternType, saveRerollTitleImageTextureWidth, saveRerollTitleImageTextureHeight, textureManager, renderer);
	const UiElementInstanceID saveRerollTitleImageElementInstID = uiManager.createImage(saveRerollTitleImageElementInitInfo, saveRerollTitleImageTextureID, state.saveRerollContextInstID, renderer);
	const Rect saveRerollTitleImageTransformRect = uiManager.getTransformGlobalRect(saveRerollTitleImageElementInstID);

	UiElementInitInfo saveRerollSaveTextBoxElementInitInfo;
	saveRerollSaveTextBoxElementInitInfo.name = "ChooseAttributesSaveRerollSaveTextBox";
	saveRerollSaveTextBoxElementInitInfo.position = Int2(saveRerollTitleImageTransformRect.getCenter().x, saveRerollTitleImageTransformRect.getBottom() + (saveRerollBackgroundProperties.itemTextureHeight / 2));
	saveRerollSaveTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	saveRerollSaveTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo saveRerollSaveTextBoxInitInfo;
	saveRerollSaveTextBoxInitInfo.text = ChooseAttributesUiModel::getMessageBoxSaveText(game);
	saveRerollSaveTextBoxInitInfo.fontName = ChooseAttributesUiView::MessageBoxItemFontName;
	saveRerollSaveTextBoxInitInfo.defaultColor = ChooseAttributesUiView::MessageBoxItemTextColor;
	saveRerollSaveTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	// @todo
	/*const std::vector<TextRenderColorOverrideInfoEntry> saveTextColorOverrides = ChooseAttributesUiModel::getMessageBoxSaveColorOverrides(game);
	for (const TextRenderColorOverrideInfoEntry &entry : saveTextColorOverrides)
	{
		panel->addOverrideColor(0, entry.charIndex, entry.color);
	}*/
	uiManager.createTextBox(saveRerollSaveTextBoxElementInitInfo, saveRerollSaveTextBoxInitInfo, state.saveRerollContextInstID, renderer);

	UiElementInitInfo saveRerollSaveImageElementInitInfo;
	saveRerollSaveImageElementInitInfo.name = "ChooseAttributesSaveRerollSaveImage";
	saveRerollSaveImageElementInitInfo.position = saveRerollTitleImageTransformRect.getBottomLeft();
	saveRerollSaveImageElementInitInfo.drawOrder = 0;

	const UiTextureID saveRerollSaveImageTextureID = uiManager.getOrAddTexture(saveRerollBackgroundProperties.patternType, saveRerollTitleImageTransformRect.width, saveRerollBackgroundProperties.itemTextureHeight, textureManager, renderer);
	uiManager.createImage(saveRerollSaveImageElementInitInfo, saveRerollSaveImageTextureID, state.saveRerollContextInstID, renderer);

	UiElementInitInfo saveRerollSaveButtonElementInitInfo;
	saveRerollSaveButtonElementInitInfo.name = "ChooseAttributesSaveRerollSaveButton";
	saveRerollSaveButtonElementInitInfo.position = saveRerollSaveImageElementInitInfo.position;
	saveRerollSaveButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo saveRerollSaveButtonInitInfo;
	saveRerollSaveButtonInitInfo.callback = ChooseAttributesUI::onSaveRerollSaveButtonSelected;
	saveRerollSaveButtonInitInfo.contentElementName = saveRerollSaveImageElementInitInfo.name;
	uiManager.createButton(saveRerollSaveButtonElementInitInfo, saveRerollSaveButtonInitInfo, state.saveRerollContextInstID);

	UiElementInitInfo saveRerollRerollTextBoxElementInitInfo;
	saveRerollRerollTextBoxElementInitInfo.name = "ChooseAttributesSaveRerollRerollTextBox";
	saveRerollRerollTextBoxElementInitInfo.position = Int2(saveRerollTitleImageTransformRect.getCenter().x, saveRerollTitleImageTransformRect.getBottom() + ((3 * saveRerollBackgroundProperties.itemTextureHeight) / 2));
	saveRerollRerollTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	saveRerollRerollTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo saveRerollRerollTextBoxInitInfo;
	saveRerollRerollTextBoxInitInfo.text = ChooseAttributesUiModel::getMessageBoxRerollText(game);
	saveRerollRerollTextBoxInitInfo.fontName = ChooseAttributesUiView::MessageBoxItemFontName;
	saveRerollRerollTextBoxInitInfo.defaultColor = ChooseAttributesUiView::MessageBoxItemTextColor;
	saveRerollRerollTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	// @todo
	/*const std::vector<TextRenderColorOverrideInfoEntry> rerollTextColorOverrides = ChooseAttributesUiModel::getMessageBoxRerollColorOverrides(game);
	for (const TextRenderColorOverrideInfoEntry &entry : rerollTextColorOverrides)
	{
		panel->addOverrideColor(1, entry.charIndex, entry.color);
	}*/
	uiManager.createTextBox(saveRerollRerollTextBoxElementInitInfo, saveRerollRerollTextBoxInitInfo, state.saveRerollContextInstID, renderer);

	UiElementInitInfo saveRerollRerollImageElementInitInfo;
	saveRerollRerollImageElementInitInfo.name = "ChooseAttributesSaveRerollRerollImage";
	saveRerollRerollImageElementInitInfo.position = saveRerollTitleImageTransformRect.getBottomLeft() + Int2(0, saveRerollBackgroundProperties.itemTextureHeight);
	saveRerollRerollImageElementInitInfo.drawOrder = 0;

	const UiTextureID saveRerollRerollImageTextureID = uiManager.getOrAddTexture(saveRerollBackgroundProperties.patternType, saveRerollTitleImageTransformRect.width, saveRerollBackgroundProperties.itemTextureHeight, textureManager, renderer);
	uiManager.createImage(saveRerollRerollImageElementInitInfo, saveRerollRerollImageTextureID, state.saveRerollContextInstID, renderer);

	UiElementInitInfo saveRerollRerollButtonElementInitInfo;
	saveRerollRerollButtonElementInitInfo.name = "ChooseAttributesSaveRerollRerollButton";
	saveRerollRerollButtonElementInitInfo.position = saveRerollRerollImageElementInitInfo.position;
	saveRerollRerollButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo saveRerollRerollButtonInitInfo;
	saveRerollRerollButtonInitInfo.callback = ChooseAttributesUI::onSaveRerollRerollButtonSelected;
	saveRerollRerollButtonInitInfo.contentElementName = saveRerollRerollImageElementInitInfo.name;
	uiManager.createButton(saveRerollRerollButtonElementInitInfo, saveRerollRerollButtonInitInfo, state.saveRerollContextInstID);

	uiManager.addInputActionListener(InputActionName::SaveAttributes, ChooseAttributesUI::onSaveRerollSaveInputAction, ContextName_SaveReroll, inputManager);
	uiManager.addInputActionListener(InputActionName::RerollAttributes, ChooseAttributesUI::onSaveRerollRerollInputAction, ContextName_SaveReroll, inputManager);
	uiManager.addInputActionListener(InputActionName::Back, ChooseAttributesUI::onSaveRerollBackInputAction, ContextName_SaveReroll, inputManager);

	// Remaining points popup.
	UiContextInitInfo remainingPointsPopUpContextInitInfo;
	remainingPointsPopUpContextInitInfo.name = ContextName_RemainingPointsPopUp;
	remainingPointsPopUpContextInitInfo.drawOrder = 1;
	state.remainingPointsPopUpContextInstID = uiManager.createContext(remainingPointsPopUpContextInitInfo);

	UiElementInitInfo remainingPointsPopUpTextBoxElementInitInfo;
	remainingPointsPopUpTextBoxElementInitInfo.name = "ChooseAttributesRemainingsPointsPopUpTextBox";
	remainingPointsPopUpTextBoxElementInitInfo.position = ChooseAttributesUiView::AppearanceTextCenterPoint;
	remainingPointsPopUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	remainingPointsPopUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo remainingPointsPopUpTextBoxInitInfo;
	remainingPointsPopUpTextBoxInitInfo.text = ChooseAttributesUiModel::getBonusPointsRemainingText(game);
	remainingPointsPopUpTextBoxInitInfo.fontName = ChooseAttributesUiView::AppearanceTextFontName;
	remainingPointsPopUpTextBoxInitInfo.defaultColor = ChooseAttributesUiView::AppearanceTextColor;
	remainingPointsPopUpTextBoxInitInfo.alignment = ChooseAttributesUiView::AppearanceTextAlignment;
	remainingPointsPopUpTextBoxInitInfo.lineSpacing = ChooseAttributesUiView::AppearanceTextLineSpacing;
	const UiElementInstanceID remainingPointsPopUpTextBoxElementInstID = uiManager.createTextBox(remainingPointsPopUpTextBoxElementInitInfo, remainingPointsPopUpTextBoxInitInfo, state.remainingPointsPopUpContextInstID, renderer);
	const Rect remainingPointsPopUpTextBoxRect = uiManager.getTransformGlobalRect(remainingPointsPopUpTextBoxElementInstID);

	UiElementInitInfo remainingPointsPopUpImageElementInitInfo;
	remainingPointsPopUpImageElementInitInfo.name = "ChooseAttributesRemainingsPointsPopUpImage";
	remainingPointsPopUpImageElementInitInfo.position = ChooseAttributesUiView::AppearanceTextCenterPoint;
	remainingPointsPopUpImageElementInitInfo.pivotType = UiPivotType::Middle;
	remainingPointsPopUpImageElementInitInfo.drawOrder = 0;

	const int remainingPointsPopUpImageTextureWidth = ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(remainingPointsPopUpTextBoxRect.width);
	const int remainingPointsPopUpImageTextureHeight = ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(remainingPointsPopUpTextBoxRect.height);
	const UiTextureID remainingPointsPopUpImageTextureID = uiManager.getOrAddTexture(ChooseAttributesUiView::AppearanceTextPatternType, remainingPointsPopUpImageTextureWidth, remainingPointsPopUpImageTextureHeight, textureManager, renderer);
	uiManager.createImage(remainingPointsPopUpImageElementInitInfo, remainingPointsPopUpImageTextureID, state.remainingPointsPopUpContextInstID, renderer);

	UiElementInitInfo remainingPointsPopUpBackButtonElementInitInfo;
	remainingPointsPopUpBackButtonElementInitInfo.name = "ChooseAttributesRemainingPointsPopUpBackButton";
	remainingPointsPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	remainingPointsPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	remainingPointsPopUpBackButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo remainingPointsPopUpBackButtonInitInfo;
	remainingPointsPopUpBackButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	remainingPointsPopUpBackButtonInitInfo.callback = ChooseAttributesUI::onRemainingPointsPopUpBackButtonSelected;
	uiManager.createButton(remainingPointsPopUpBackButtonElementInitInfo, remainingPointsPopUpBackButtonInitInfo, state.remainingPointsPopUpContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseAttributesUI::onRemainingPointsPopUpBackInputAction, ContextName_RemainingPointsPopUp, inputManager);

	// Attributes saved, portrait now available popup.
	UiContextInitInfo portraitPopUpContextInitInfo;
	portraitPopUpContextInitInfo.name = ContextName_PortraitPopUp;
	portraitPopUpContextInitInfo.drawOrder = 1;
	state.portraitPopUpContextInstID = uiManager.createContext(portraitPopUpContextInitInfo);

	UiElementInitInfo portraitPopUpTextBoxElementInitInfo;
	portraitPopUpTextBoxElementInitInfo.name = "ChooseAttributesPortraitPopUpTextBox";
	portraitPopUpTextBoxElementInitInfo.position = ChooseAttributesUiView::AppearanceTextCenterPoint;
	portraitPopUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	portraitPopUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo portraitPopUpTextBoxInitInfo;
	portraitPopUpTextBoxInitInfo.text = ChooseAttributesUiModel::getAppearanceText(game);
	portraitPopUpTextBoxInitInfo.fontName = ChooseAttributesUiView::AppearanceTextFontName;
	portraitPopUpTextBoxInitInfo.defaultColor = ChooseAttributesUiView::AppearanceTextColor;
	portraitPopUpTextBoxInitInfo.alignment = ChooseAttributesUiView::AppearanceTextAlignment;
	portraitPopUpTextBoxInitInfo.lineSpacing = ChooseAttributesUiView::AppearanceTextLineSpacing;
	const UiElementInstanceID portraitPopUpTextBoxElementInstID = uiManager.createTextBox(portraitPopUpTextBoxElementInitInfo, portraitPopUpTextBoxInitInfo, state.portraitPopUpContextInstID, renderer);
	const Rect portraitPopUpTextBoxRect = uiManager.getTransformGlobalRect(portraitPopUpTextBoxElementInstID);

	UiElementInitInfo portraitPopUpImageElementInitInfo;
	portraitPopUpImageElementInitInfo.name = "ChooseAttributesPortraitPopUpImage";
	portraitPopUpImageElementInitInfo.position = ChooseAttributesUiView::AppearanceTextCenterPoint;
	portraitPopUpImageElementInitInfo.pivotType = UiPivotType::Middle;
	portraitPopUpImageElementInitInfo.drawOrder = 0;

	const int portraitPopUpImageTextureWidth = ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(portraitPopUpTextBoxRect.width);
	const int portraitPopUpImageTextureHeight = ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(portraitPopUpTextBoxRect.height);
	const UiTextureID portraitPopUpImageTextureID = uiManager.getOrAddTexture(ChooseAttributesUiView::AppearanceTextPatternType, portraitPopUpImageTextureWidth, portraitPopUpImageTextureHeight, textureManager, renderer);
	uiManager.createImage(portraitPopUpImageElementInitInfo, portraitPopUpImageTextureID, state.portraitPopUpContextInstID, renderer);

	UiElementInitInfo portraitPopUpBackButtonElementInitInfo;
	portraitPopUpBackButtonElementInitInfo.name = "ChooseAttributesPortraitPopUpBackButton";
	portraitPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	portraitPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	portraitPopUpBackButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo portraitPopUpBackButtonInitInfo;
	portraitPopUpBackButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	portraitPopUpBackButtonInitInfo.callback = ChooseAttributesUI::onPortraitPopUpBackButtonSelected;
	uiManager.createButton(portraitPopUpBackButtonElementInitInfo, portraitPopUpBackButtonInitInfo, state.portraitPopUpContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseAttributesUI::onPortraitPopUpBackInputAction, ContextName_PortraitPopUp, inputManager);

	// Only enable initial popup at start.
	uiManager.setContextEnabled(state.saveRerollContextInstID, false);
	uiManager.setContextEnabled(state.remainingPointsPopUpContextInstID, false);
	uiManager.setContextEnabled(state.portraitPopUpContextInstID, false);

	// Default to first attribute.
	OnPrimaryAttributeButtonSelected(0);
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

	if (state.initialPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.initialPopUpContextInstID, game.inputManager, game.renderer);
		state.initialPopUpContextInstID = -1;
	}

	if (state.saveRerollContextInstID >= 0)
	{
		uiManager.freeContext(state.saveRerollContextInstID, game.inputManager, game.renderer);
		state.saveRerollContextInstID = -1;
	}

	if (state.remainingPointsPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.remainingPointsPopUpContextInstID, game.inputManager, game.renderer);
		state.remainingPointsPopUpContextInstID = -1;
	}

	if (state.portraitPopUpContextInstID >= 0)
	{
		uiManager.freeContext(state.portraitPopUpContextInstID, game.inputManager, game.renderer);
		state.portraitPopUpContextInstID = -1;
	}

	state.attributesAreSaved = false;
	state.selectedAttributeIndex = -1;
}

void ChooseAttributesUI::update(double dt)
{
	// Do nothing.
	static_cast<void>(dt);
}

void ChooseAttributesUI::randomizeStats(CharacterCreationState &charCreationState, Random &random, ArenaRandom &arenaRandom)
{
	charCreationState.clearChangedPoints();
	charCreationState.populateBaseAttributes();

	for (PrimaryAttribute &attribute : charCreationState.attributes.getView())
	{
		const int addedValue = ChooseAttributesUiModel::rollClassic(ChooseAttributesUiModel::PrimaryAttributeRandomMax, arenaRandom);
		attribute.maxValue += addedValue;
	}

	const PrimaryAttributes &primaryAttributes = charCreationState.attributes;
	charCreationState.derivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(primaryAttributes);
	charCreationState.maxHealth = ArenaPlayerUtils::calculateMaxHealthPoints(charCreationState.classDefID, random);
	charCreationState.maxStamina = ArenaPlayerUtils::calculateMaxStamina(primaryAttributes.strength.maxValue, primaryAttributes.endurance.maxValue);
	charCreationState.maxSpellPoints = ArenaPlayerUtils::calculateMaxSpellPoints(charCreationState.classDefID, primaryAttributes.intelligence.maxValue);
	charCreationState.gold = ArenaPlayerUtils::calculateStartingGold(random);
	charCreationState.bonusPoints = ChooseAttributesUiModel::rollClassic(ChooseAttributesUiModel::BonusPointsRandomMax, arenaRandom);
}

void ChooseAttributesUI::updatePrimaryAttributes()
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	Span<const PrimaryAttribute> playerAttributesView = charCreationState.attributes.getView();
	for (const PrimaryAttribute &attribute : playerAttributesView)
	{
		const std::string attributeElementName = GetPrimaryAttributeTextBoxElementName(attribute.name);
		const UiElementInstanceID attributeTextBoxElementInstID = uiManager.getElementByName(attributeElementName.c_str());

		const std::string attributeValueText = std::to_string(attribute.maxValue);
		uiManager.setTextBoxText(attributeTextBoxElementInstID, attributeValueText.c_str());
	}
}

void ChooseAttributesUI::updateDerivedAttributes()
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

		DebugAssertIndex(CharacterSheetUiModel::DerivedAttributeUiNames, i);
		const std::string derivedAttributeElementName = GetDerivedAttributeTextBoxElementName(CharacterSheetUiModel::DerivedAttributeUiNames[i]);
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

void ChooseAttributesUI::updateGold()
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	const std::string playerGoldText = ChooseAttributesUiModel::getPlayerGold(game);
	const UiElementInstanceID goldTextBoxElementInstID = uiManager.getElementByName("ChooseAttributesGoldTextBox");
	uiManager.setTextBoxText(goldTextBoxElementInstID, playerGoldText.c_str());
}

void ChooseAttributesUI::updateBonusPoints()
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	UiManager &uiManager = game.uiManager;

	const std::string bonusPointsText = std::to_string(charCreationState.bonusPoints);
	const UiElementInstanceID bonusPointsTextBoxElementInstID = uiManager.getElementByName(BonusPointsTextBoxElementName);
	uiManager.setTextBoxText(bonusPointsTextBoxElementInstID, bonusPointsText.c_str());
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

	if (state.attributesAreSaved)
	{
		auto gameStateFunction = [](Game &game)
		{
			GameState &gameState = game.gameState;
			gameState.init(game.arenaRandom);

			// Find starting dungeon location definition.
			constexpr int provinceIndex = ArenaLocationUtils::CENTER_PROVINCE_ID;
			const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
			const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
			const std::optional<int> locationIndex = [&provinceDef]() -> std::optional<int>
			{
				for (int i = 0; i < provinceDef.getLocationCount(); i++)
				{
					const LocationDefinition &locationDef = provinceDef.getLocationDef(i);
					if (locationDef.getType() == LocationDefinitionType::MainQuestDungeon)
					{
						const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = locationDef.getMainQuestDungeonDefinition();

						if (mainQuestDungeonDef.type == LocationMainQuestDungeonDefinitionType::Start)
						{
							return i;
						}
					}
				}

				return std::nullopt;
			}();

			DebugAssertMsg(locationIndex.has_value(), "Couldn't find start dungeon location definition.");

			// Load starting dungeon.
			const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);
			const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = locationDef.getMainQuestDungeonDefinition();
			const std::string mifName = mainQuestDungeonDef.mapFilename;

			constexpr std::optional<bool> rulerIsMale; // Not needed.
			const std::string interiorDisplayName; // Unused.

			MapGenerationInteriorInfo interiorGenInfo;
			interiorGenInfo.initPrefab(mifName, ArenaInteriorType::Dungeon, rulerIsMale, interiorDisplayName);

			const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);

			MapDefinition mapDefinition;
			if (!mapDefinition.initInterior(interiorGenInfo, game.textureManager))
			{
				DebugLogError("Couldn't init MapDefinition for start dungeon \"" + mifName + "\".");
				return;
			}

			gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true);

			// Initialize player.
			const CharacterCreationState &charCreationState = game.getCharacterCreationState();
			const std::string_view name = charCreationState.name;
			const bool male = charCreationState.male;
			const int raceIndex = charCreationState.raceIndex;

			const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
			const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
			const ExeData &exeData = binaryAssetLibrary.getExeData();

			const int charClassDefID = charCreationState.classDefID;
			const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(charClassDefID);
			const int portraitIndex = charCreationState.portraitIndex;

			const PrimaryAttributes &attributes = charCreationState.attributes;
			const int maxHealth = charCreationState.maxHealth;
			const int maxStamina = charCreationState.maxStamina;
			const int maxSpellPoints = charCreationState.maxSpellPoints;
			const int gold = charCreationState.gold;

			const int allowedWeaponCount = charClassDef.getAllowedWeaponCount();
			const int weaponID = charClassDef.getAllowedWeapon(game.random.next(allowedWeaponCount));

			Player &player = game.player;
			player.init(std::string(name), male, raceIndex, charClassDefID, portraitIndex, attributes, maxHealth, maxStamina, maxSpellPoints,
				gold, weaponID, game.options.getMisc_GhostMode(), exeData, game.physicsSystem);

			// Face west so we don't start looking at a wall.
			player.setCameraFrameFromAngles(CardinalDirection::DegreesWest, 0.0);
		};

		gameStateFunction(game);

		const auto &cinematicLibrary = CinematicLibrary::getInstance();
		int textCinematicDefIndex;
		const TextCinematicDefinition *defPtr = nullptr;
		const bool success = cinematicLibrary.findTextDefinitionIndexIf(
			[&defPtr](const TextCinematicDefinition &def)
		{
			if (def.type == TextCinematicDefinitionType::MainQuest)
			{
				const MainQuestTextCinematicDefinition &mainQuestCinematicDef = def.mainQuest;
				const bool isMainQuestStartCinematic = mainQuestCinematicDef.progress == 0;
				if (isMainQuestStartCinematic)
				{
					defPtr = &def;
					return true;
				}
			}

			return false;
		}, &textCinematicDefIndex);

		if (!success)
		{
			DebugCrash("Couldn't find main quest start text cinematic definition.");
		}

		game.setCharacterCreationState(nullptr);

		TextureManager &textureManager = game.textureManager;
		const std::string &cinematicFilename = defPtr->animFilename;
		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(cinematicFilename.c_str());
		if (!metadataID.has_value())
		{
			DebugLogError("Couldn't get texture file metadata for main quest start cinematic \"" + cinematicFilename + "\".");
			return;
		}

		const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);

		TextCinematicUiInitInfo &textCinematicInitInfo = TextCinematicUI::state.initInfo;
		textCinematicInitInfo.init(textCinematicDefIndex, metadata.getSecondsPerFrame(), [&game]() { ChooseAttributesUiController::onPostCharacterCreationCinematicFinished(game); });
		game.setPanel<TextCinematicPanel>();

		// Play dream music.
		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
			MusicType::Cinematic, game.random, [](const MusicDefinition &def)
		{
			DebugAssert(def.type == MusicType::Cinematic);
			const CinematicMusicDefinition &cinematicMusicDef = def.cinematic;
			return cinematicMusicDef.type == CinematicMusicType::DreamGood;
		});

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing vision music.");
		}

		AudioManager &audioManager = game.audioManager;
		audioManager.setMusic(musicDef);
	}
	else
	{
		// Show save/reroll message box.
		UiManager &uiManager = game.uiManager;
		uiManager.setContextEnabled(state.saveRerollContextInstID, true);

		InputManager &inputManager = game.inputManager;
		inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, true);
	}
}

void ChooseAttributesUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setPanel<ChooseRacePanel>();
	}
}

void ChooseAttributesUI::onInitialPopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
}

void ChooseAttributesUI::onInitialPopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseAttributesUI::onInitialPopUpBackButtonSelected(MouseButtonType::Left);
	}
}

void ChooseAttributesUI::onSaveRerollSaveButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	const CharacterCreationState &charCreationState = game.getCharacterCreationState();
	UiManager &uiManager = game.uiManager;

	uiManager.disableTopMostContext();

	if (charCreationState.bonusPoints == 0)
	{
		state.attributesAreSaved = true;

		// Show portrait selection pop-up.
		uiManager.setContextEnabled(state.portraitPopUpContextInstID, true);
	}
	else
	{
		// Tell the player to spend remaining points.
		uiManager.setContextEnabled(state.remainingPointsPopUpContextInstID, true);
	}

	InputManager &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, false);
}

void ChooseAttributesUI::onSaveRerollSaveInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseAttributesUI::onSaveRerollSaveButtonSelected(MouseButtonType::Left);
	}
}

void ChooseAttributesUI::onSaveRerollRerollButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	CharacterCreationState &charCreationState = *game.charCreationState;
	ChooseAttributesUI::randomizeStats(charCreationState, game.random, game.arenaRandom);
	ChooseAttributesUI::updatePrimaryAttributes();
	ChooseAttributesUI::updateDerivedAttributes();
	ChooseAttributesUI::updateGold();
	ChooseAttributesUI::updateBonusPoints();
	uiManager.setContextEnabled(state.initialPopUpContextInstID, true);

	InputManager &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, false);
}

void ChooseAttributesUI::onSaveRerollRerollInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseAttributesUI::onSaveRerollRerollButtonSelected(MouseButtonType::Left);
	}
}

void ChooseAttributesUI::onSaveRerollBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseAttributesUI::onSaveRerollRerollButtonSelected(MouseButtonType::Left);
	}
}

void ChooseAttributesUI::onRemainingPointsPopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
}

void ChooseAttributesUI::onRemainingPointsPopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseAttributesUI::onRemainingPointsPopUpBackButtonSelected(MouseButtonType::Left);
	}
}

void ChooseAttributesUI::onPortraitPopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseAttributesUiState &state = ChooseAttributesUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
}

void ChooseAttributesUI::onPortraitPopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseAttributesUI::onPortraitPopUpBackButtonSelected(MouseButtonType::Left);
	}
}
