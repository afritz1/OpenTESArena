#include "LoadSaveUiModel.h"
#include "LoadSaveUiState.h"
#include "LoadSaveUiView.h"
#include "MainMenuPanel.h"
#include "PauseMenuPanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"

namespace
{
	constexpr char ContextName_PopUp[] = "LoadSavePopUp";

	constexpr MouseButtonTypeFlags PopUpMouseButtonTypeFlags = MouseButtonType::Left | MouseButtonType::Right;

	std::string GetEntryTextBoxElementName(int index)
	{
		char elementName[32];
		std::snprintf(elementName, sizeof(elementName), "LoadSaveEntryTextBox%d", index);
		return std::string(elementName);
	}

	std::string GetEntryButtonElementName(int index)
	{
		char elementName[32];
		std::snprintf(elementName, sizeof(elementName), "LoadSaveEntryButton%d", index);
		return std::string(elementName);
	}
}

LoadSaveUiState::LoadSaveUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->popUpContextInstID = -1;
	this->type = LoadSaveType::Load;
}

void LoadSaveUiState::init(Game &game)
{
	this->game = &game;
}

void LoadSaveUI::create(Game &game)
{
	LoadSaveUiState &state = LoadSaveUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(LoadSaveUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	UiContextInitInfo popUpContextInitInfo;
	popUpContextInitInfo.name = ContextName_PopUp;
	popUpContextInitInfo.drawOrder = 1;
	state.popUpContextInstID = uiManager.createContext(popUpContextInitInfo);
	uiManager.setContextEnabled(state.popUpContextInstID, false);

	for (int i = 0; i < LoadSaveUiModel::SlotCount; i++)
	{
		UiElementInitInfo entryTextBoxElementInitInfo;
		entryTextBoxElementInitInfo.name = GetEntryTextBoxElementName(i);
		entryTextBoxElementInitInfo.position = LoadSaveUiView::getEntryCenterPoint(i);
		entryTextBoxElementInitInfo.drawOrder = 1;

		UiTextBoxInitInfo entryTextBoxInitInfo;
		entryTextBoxInitInfo.worstCaseText = TextRenderUtils::makeWorstCaseText(20);
		entryTextBoxInitInfo.fontName = LoadSaveUiView::EntryFontName;
		entryTextBoxInitInfo.defaultColor = LoadSaveUiView::getEntryTextColor();
		entryTextBoxInitInfo.alignment = LoadSaveUiView::EntryTextAlignment;
		uiManager.createTextBox(entryTextBoxElementInitInfo, entryTextBoxInitInfo, state.contextInstID, renderer);

		const Rect entryButtonRect = LoadSaveUiModel::getSlotRect(i);
		UiElementInitInfo entryButtonElementInitInfo;
		entryButtonElementInitInfo.name = GetEntryButtonElementName(i);
		entryButtonElementInitInfo.position = entryButtonRect.getCenter();
		entryButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
		entryButtonElementInitInfo.size = entryButtonRect.getSize();
		entryButtonElementInitInfo.pivotType = UiPivotType::Middle;

		UiButtonInitInfo entryButtonInitInfo;
		entryButtonInitInfo.callback = [i](MouseButtonType) { LoadSaveUI::onEntrySelected(i); };
		uiManager.createButton(entryButtonElementInitInfo, entryButtonInitInfo, state.contextInstID);
	}

	const std::vector<LoadSaveUiEntry> entries = LoadSaveUiModel::getSaveEntries(game);
	for (int i = 0; i < static_cast<int>(entries.size()); i++)
	{
		const LoadSaveUiEntry &entry = entries[i];
		const std::string entryTextBoxElementName = GetEntryTextBoxElementName(i);
		const UiElementInstanceID entryTextBoxElementInstID = uiManager.getElementByName(entryTextBoxElementName.c_str());
		uiManager.setTextBoxText(entryTextBoxElementInstID, entry.text.c_str());
	}
}

void LoadSaveUI::destroy()
{
	LoadSaveUiState &state = LoadSaveUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	if (state.popUpContextInstID >= 0)
	{
		uiManager.freeContext(state.popUpContextInstID, inputManager, renderer);
		state.popUpContextInstID = -1;
	}
}

void LoadSaveUI::update(double dt)
{
	// Do nothing.
}

void LoadSaveUI::onEntrySelected(int index)
{
	LoadSaveUiState &state = LoadSaveUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	uiManager.clearContextElements(state.popUpContextInstID, inputManager, renderer);
	uiManager.setContextEnabled(state.popUpContextInstID, true);

	UiElementInitInfo popUpTextBoxElementInitInfo;
	popUpTextBoxElementInitInfo.name = "LoadSavePopUpTextBox";
	popUpTextBoxElementInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH / 2, ArenaRenderUtils::SCREEN_HEIGHT / 2);
	popUpTextBoxElementInitInfo.pivotType = UiPivotType::Middle;
	popUpTextBoxElementInitInfo.drawOrder = 1;

	UiTextBoxInitInfo popUpTextBoxInitInfo;
	popUpTextBoxInitInfo.text = "Not implemented\n(save slot " + std::to_string(index) + ")";
	popUpTextBoxInitInfo.fontName = ArenaFontName::Arena;
	popUpTextBoxInitInfo.defaultColor = Color(150, 97, 0);
	popUpTextBoxInitInfo.alignment = TextAlignment::MiddleCenter;
	popUpTextBoxInitInfo.lineSpacing = 1;
	const UiElementInstanceID popUpTextBoxElementInstID = uiManager.createTextBox(popUpTextBoxElementInitInfo, popUpTextBoxInitInfo, state.popUpContextInstID, renderer);
	const Rect popUpTextBoxRect = uiManager.getTransformGlobalRect(popUpTextBoxElementInstID);

	UiElementInitInfo popUpImageElementInitInfo;
	popUpImageElementInitInfo.name = "LoadSavePopUpImage";
	popUpImageElementInitInfo.position = popUpTextBoxElementInitInfo.position;
	popUpImageElementInitInfo.pivotType = popUpTextBoxElementInitInfo.pivotType;
	popUpImageElementInitInfo.drawOrder = 0;

	const int popUpTextureWidth = popUpTextBoxRect.width + 10;
	const int popUpTextureHeight = popUpTextBoxRect.height + 10;
	const UiTextureID popUpTextureID = uiManager.getOrAddTexture(UiTexturePatternType::Dark, popUpTextureWidth, popUpTextureHeight, textureManager, renderer);
	uiManager.createImage(popUpImageElementInitInfo, popUpTextureID, state.popUpContextInstID, renderer);

	UiElementInitInfo popUpBackButtonElementInitInfo;
	popUpBackButtonElementInitInfo.name = "LoadSavePopUpBackButton";
	popUpBackButtonElementInitInfo.sizeType = UiTransformSizeType::Manual;
	popUpBackButtonElementInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);

	UiButtonCallback popUpBackButtonCallback = [](MouseButtonType)
	{
		LoadSaveUiState &state = LoadSaveUI::state;
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		uiManager.setContextEnabled(state.popUpContextInstID, false);
	};

	UiButtonInitInfo popUpBackButtonInitInfo;
	popUpBackButtonInitInfo.callback = popUpBackButtonCallback;
	popUpBackButtonInitInfo.mouseButtonFlags = PopUpMouseButtonTypeFlags;
	uiManager.createButton(popUpBackButtonElementInitInfo, popUpBackButtonInitInfo, state.popUpContextInstID);

	InputActionCallback popUpBackInputActionCallback = [popUpBackButtonCallback](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			popUpBackButtonCallback(MouseButtonType::Left);
		}
	};

	uiManager.addInputActionListener(InputActionName::Back, popUpBackInputActionCallback, ContextName_PopUp, inputManager);
}

void LoadSaveUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		Game &game = values.game;
		const GameState &gameState = game.gameState;
		if (gameState.isActiveMapValid())
		{
			game.setPanel<PauseMenuPanel>();
		}
		else
		{
			game.setPanel<MainMenuPanel>();
		}
	}
}
