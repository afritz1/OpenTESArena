#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "ChooseGenderUiState.h"
#include "../Game/Game.h"

ChooseGenderUiState::ChooseGenderUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
}

void ChooseGenderUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseGenderUI::create(Game &game)
{
	ChooseGenderUiState &state = ChooseGenderUI::state;
	state.init(game);

	UiManager &uiManager = game.uiManager;
	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseGenderUI::ContextName);
	state.contextInstID = uiManager.createContext(contextDef, inputManager, textureManager, renderer);

	const std::string titleText = ChooseGenderUiModel::getTitleText(game);
	const UiElementInstanceID titleTextBoxElementInstID = uiManager.getElementByName("ChooseGenderTitleTextBox");
	uiManager.setTextBoxText(titleTextBoxElementInstID, titleText.c_str());

	const std::string maleText = ChooseGenderUiModel::getMaleText(game);
	const UiElementInstanceID maleTextBoxElementInstID = uiManager.getElementByName("ChooseGenderMaleTextBox");
	uiManager.setTextBoxText(maleTextBoxElementInstID, maleText.c_str());

	const std::string femaleText = ChooseGenderUiModel::getFemaleText(game);
	const UiElementInstanceID femaleTextBoxElementInstID = uiManager.getElementByName("ChooseGenderFemaleTextBox");
	uiManager.setTextBoxText(femaleTextBoxElementInstID, femaleText.c_str());
}

void ChooseGenderUI::destroy()
{
	ChooseGenderUiState &state = ChooseGenderUI::state;
	Game &game = *state.game;
	UiManager &uiManager = game.uiManager;
	
	if (state.contextInstID >= 0)
	{
		uiManager.freeContext(state.contextInstID, game.inputManager, game.renderer);
		state.contextInstID = -1;
	}
}

void ChooseGenderUI::update(double dt)
{
	// Do nothing.
	static_cast<void>(dt);
}

void ChooseGenderUI::onMaleButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseGenderUiState &state = ChooseGenderUI::state;
	ChooseGenderUiController::onMaleButtonSelected(*state.game);
}

void ChooseGenderUI::onFemaleButtonSelected(MouseButtonType mouseButtonType)
{
	ChooseGenderUiState &state = ChooseGenderUI::state;
	ChooseGenderUiController::onFemaleButtonSelected(*state.game);
}

void ChooseGenderUI::onBackInputAction(const InputActionCallbackValues &values)
{
	ChooseGenderUiController::onBackToChooseNameInputAction(values);
}
