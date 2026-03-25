#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseAttributesUiState.h"
#include "ChooseGenderUiState.h"
#include "ChooseRaceUiState.h"
#include "WorldMapUiModel.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"

namespace
{
	constexpr char ContextName_ProvinceConfirm[] = "ChooseRaceProvinceConfirm";
	constexpr char ContextName_ProvinceConfirmed1[] = "ChooseRaceProvinceConfirmed1";
	constexpr char ContextName_ProvinceConfirmed2[] = "ChooseRaceProvinceConfirmed2";
	constexpr char ContextName_ProvinceConfirmed3[] = "ChooseRaceProvinceConfirmed3";
	constexpr char ContextName_ProvinceConfirmed4[] = "ChooseRaceProvinceConfirmed4";

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;
}

ChooseRaceUiState::ChooseRaceUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->initialPopUpContextInstID = -1;
	this->provinceConfirmContextInstID = -1;
	this->provinceConfirmed1ContextInstID = -1;
	this->provinceConfirmed2ContextInstID = -1;
	this->provinceConfirmed3ContextInstID = -1;
	this->provinceConfirmed4ContextInstID = -1;
}

void ChooseRaceUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseRaceUI::create(Game &game)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseRaceUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	uiManager.addMouseButtonChangedListener(ChooseRaceUI::onMouseButtonChanged, contextDef.name.c_str(), inputManager);

	// Initial pop-up.
	const UiTextureID initialPopUpTextureID = uiManager.getOrAddTexture(ChooseRaceUiView::InitialPopUpPatternType, ChooseRaceUiView::InitialPopUpTextureWidth, ChooseRaceUiView::InitialPopUpTextureHeight, textureManager, renderer);

	UiContextInitInfo initialPopUpContextInitInfo;
	initialPopUpContextInitInfo.name = "ChooseRaceInitialPopUp";
	initialPopUpContextInitInfo.drawOrder = 1;
	state.initialPopUpContextInstID = uiManager.createContext(initialPopUpContextInitInfo);

	UiElementInitInfo initialPopUpTextureElementInitInfo;
	initialPopUpTextureElementInitInfo.name = "ChooseRaceInitialPopUpTexture";
	initialPopUpTextureElementInitInfo.position = ChooseRaceUiView::InitialPopUpTextureCenterPoint;
	initialPopUpTextureElementInitInfo.pivotType = UiPivotType::Middle;
	initialPopUpTextureElementInitInfo.drawOrder = 0;
	uiManager.createImage(initialPopUpTextureElementInitInfo, initialPopUpTextureID, state.initialPopUpContextInstID, renderer);
	
	UiElementInitInfo initialPopUpTextBoxElementInitInfo;
	initialPopUpTextBoxElementInitInfo.name = "ChooseRaceInitialPopUpTextBox";
	initialPopUpTextBoxElementInitInfo.position = ChooseRaceUiView::InitialPopUpTextCenterPoint;
	initialPopUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	initialPopUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo initialPopUpTextBoxInitInfo;
	initialPopUpTextBoxInitInfo.text = ChooseRaceUiModel::getTitleText(game);
	initialPopUpTextBoxInitInfo.fontName = ChooseRaceUiView::InitialPopUpFontName;
	initialPopUpTextBoxInitInfo.defaultColor = ChooseRaceUiView::InitialPopUpColor;
	initialPopUpTextBoxInitInfo.alignment = ChooseRaceUiView::InitialPopUpAlignment;
	initialPopUpTextBoxInitInfo.lineSpacing = ChooseRaceUiView::InitialPopUpLineSpacing;
	uiManager.createTextBox(initialPopUpTextBoxElementInitInfo, initialPopUpTextBoxInitInfo, state.initialPopUpContextInstID, renderer);
	
	UiElementInitInfo initialPopUpBackButtonElementInitInfo;
	initialPopUpBackButtonElementInitInfo.name = "PopUpBackButton";
	initialPopUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	initialPopUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	initialPopUpBackButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo initialPopUpBackButtonInitInfo;
	initialPopUpBackButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	initialPopUpBackButtonInitInfo.callback = ChooseRaceUI::onInitialPopUpBackButtonSelected;
	uiManager.createButton(initialPopUpBackButtonElementInitInfo, initialPopUpBackButtonInitInfo, state.initialPopUpContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseRaceUI::onInitialPopUpBackInputAction, initialPopUpContextInitInfo.name.c_str(), inputManager);

	// Province confirm pop-up.
	UiContextInitInfo provinceConfirmContextInitInfo;
	provinceConfirmContextInitInfo.name = ContextName_ProvinceConfirm;
	provinceConfirmContextInitInfo.drawOrder = 1;
	state.provinceConfirmContextInstID = uiManager.createContext(provinceConfirmContextInitInfo);

	// Province confirmed pop-ups.
	UiContextInitInfo provinceConfirmed1ContextInitInfo;
	provinceConfirmed1ContextInitInfo.name = ContextName_ProvinceConfirmed1;
	provinceConfirmed1ContextInitInfo.drawOrder = 1;
	state.provinceConfirmed1ContextInstID = uiManager.createContext(provinceConfirmed1ContextInitInfo);

	UiContextInitInfo provinceConfirmed2ContextInitInfo;
	provinceConfirmed2ContextInitInfo.name = ContextName_ProvinceConfirmed2;
	provinceConfirmed2ContextInitInfo.drawOrder = 1;
	state.provinceConfirmed2ContextInstID = uiManager.createContext(provinceConfirmed2ContextInitInfo);

	UiContextInitInfo provinceConfirmed3ContextInitInfo;
	provinceConfirmed3ContextInitInfo.name = ContextName_ProvinceConfirmed3;
	provinceConfirmed3ContextInitInfo.drawOrder = 1;
	state.provinceConfirmed3ContextInstID = uiManager.createContext(provinceConfirmed3ContextInitInfo);

	UiContextInitInfo provinceConfirmed4ContextInitInfo;
	provinceConfirmed4ContextInitInfo.name = ContextName_ProvinceConfirmed4;
	provinceConfirmed4ContextInitInfo.drawOrder = 1;
	state.provinceConfirmed4ContextInstID = uiManager.createContext(provinceConfirmed4ContextInitInfo);

	uiManager.setContextEnabled(state.provinceConfirmContextInstID, false);
	uiManager.setContextEnabled(state.provinceConfirmed1ContextInstID, false);
	uiManager.setContextEnabled(state.provinceConfirmed2ContextInstID, false);
	uiManager.setContextEnabled(state.provinceConfirmed3ContextInstID, false);
	uiManager.setContextEnabled(state.provinceConfirmed4ContextInstID, false);

	// @todo: scroll unravel animation
}

