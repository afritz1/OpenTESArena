#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseRacePanel.h"
#include "CommonUiView.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/Surface.h"

ChooseRacePanel::ChooseRacePanel(Game &game)
	: Panel(game) { }

bool ChooseRacePanel::init()
{
	auto &game = this->getGame();

	this->addInputActionListener(InputActionName::Back, ChooseRaceUiController::onBackToChooseGenderInputAction);
	this->addMouseButtonChangedListener(ChooseRaceUiController::onMouseButtonChanged);

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const UiTextureID backgroundTextureID = ChooseRaceUiView::allocBackgroundTexture(textureManager, renderer);
	const UiTextureID noExitTextureID = ChooseRaceUiView::allocNoExitTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);
	this->noExitTextureRef.init(noExitTextureID, renderer);

	UiDrawCallInitInfo bgDrawCallInitInfo;
	bgDrawCallInitInfo.textureID = this->backgroundTextureRef.get();
	bgDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(bgDrawCallInitInfo);

	UiDrawCallInitInfo noExitDrawCallInitInfo;
	noExitDrawCallInitInfo.textureID = this->noExitTextureRef.get();
	noExitDrawCallInitInfo.position = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	noExitDrawCallInitInfo.size = this->noExitTextureRef.getDimensions();
	noExitDrawCallInitInfo.pivotType = UiPivotType::BottomRight;
	this->addDrawCall(noExitDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	// Push the initial text sub-panel.
	// @todo: scroll unravel animation
	std::unique_ptr<Panel> textSubPanel = ChooseRacePanel::getInitialSubPanel(game);
	game.pushSubPanel(std::move(textSubPanel));

	return true;
}

std::unique_ptr<Panel> ChooseRacePanel::getInitialSubPanel(Game &game)
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
}
