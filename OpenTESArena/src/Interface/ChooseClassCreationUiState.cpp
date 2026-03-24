#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "ChooseClassCreationUiState.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"

ChooseClassCreationUiState::ChooseClassCreationUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
}

void ChooseClassCreationUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseClassCreationUI::create(Game &game)
{
	ChooseClassCreationUiState &state = ChooseClassCreationUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseClassCreationUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const std::string titleText = ChooseClassCreationUiModel::getTitleText(game);
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.getElementByName("ChooseClassCreationTitleTextBox");
	uiManager.setTextBoxText(titleTextBoxElementInstID, titleText.c_str());

	const std::string generateText = ChooseClassCreationUiModel::getGenerateButtonText(game);
	const UiElementInstanceID generateTextBoxElementInstID = uiManager.getElementByName("ChooseClassCreationGenerateTextBox");
	uiManager.setTextBoxText(generateTextBoxElementInstID, generateText.c_str());

	const std::string selectText = ChooseClassCreationUiModel::getSelectButtonText(game);
	const UiElementInstanceID selectTextBoxElementInstID = uiManager.getElementByName("ChooseClassCreationSelectTextBox");
	uiManager.setTextBoxText(selectTextBoxElementInstID, selectText.c_str());

	game.setCursorOverride(std::nullopt);

	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, true);
}

void ChooseClassCreationUI::destroy()
{
	ChooseClassCreationUiState &state = ChooseClassCreationUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;

	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, game.inputManager, game.renderer);
		state.contextInstID = -1;
	}
}

void ChooseClassCreationUI::update(double dt)
{
	// Do nothing.
	static_cast<void>(dt);
}

void ChooseClassCreationUI::onGenerateButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassCreationUiState &state = ChooseClassCreationUI::state;
	ChooseClassCreationUiController::onGenerateButtonSelected(*state.game);
}

void ChooseClassCreationUI::onSelectButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseClassCreationUiState &state = ChooseClassCreationUI::state;
	ChooseClassCreationUiController::onSelectButtonSelected(*state.game);
}

void ChooseClassCreationUI::onBackInputAction(const InputActionCallbackValues &values)
{
	ChooseClassCreationUiController::onBackToMainMenuInputAction(values);
}