void ChooseRaceUI::destroy()
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
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

	if (state.provinceConfirmContextInstID >= 0)
	{
		uiManager.freeContext(state.provinceConfirmContextInstID, game.inputManager, game.renderer);
		state.provinceConfirmContextInstID = -1;
	}

	if (state.provinceConfirmed1ContextInstID >= 0)
	{
		uiManager.freeContext(state.provinceConfirmed1ContextInstID, game.inputManager, game.renderer);
		state.provinceConfirmed1ContextInstID = -1;
	}

	if (state.provinceConfirmed2ContextInstID >= 0)
	{
		uiManager.freeContext(state.provinceConfirmed2ContextInstID, game.inputManager, game.renderer);
		state.provinceConfirmed2ContextInstID = -1;
	}

	if (state.provinceConfirmed3ContextInstID >= 0)
	{
		uiManager.freeContext(state.provinceConfirmed3ContextInstID, game.inputManager, game.renderer);
		state.provinceConfirmed3ContextInstID = -1;
	}

	if (state.provinceConfirmed4ContextInstID >= 0)
	{
		uiManager.freeContext(state.provinceConfirmed4ContextInstID, game.inputManager, game.renderer);
		state.provinceConfirmed4ContextInstID = -1;
	}
}

void ChooseRaceUI::update(double dt)
{
	// Do nothing.
	static_cast<void>(dt);

	// @todo unravel animation
}

