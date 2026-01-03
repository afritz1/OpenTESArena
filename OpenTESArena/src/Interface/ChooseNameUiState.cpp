#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "ChooseNameUiState.h"
#include "../Game/Game.h"

namespace
{
	constexpr char EntryTextBoxElementName[] = "ChooseNameEntryTextBox";
}

ChooseNameUiState::ChooseNameUiState()
{
	this->game = nullptr;
}

void ChooseNameUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseNameUI::create(Game &game)
{
	ChooseNameUiState &state = ChooseNameUI::state;
	state.init(game);

	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	UiManager &uiManager = game.uiManager;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseNameUI::ContextType);
	uiManager.createContext(contextDef, state.contextState, inputManager, textureManager, renderer);

	const std::string titleText = ChooseNameUiModel::getTitleText(game);
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.getElementByName("ChooseNameTitleTextBox");
	uiManager.setTextBoxText(titleTextBoxElementInstID, titleText.c_str());
	
	const InputListenerID textInputListenerID = inputManager.addTextInputListener(ChooseNameUI::onTextInput);
	state.contextState.textInputListenerIDs.emplace_back(textInputListenerID);

	const UiTextureID cursorTextureID = game.defaultCursorTextureID;
	const std::optional<Int2> cursorDims = renderer.tryGetUiTextureDims(cursorTextureID);
	DebugAssert(cursorDims.has_value());

	const Options &options = game.options;
	const double cursorScale = options.getGraphics_CursorScale();
	const Int2 cursorSize(
		static_cast<int>(static_cast<double>(cursorDims->x) * cursorScale),
		static_cast<int>(static_cast<double>(cursorDims->y) * cursorScale));

	uiManager.setTransformSize(game.cursorImageElementInstID, cursorSize);
	uiManager.setTransformPivot(game.cursorImageElementInstID, UiPivotType::TopLeft);
	uiManager.setImageTexture(game.cursorImageElementInstID, cursorTextureID);

	inputManager.setTextInputMode(true);
}

void ChooseNameUI::destroy()
{
	ChooseNameUiState &state = ChooseNameUI::state;
	Game &game = *state.game;
	state.contextState.free(game.inputManager, game.uiManager, game.renderer);
	state.name.clear();

	InputManager &inputManager = game.inputManager;
	inputManager.setTextInputMode(false);
}

void ChooseNameUI::update(double dt)
{
	// Do nothing.
	static_cast<void>(dt);
}

void ChooseNameUI::onTextInput(const std::string_view text)
{
	ChooseNameUiState &state = ChooseNameUI::state;

	bool isTextChanged = false;
	ChooseNameUiController::onTextInput(text, state.name, &isTextChanged);

	if (isTextChanged)
	{
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		UiElementInstanceID entryTextBoxElementInstID = uiManager.getElementByName(EntryTextBoxElementName);
		uiManager.setTextBoxText(entryTextBoxElementInstID, state.name.c_str());
	}
}

void ChooseNameUI::onAcceptInputAction(const InputActionCallbackValues &values)
{
	const ChooseNameUiState &state = ChooseNameUI::state;
	ChooseNameUiController::onAcceptInputAction(values, state.name);
}

void ChooseNameUI::onBackspaceInputAction(const InputActionCallbackValues &values)
{
	ChooseNameUiState &state = ChooseNameUI::state;

	bool isTextChanged = false;
	ChooseNameUiController::onBackspaceInputAction(values, state.name, &isTextChanged);

	if (isTextChanged)
	{
		Game &game = *state.game;
		UiManager &uiManager = game.uiManager;
		UiElementInstanceID entryTextBoxElementInstID = uiManager.getElementByName(EntryTextBoxElementName);
		uiManager.setTextBoxText(entryTextBoxElementInstID, state.name.c_str());
	}
}

void ChooseNameUI::onBackInputAction(const InputActionCallbackValues &values)
{
	ChooseNameUiController::onBackToChooseClassInputAction(values);
}
