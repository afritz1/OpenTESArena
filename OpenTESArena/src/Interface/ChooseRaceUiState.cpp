#include "CharacterCreationUiController.h"
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
}

void ChooseRaceUiState::init(Game &game)
{
	this->game = &game;
}

void ChooseRaceUI::create(Game &game)
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	state.init(game);

	InputManager &inputManager = game.inputManager;
	TextureManager &textureManager = game.textureManager;
	Renderer &renderer = game.renderer;
	UiManager &uiManager = game.uiManager;

	const UiLibrary &uiLibrary = UiLibrary::getInstance();
	const UiContextDefinition &contextDef = uiLibrary.getDefinition(ChooseRaceUI::ContextType);
	uiManager.createContext(contextDef, state.contextState, inputManager, textureManager, renderer);

	const InputListenerID mouseButtonChangedListenerID = inputManager.addMouseButtonChangedListener(ChooseRaceUI::onMouseButtonChanged);
	state.contextState.mouseButtonChangedListenerIDs.emplace_back(mouseButtonChangedListenerID);

	// @todo popup about "from where are ye from etc"
	// - UiLayerID?
	// - will "pushing a UiContextDefinition" mean picking the next UiLayerID higher?

	/*
	
	// Push the initial text sub-panel.
	// @todo: scroll unravel animation
	std::unique_ptr<Panel> textSubPanel = GetInitialSubPanel(game);
	game.pushSubPanel(std::move(textSubPanel));
	
	*/

	// @todo cancelling the selected province also needs to rerun that popup ^ in ChooseRaceUiController::onProvinceCancelButtonSelected
	DebugLogErrorFormat("Not implemented: ChooseRaceUI::create() GetInitialSubPanel()");
}

void ChooseRaceUI::destroy()
{
	ChooseRaceUiState &state = ChooseRaceUI::state;
	Game &game = *state.game;
	state.contextState.free(game.inputManager, game.uiManager, game.renderer);
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