void ChooseRaceUI::onMouseButtonChanged(Game &game, MouseButtonType mouseButtonType, const Int2 &position, bool pressed)
{
	if (mouseButtonType == MouseButtonType::Left)
	{
		if (pressed)
		{
			const std::optional<int> provinceID = WorldMapUiModel::getMaskID(game, position, true, true);
			if (provinceID.has_value())
			{
				CharacterCreationState &charCreationState = game.getCharacterCreationState();
				charCreationState.raceIndex = *provinceID;

				ChooseRaceUiState &state = ChooseRaceUI::state;
				UiManager &uiManager = game.uiManager;
				InputManager &inputManager = game.inputManager;
				TextureManager &textureManager = game.textureManager;
				Renderer &renderer = game.renderer;

				// @todo this is very annoying to author without some UiMessageBox to reduce the amount of code
				// - can make UiMessageBox either be:
				//   - (cleaner, less user-side code) one giant element with a title text box + texture and item text boxes + textures + buttons + input actions. it would be several draw calls.
				//   - (more complex) a driver of other UiElements' positions; it registers them at init, they follow its position and pivot type. basically a ui transform manager/layout element.

				uiManager.clearContextElements(state.provinceConfirmContextInstID, inputManager, renderer);

				const MessageBoxBackgroundProperties provinceConfirmTitleBackgroundProperties = ChooseRaceUiView::getProvinceConfirmMessageBoxBackgroundProperties();

				// Create province confirm message box.
				// - have to make text box first so parchment texture can be sized properly (a 9-sliced image component would help fix this)
				UiElementInitInfo provinceConfirmTitleTextBoxElementInitInfo;
				provinceConfirmTitleTextBoxElementInitInfo.name = "ProvinceConfirmPopUpTitleTextBox";
				provinceConfirmTitleTextBoxElementInitInfo.position = ChooseRaceUiView::ProvinceConfirmTitleCenterPoint;
				provinceConfirmTitleTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
				provinceConfirmTitleTextBoxElementInitInfo.drawOrder = 1;

				UiTextBoxInitInfo provinceConfirmTitleTextBoxInitInfo;
				provinceConfirmTitleTextBoxInitInfo.text = ChooseRaceUiModel::getProvinceConfirmTitleText(game);
				provinceConfirmTitleTextBoxInitInfo.fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
				provinceConfirmTitleTextBoxInitInfo.defaultColor = ChooseRaceUiView::ProvinceConfirmTitleTextColor;
				provinceConfirmTitleTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
				provinceConfirmTitleTextBoxInitInfo.lineSpacing = ChooseRaceUiView::ProvinceConfirmTitleLineSpacing;
				const UiElementInstanceID provinceConfirmTitleTextBoxElementInstID = uiManager.createTextBox(provinceConfirmTitleTextBoxElementInitInfo, provinceConfirmTitleTextBoxInitInfo, state.provinceConfirmContextInstID, renderer);
				const Rect provinceConfirmTitleTextBoxRect = uiManager.getTransformGlobalRect(provinceConfirmTitleTextBoxElementInstID);

				UiElementInitInfo provinceConfirmTitleImageElementInitInfo;
				provinceConfirmTitleImageElementInitInfo.name = "ProvinceConfirmPopUpTitleImage";
				provinceConfirmTitleImageElementInitInfo.position = ChooseRaceUiView::ProvinceConfirmTitleCenterPoint;
				provinceConfirmTitleImageElementInitInfo.pivotType = UiPivotType::Middle;
				provinceConfirmTitleImageElementInitInfo.drawOrder = 0;

				const UiTextureID provinceConfirmTitleImageTextureID = uiManager.getOrAddTexture(
					ChooseRaceUiView::ProvinceConfirmTitleTexturePatternType,
					provinceConfirmTitleTextBoxRect.width + provinceConfirmTitleBackgroundProperties.extraTitleWidth,
					*provinceConfirmTitleBackgroundProperties.heightOverride,
					textureManager,
					renderer);
				const UiElementInstanceID provinceConfirmTitleImageElementInstID = uiManager.createImage(provinceConfirmTitleImageElementInitInfo, provinceConfirmTitleImageTextureID, state.provinceConfirmContextInstID, renderer);
				const Rect provinceConfirmTitleImageTransformRect = uiManager.getTransformGlobalRect(provinceConfirmTitleImageElementInstID);

				UiElementInitInfo provinceConfirmAcceptTextBoxElementInitInfo;
				provinceConfirmAcceptTextBoxElementInitInfo.name = "ProvinceConfirmPopUpAcceptTextBox";
				provinceConfirmAcceptTextBoxElementInitInfo.position = Int2(provinceConfirmTitleImageTransformRect.getCenter().x, provinceConfirmTitleImageTransformRect.getBottom() + (provinceConfirmTitleBackgroundProperties.itemTextureHeight / 2));
				provinceConfirmAcceptTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
				provinceConfirmAcceptTextBoxElementInitInfo.drawOrder = 1;

				UiTextBoxInitInfo provinceConfirmAcceptTextBoxInitInfo;
				provinceConfirmAcceptTextBoxInitInfo.text = ChooseRaceUiModel::getProvinceConfirmYesText(game);
				provinceConfirmAcceptTextBoxInitInfo.fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
				provinceConfirmAcceptTextBoxInitInfo.defaultColor = ChooseRaceUiView::ProvinceConfirmTitleTextColor;
				provinceConfirmAcceptTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
				provinceConfirmAcceptTextBoxInitInfo.lineSpacing = ChooseRaceUiView::ProvinceConfirmTitleLineSpacing;
				uiManager.createTextBox(provinceConfirmAcceptTextBoxElementInitInfo, provinceConfirmAcceptTextBoxInitInfo, state.provinceConfirmContextInstID, renderer);

				UiElementInitInfo provinceConfirmAcceptImageElementInitInfo;
				provinceConfirmAcceptImageElementInitInfo.name = "ProvinceConfirmPopUpAcceptImage";
				provinceConfirmAcceptImageElementInitInfo.position = provinceConfirmTitleImageTransformRect.getBottomLeft();
				provinceConfirmAcceptImageElementInitInfo.drawOrder = 0;
				
				const UiTextureID provinceConfirmAcceptImageTextureID = uiManager.getOrAddTexture(
					ChooseRaceUiView::ProvinceConfirmTitleTexturePatternType,
					provinceConfirmTitleImageTransformRect.width,
					provinceConfirmTitleBackgroundProperties.itemTextureHeight,
					textureManager,
					renderer);
				uiManager.createImage(provinceConfirmAcceptImageElementInitInfo, provinceConfirmAcceptImageTextureID, state.provinceConfirmContextInstID, renderer);

				UiElementInitInfo provinceConfirmAcceptButtonElementInitInfo;
				provinceConfirmAcceptButtonElementInitInfo.name = "ProvinceConfirmPopUpAcceptButton";
				provinceConfirmAcceptButtonElementInitInfo.position = provinceConfirmAcceptImageElementInitInfo.position;
				provinceConfirmAcceptButtonElementInitInfo.drawOrder = 2;
				
				UiButtonInitInfo provinceConfirmAcceptButtonInitInfo;
				provinceConfirmAcceptButtonInitInfo.callback = ChooseRaceUI::onProvinceConfirmPopUpAcceptButtonSelected;
				provinceConfirmAcceptButtonInitInfo.contentElementName = provinceConfirmAcceptImageElementInitInfo.name;
				uiManager.createButton(provinceConfirmAcceptButtonElementInitInfo, provinceConfirmAcceptButtonInitInfo, state.provinceConfirmContextInstID);

				UiElementInitInfo provinceConfirmCancelTextBoxElementInitInfo;
				provinceConfirmCancelTextBoxElementInitInfo.name = "ProvinceConfirmPopUpCancelTextBox";
				provinceConfirmCancelTextBoxElementInitInfo.position = Int2(provinceConfirmTitleImageTransformRect.getCenter().x, provinceConfirmTitleImageTransformRect.getBottom() + ((3 * provinceConfirmTitleBackgroundProperties.itemTextureHeight) / 2));
				provinceConfirmCancelTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
				provinceConfirmCancelTextBoxElementInitInfo.drawOrder = 1;

				UiTextBoxInitInfo provinceConfirmCancelTextBoxInitInfo;
				provinceConfirmCancelTextBoxInitInfo.text = ChooseRaceUiModel::getProvinceConfirmNoText(game);
				provinceConfirmCancelTextBoxInitInfo.fontName = ChooseRaceUiView::ProvinceConfirmTitleFontName;
				provinceConfirmCancelTextBoxInitInfo.defaultColor = ChooseRaceUiView::ProvinceConfirmTitleTextColor;
				provinceConfirmCancelTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
				provinceConfirmCancelTextBoxInitInfo.lineSpacing = ChooseRaceUiView::ProvinceConfirmTitleLineSpacing;
				uiManager.createTextBox(provinceConfirmCancelTextBoxElementInitInfo, provinceConfirmCancelTextBoxInitInfo, state.provinceConfirmContextInstID, renderer);

				UiElementInitInfo provinceConfirmCancelImageElementInitInfo;
				provinceConfirmCancelImageElementInitInfo.name = "ProvinceConfirmPopUpCancelImage";
				provinceConfirmCancelImageElementInitInfo.position = provinceConfirmTitleImageTransformRect.getBottomLeft() + Int2(0, provinceConfirmTitleBackgroundProperties.itemTextureHeight);
				provinceConfirmCancelImageElementInitInfo.drawOrder = 0;

				const UiTextureID provinceConfirmCancelImageTextureID = uiManager.getOrAddTexture(
					ChooseRaceUiView::ProvinceConfirmTitleTexturePatternType,
					provinceConfirmTitleImageTransformRect.width,
					provinceConfirmTitleBackgroundProperties.itemTextureHeight,
					textureManager,
					renderer);
				uiManager.createImage(provinceConfirmCancelImageElementInitInfo, provinceConfirmCancelImageTextureID, state.provinceConfirmContextInstID, renderer);

				UiElementInitInfo provinceConfirmCancelButtonElementInitInfo;
				provinceConfirmCancelButtonElementInitInfo.name = "ProvinceConfirmPopUpCancelButton";
				provinceConfirmCancelButtonElementInitInfo.position = provinceConfirmCancelImageElementInitInfo.position;
				provinceConfirmCancelButtonElementInitInfo.drawOrder = 2;

				UiButtonInitInfo provinceConfirmCancelButtonInitInfo;
				provinceConfirmCancelButtonInitInfo.callback = ChooseRaceUI::onProvinceConfirmPopUpCancelButtonSelected;
				provinceConfirmCancelButtonInitInfo.contentElementName = provinceConfirmCancelImageElementInitInfo.name;
				uiManager.createButton(provinceConfirmCancelButtonElementInitInfo, provinceConfirmCancelButtonInitInfo, state.provinceConfirmContextInstID);

				uiManager.addInputActionListener(InputActionName::Back, ChooseRaceUI::onProvinceConfirmPopUpBackInputAction, ContextName_ProvinceConfirm, inputManager);

				uiManager.setContextEnabled(state.provinceConfirmContextInstID, true);
			}
		}
	}
}

void ChooseRaceUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		game.setNextContext(ChooseGenderUI::ContextName);
	}
}

void ChooseRaceUI::onInitialPopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
}

void ChooseRaceUI::onInitialPopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseRaceUI::onInitialPopUpBackButtonSelected(MouseButtonType::Left);
	}
}

void ChooseRaceUI::onProvinceConfirmPopUpAcceptButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	uiManager.clearContextElements(state.provinceConfirmed1ContextInstID, inputManager, renderer);

	UiElementInitInfo provinceConfirmed1TextBoxElementInitInfo;
	provinceConfirmed1TextBoxElementInitInfo.name = "ProvinceConfirmed1PopUpAcceptTextBox";
	provinceConfirmed1TextBoxElementInitInfo.position = ChooseRaceUiView::ProvinceConfirmedFirstTextCenterPoint;
	provinceConfirmed1TextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	provinceConfirmed1TextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo provinceConfirmed1TextBoxInitInfo;
	provinceConfirmed1TextBoxInitInfo.text = ChooseRaceUiModel::getProvinceConfirmedFirstText(game);
	provinceConfirmed1TextBoxInitInfo.fontName = ChooseRaceUiView::ProvinceConfirmedFirstTextFontName;
	provinceConfirmed1TextBoxInitInfo.defaultColor = ChooseRaceUiView::ProvinceConfirmedFirstTextColor;
	provinceConfirmed1TextBoxInitInfo.alignment = ChooseRaceUiView::ProvinceConfirmedFirstTextAlignment;
	provinceConfirmed1TextBoxInitInfo.lineSpacing = ChooseRaceUiView::ProvinceConfirmedFirstTextLineSpacing;
	const UiElementInstanceID provinceConfirmed1TextBoxElementInstID = uiManager.createTextBox(provinceConfirmed1TextBoxElementInitInfo, provinceConfirmed1TextBoxInitInfo, state.provinceConfirmed1ContextInstID, renderer);
	const Rect provinceConfirmed1TextBoxTransformRect = uiManager.getTransformGlobalRect(provinceConfirmed1TextBoxElementInstID);

	const Rect provinceConfirmed1ImageRect = ChooseRaceUiView::getProvinceConfirmedFirstTextureRect(provinceConfirmed1TextBoxTransformRect.width, provinceConfirmed1TextBoxTransformRect.height);
	UiElementInitInfo provinceConfirmed1ImageElementInitInfo;
	provinceConfirmed1ImageElementInitInfo.name = "ProvinceConfirmed1PopUpAcceptImage";
	provinceConfirmed1ImageElementInitInfo.position = provinceConfirmed1ImageRect.getCenter();
	provinceConfirmed1ImageElementInitInfo.pivotType = UiPivotType::Middle;
	provinceConfirmed1ImageElementInitInfo.drawOrder = 0;

	const UiTextureID provinceConfirmed1ImageTextureID = uiManager.getOrAddTexture(ChooseRaceUiView::ProvinceConfirmedFirstTextPatternType, provinceConfirmed1ImageRect.width, provinceConfirmed1ImageRect.height, textureManager, renderer);
	uiManager.createImage(provinceConfirmed1ImageElementInitInfo, provinceConfirmed1ImageTextureID, state.provinceConfirmed1ContextInstID, renderer);

	UiElementInitInfo provinceConfirmed1ButtonElementInitInfo;
	provinceConfirmed1ButtonElementInitInfo.name = "ProvinceConfirmed1PopUpAcceptButton";
	provinceConfirmed1ButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	provinceConfirmed1ButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	provinceConfirmed1ButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo provinceConfirmed1ButtonInitInfo;
	provinceConfirmed1ButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	provinceConfirmed1ButtonInitInfo.callback = ChooseRaceUI::onProvinceConfirmed1PopUpBackButtonSelected;
	uiManager.createButton(provinceConfirmed1ButtonElementInitInfo, provinceConfirmed1ButtonInitInfo, state.provinceConfirmed1ContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseRaceUI::onProvinceConfirmed1PopUpBackInputAction, ContextName_ProvinceConfirmed1, inputManager);

	uiManager.setContextEnabled(state.provinceConfirmed1ContextInstID, true);
}

