#include "CharacterCreationUiMVC.h"
#include "ChooseNameUiState.h"
#include "../Game/Game.h"

namespace
{
	constexpr char EntryTextBoxElementName[] = "ChooseNameEntryTextBox";
}

ChooseNameUiState::ChooseNameUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
}

void ChooseNameUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseNameUI::create(Game &game)
{
	ChooseNameUiState &state = ChooseNameUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseNameUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const std::string titleText = ChooseNameUiModel::getTitleText(game);
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.getElementByName("ChooseNameTitleTextBox");
	uiManager.setTextBoxText(titleTextBoxElementInstID, titleText.c_str());
	
	uiManager.addTextInputListener(ChooseNameUI::onTextInput, contextDef.name.c_str(), inputManager);

	game.setCursorOverride(std::nullopt);

	inputManager.setTextInputMode(true);
}

void ChooseNameUI::destroy()
{
	ChooseNameUiState &state = ChooseNameUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, inputManager, game.renderer);
		state.contextInstID = -1;
	}
	
	state.name.clear();

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
