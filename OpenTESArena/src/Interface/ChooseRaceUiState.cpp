#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseRaceUiState.h"
#include "WorldMapUiModel.h"
#include "../Game/Game.h"

namespace
{
	/*std::unique_ptr<Panel> GetInitialSubPanel(Game &game)
	{
		const std::string text = ChooseRaceUiModel::getTitleText(game);
		const TextBoxInitInfo textBoxInitInfo = ChooseRaceUiView::getInitialPopUpTextBoxInitInfo(text, game);

		auto &textureManager = game.textureManager;
		auto &renderer = game.renderer;
		const UiTextureID textureID = ChooseRaceUiView::allocInitialPopUpTexture(textureManager, renderer);
		ScopedUiTextureRef textureRef(textureID, renderer);

		std::unique_ptr<TextSubPanel> subPanel = std::make_unique<TextSubPanel>(game);
		if (!subPanel->init(textBoxInitInfo, text, ChooseRaceUiController::onInitialPopUpButtonSelected,
			std::move(textureRef), ChooseRaceUiView::InitialPopUpTextureCenterPoint))
		{
			DebugCrash("Couldn't init choose race initial sub-panel.");
		}

		return subPanel;
	}*/
}

ChooseRaceUiState::ChooseRaceUiState()
{
	this->game = nullptr;
	this->contextInstID = -1;
	this->initialPopUpContextInstID = -1;
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

	uiManager.addMouseButtonChangedListener(ChooseRaceUI::onMouseButtonChanged, state.contextInstID, inputManager);

	// @todo create popup contexts and set active=false, only set the "from where dost thou hail" popup active=true.
	UiContextInitInfo initialPopUpContextInitInfo;
	initialPopUpContextInitInfo.name = "ChooseRaceInitialPopUp";
	initialPopUpContextInitInfo.drawOrder = 1;
	state.initialPopUpContextInstID = uiManager.createContext(initialPopUpContextInitInfo);

	//const UiTextureID initialPopUpTextureID = ChooseRaceUiView::allocInitialPopUpTexture(textureManager, renderer);
	//const std::string initialPopUpText = ChooseRaceUiModel::getTitleText(game);
	// @todo ui text box init info
	
	// @todo ui image init info

	// @todo: scroll unravel animation

	// @todo cancelling the selected province also needs to rerun that popup ^ in ChooseRaceUiController::onProvinceCancelButtonSelected
	DebugLogErrorFormat("Not implemented: ChooseRaceUI::create() GetInitialSubPanel()");
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
}

void ChooseRaceUI::update(double dt)
{
	// Do nothing.
	static_cast<void>(dt);
}

void ChooseRaceUI::onMouseButtonChanged(Game &game, MouseButtonType mouseButtonType, const Int2 &position, bool pressed)
{
	if (mouseButtonType == MouseButtonType::Left)
	{
		if (pressed)
		{
			// Check if the mouse is over a province mask.
			const std::optional<int> provinceID = WorldMapUiModel::getMaskID(game, position, true, true);
			if (provinceID.has_value())
			{
				ChooseRaceUiController::onProvinceButtonSelected(game, *provinceID);
			}
		}
	}
}

void ChooseRaceUI::onBackInputAction(const InputActionCallbackValues &values)
{
	ChooseRaceUiController::onBackToChooseGenderInputAction(values);
}