void ChooseRaceUI::onProvinceConfirmPopUpCancelButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	uiManager.setContextEnabled(state.initialPopUpContextInstID, true);
}

void ChooseRaceUI::onProvinceConfirmPopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseRaceUI::onProvinceConfirmPopUpCancelButtonSelected(MouseButtonType::Left);
	}
}

void ChooseRaceUI::onProvinceConfirmed1PopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();
	
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	uiManager.clearContextElements(state.provinceConfirmed2ContextInstID, inputManager, renderer);

	UiElementInitInfo provinceConfirmed2TextBoxElementInitInfo;
	provinceConfirmed2TextBoxElementInitInfo.name = "ProvinceConfirmed2PopUpAcceptTextBox";
	provinceConfirmed2TextBoxElementInitInfo.position = ChooseRaceUiView::ProvinceConfirmedSecondTextCenterPoint;
	provinceConfirmed2TextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	provinceConfirmed2TextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo provinceConfirmed2TextBoxInitInfo;
	provinceConfirmed2TextBoxInitInfo.text = ChooseRaceUiModel::getProvinceConfirmedSecondText(game);
	provinceConfirmed2TextBoxInitInfo.fontName = ChooseRaceUiView::ProvinceConfirmedSecondTextFontName;
	provinceConfirmed2TextBoxInitInfo.defaultColor = ChooseRaceUiView::ProvinceConfirmedSecondTextColor;
	provinceConfirmed2TextBoxInitInfo.alignment = ChooseRaceUiView::ProvinceConfirmedSecondTextAlignment;
	provinceConfirmed2TextBoxInitInfo.lineSpacing = ChooseRaceUiView::ProvinceConfirmedSecondTextLineSpacing;
	const UiElementInstanceID provinceConfirmed2TextBoxElementInstID = uiManager.createTextBox(provinceConfirmed2TextBoxElementInitInfo, provinceConfirmed2TextBoxInitInfo, state.provinceConfirmed2ContextInstID, renderer);
	const Rect provinceConfirmed2TextBoxTransformRect = uiManager.getTransformGlobalRect(provinceConfirmed2TextBoxElementInstID);

	const Rect provinceConfirmed2ImageRect = ChooseRaceUiView::getProvinceConfirmedSecondTextureRect(provinceConfirmed2TextBoxTransformRect.width, provinceConfirmed2TextBoxTransformRect.height);
	UiElementInitInfo provinceConfirmed2ImageElementInitInfo;
	provinceConfirmed2ImageElementInitInfo.name = "ProvinceConfirmed2PopUpAcceptImage";
	provinceConfirmed2ImageElementInitInfo.position = provinceConfirmed2ImageRect.getCenter();
	provinceConfirmed2ImageElementInitInfo.pivotType = UiPivotType::Middle;
	provinceConfirmed2ImageElementInitInfo.drawOrder = 0;

	const UiTextureID provinceConfirmed2ImageTextureID = uiManager.getOrAddTexture(ChooseRaceUiView::ProvinceConfirmedSecondTextPatternType, provinceConfirmed2ImageRect.width, provinceConfirmed2ImageRect.height, textureManager, renderer);
	uiManager.createImage(provinceConfirmed2ImageElementInitInfo, provinceConfirmed2ImageTextureID, state.provinceConfirmed2ContextInstID, renderer);

	UiElementInitInfo provinceConfirmed2ButtonElementInitInfo;
	provinceConfirmed2ButtonElementInitInfo.name = "ProvinceConfirmed2PopUpAcceptButton";
	provinceConfirmed2ButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	provinceConfirmed2ButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	provinceConfirmed2ButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo provinceConfirmed2ButtonInitInfo;
	provinceConfirmed2ButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	provinceConfirmed2ButtonInitInfo.callback = ChooseRaceUI::onProvinceConfirmed2PopUpBackButtonSelected;
	uiManager.createButton(provinceConfirmed2ButtonElementInitInfo, provinceConfirmed2ButtonInitInfo, state.provinceConfirmed2ContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseRaceUI::onProvinceConfirmed2PopUpBackInputAction, ContextName_ProvinceConfirmed2, inputManager);

	uiManager.setContextEnabled(state.provinceConfirmed2ContextInstID, true);
}

