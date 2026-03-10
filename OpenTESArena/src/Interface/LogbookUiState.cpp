#include "GameWorldPanel.h"
#include "LogbookUiModel.h"
#include "LogbookUiState.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"

LogbookUiState::LogbookUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
}

void LogbookUiState::init(Game &game)
{
	this->game = &game;
}

void LogbookUI::create(Game &game)
{
	LogbookUiState &state = LogbookUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(LogbookUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const std::string noEntriesText = LogbookUiModel::getNoEntriesText(game);
	const UiElementInstanceID noEntriesTextBoxElementInstID = uiManager.getElementByName("LogbookNoEntriesTextBox");
	uiManager.setTextBoxText(noEntriesTextBoxElementInstID, noEntriesText.c_str());

	const bool logbookHasEntries = false; // @todo implement logbook entries
	if (logbookHasEntries)
	{
		uiManager.setElementActive(noEntriesTextBoxElementInstID, false);
	}

	const UiTextureID cursorTextureID = game.defaultCursorTextureID;
	const std::optional<Int2> cursorDims = renderer.tryGetUiTextureDims(cursorTextureID);
	DebugAssert(cursorDims.has_value());

	const Options &options = game.options;
	const double cursorScale = options.getGraphics_CursorScale();
	const Int2 cursorSize(
		static_cast<int>(static_cast<double>(cursorDims->x) * cursorScale),
		static_cast<int>(static_cast<double>(cursorDims->y) * cursorScale));
	uiManager.setTransformSize(game.cursorImageElementInstID, cursorSize);
	uiManager.setImageTexture(game.cursorImageElementInstID, cursorTextureID);
	uiManager.setTransformPivot(game.cursorImageElementInstID, UiPivotType::TopLeft);

	inputManager.setInputActionMapActive(InputActionMapName::Logbook, true);
}

void LogbookUI::destroy()
{
	LogbookUiState &state = LogbookUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	Renderer &renderer = game.renderer;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, renderer);
		state.contextInstID = -1;
	}

	inputManager.setInputActionMapActive(InputActionMapName::Logbook, false);
}

void LogbookUI::update(double dt)
{
	// Do nothing.
}

void LogbookUI::onBackButtonSelected(MouseButtonType mouseButtonType)
{
	LogbookUiState &state = LogbookUI::state;
	Game &game = *state.game;
	game.setPanel<GameWorldPanel>();
}

void LogbookUI::onBackInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		LogbookUI::onBackButtonSelected(MouseButtonType::Left);
	}
}