void ChooseRaceUI::onProvinceConfirmed2PopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	uiManager.clearContextElements(state.provinceConfirmed3ContextInstID, inputManager, renderer);

	UiElementInitInfo provinceConfirmed3TextBoxElementInitInfo;
	provinceConfirmed3TextBoxElementInitInfo.name = "ProvinceConfirmed3PopUpAcceptTextBox";
	provinceConfirmed3TextBoxElementInitInfo.position = ChooseRaceUiView::ProvinceConfirmedThirdTextCenterPoint;
	provinceConfirmed3TextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	provinceConfirmed3TextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo provinceConfirmed3TextBoxInitInfo;
	provinceConfirmed3TextBoxInitInfo.text = ChooseRaceUiModel::getProvinceConfirmedThirdText(game);
	provinceConfirmed3TextBoxInitInfo.fontName = ChooseRaceUiView::ProvinceConfirmedThirdTextFontName;
	provinceConfirmed3TextBoxInitInfo.defaultColor = ChooseRaceUiView::ProvinceConfirmedThirdTextColor;
	provinceConfirmed3TextBoxInitInfo.alignment = ChooseRaceUiView::ProvinceConfirmedThirdTextAlignment;
	provinceConfirmed3TextBoxInitInfo.lineSpacing = ChooseRaceUiView::ProvinceConfirmedThirdTextLineSpacing;
	const UiElementInstanceID provinceConfirmed3TextBoxElementInstID = uiManager.createTextBox(provinceConfirmed3TextBoxElementInitInfo, provinceConfirmed3TextBoxInitInfo, state.provinceConfirmed3ContextInstID, renderer);
	const Rect provinceConfirmed3TextBoxTransformRect = uiManager.getTransformGlobalRect(provinceConfirmed3TextBoxElementInstID);

	const Rect provinceConfirmed3ImageRect = ChooseRaceUiView::getProvinceConfirmedThirdTextureRect(provinceConfirmed3TextBoxTransformRect.width, provinceConfirmed3TextBoxTransformRect.height);
	UiElementInitInfo provinceConfirmed3ImageElementInitInfo;
	provinceConfirmed3ImageElementInitInfo.name = "ProvinceConfirmed3PopUpAcceptImage";
	provinceConfirmed3ImageElementInitInfo.position = provinceConfirmed3ImageRect.getCenter();
	provinceConfirmed3ImageElementInitInfo.pivotType = UiPivotType::Middle;
	provinceConfirmed3ImageElementInitInfo.drawOrder = 0;

	const UiTextureID provinceConfirmed3ImageTextureID = uiManager.getOrAddTexture(ChooseRaceUiView::ProvinceConfirmedThirdTextPatternType, provinceConfirmed3ImageRect.width, provinceConfirmed3ImageRect.height, textureManager, renderer);
	uiManager.createImage(provinceConfirmed3ImageElementInitInfo, provinceConfirmed3ImageTextureID, state.provinceConfirmed3ContextInstID, renderer);

	UiElementInitInfo provinceConfirmed3ButtonElementInitInfo;
	provinceConfirmed3ButtonElementInitInfo.name = "ProvinceConfirmed3PopUpAcceptButton";
	provinceConfirmed3ButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	provinceConfirmed3ButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	provinceConfirmed3ButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo provinceConfirmed3ButtonInitInfo;
	provinceConfirmed3ButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	provinceConfirmed3ButtonInitInfo.callback = ChooseRaceUI::onProvinceConfirmed3PopUpBackButtonSelected;
	uiManager.createButton(provinceConfirmed3ButtonElementInitInfo, provinceConfirmed3ButtonInitInfo, state.provinceConfirmed3ContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseRaceUI::onProvinceConfirmed3PopUpBackInputAction, ContextName_ProvinceConfirmed3, inputManager);

	uiManager.setContextEnabled(state.provinceConfirmed3ContextInstID, true);
}

void ChooseRaceUI::onProvinceConfirmed3PopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	uiManager.clearContextElements(state.provinceConfirmed4ContextInstID, inputManager, renderer);

	UiElementInitInfo provinceConfirmed4TextBoxElementInitInfo;
	provinceConfirmed4TextBoxElementInitInfo.name = "ProvinceConfirmed4PopUpAcceptTextBox";
	provinceConfirmed4TextBoxElementInitInfo.position = ChooseRaceUiView::ProvinceConfirmedFourthTextCenterPoint;
	provinceConfirmed4TextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	provinceConfirmed4TextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo provinceConfirmed4TextBoxInitInfo;
	provinceConfirmed4TextBoxInitInfo.text = ChooseRaceUiModel::getProvinceConfirmedFourthText(game);
	provinceConfirmed4TextBoxInitInfo.fontName = ChooseRaceUiView::ProvinceConfirmedFourthTextFontName;
	provinceConfirmed4TextBoxInitInfo.defaultColor = ChooseRaceUiView::ProvinceConfirmedFourthTextColor;
	provinceConfirmed4TextBoxInitInfo.alignment = ChooseRaceUiView::ProvinceConfirmedFourthTextAlignment;
	provinceConfirmed4TextBoxInitInfo.lineSpacing = ChooseRaceUiView::ProvinceConfirmedFourthTextLineSpacing;
	const UiElementInstanceID provinceConfirmed4TextBoxElementInstID = uiManager.createTextBox(provinceConfirmed4TextBoxElementInitInfo, provinceConfirmed4TextBoxInitInfo, state.provinceConfirmed4ContextInstID, renderer);
	const Rect provinceConfirmed4TextBoxTransformRect = uiManager.getTransformGlobalRect(provinceConfirmed4TextBoxElementInstID);

	const Rect provinceConfirmed4ImageRect = ChooseRaceUiView::getProvinceConfirmedFourthTextureRect(provinceConfirmed4TextBoxTransformRect.width, provinceConfirmed4TextBoxTransformRect.height);
	UiElementInitInfo provinceConfirmed4ImageElementInitInfo;
	provinceConfirmed4ImageElementInitInfo.name = "ProvinceConfirmed4PopUpAcceptImage";
	provinceConfirmed4ImageElementInitInfo.position = provinceConfirmed4ImageRect.getCenter();
	provinceConfirmed4ImageElementInitInfo.pivotType = UiPivotType::Middle;
	provinceConfirmed4ImageElementInitInfo.drawOrder = 0;

	const UiTextureID provinceConfirmed4ImageTextureID = uiManager.getOrAddTexture(ChooseRaceUiView::ProvinceConfirmedFourthTextPatternType, provinceConfirmed4ImageRect.width, provinceConfirmed4ImageRect.height, textureManager, renderer);
	uiManager.createImage(provinceConfirmed4ImageElementInitInfo, provinceConfirmed4ImageTextureID, state.provinceConfirmed4ContextInstID, renderer);

	UiElementInitInfo provinceConfirmed4ButtonElementInitInfo;
	provinceConfirmed4ButtonElementInitInfo.name = "ProvinceConfirmed4PopUpAcceptButton";
	provinceConfirmed4ButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	provinceConfirmed4ButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	provinceConfirmed4ButtonElementInitInfo.drawOrder = 2;

	UiButtonInitInfo provinceConfirmed4ButtonInitInfo;
	provinceConfirmed4ButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	provinceConfirmed4ButtonInitInfo.callback = ChooseRaceUI::onProvinceConfirmed4PopUpBackButtonSelected;
	uiManager.createButton(provinceConfirmed4ButtonElementInitInfo, provinceConfirmed4ButtonInitInfo, state.provinceConfirmed4ContextInstID);

	uiManager.addInputActionListener(InputActionName::Back, ChooseRaceUI::onProvinceConfirmed4PopUpBackInputAction, ContextName_ProvinceConfirmed4, inputManager);

	uiManager.setContextEnabled(state.provinceConfirmed4ContextInstID, true);
}

void ChooseRaceUI::onProvinceConfirmed4PopUpBackButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	uiManager.disableTopMostContext();

	game.setNextContext(ChooseAttributesUI::ContextName);
}

void ChooseRaceUI::onProvinceConfirmed1PopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseRaceUI::onProvinceConfirmed1PopUpBackButtonSelected(MouseButtonType::Left);
	}
}

void ChooseRaceUI::onProvinceConfirmed2PopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseRaceUI::onProvinceConfirmed2PopUpBackButtonSelected(MouseButtonType::Left);
	}
}

void ChooseRaceUI::onProvinceConfirmed3PopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseRaceUI::onProvinceConfirmed3PopUpBackButtonSelected(MouseButtonType::Left);
	}
}

void ChooseRaceUI::onProvinceConfirmed4PopUpBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		ChooseRaceUI::onProvinceConfirmed4PopUpBackButtonSelected(MouseButtonType::Left);
	}
}
