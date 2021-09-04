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

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const UiTextureID backgroundTextureID = ChooseRaceUiView::allocBackgroundTexture(textureManager, renderer);
	const UiTextureID noExitTextureID = ChooseRaceUiView::allocNoExitTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);
	this->noExitTextureRef.init(noExitTextureID, renderer);

	this->addDrawCall(
		this->backgroundTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);
	this->addDrawCall(
		this->noExitTextureRef.get(),
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		Int2(this->noExitTextureRef.getWidth(), this->noExitTextureRef.getHeight()),
		PivotType::BottomRight);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	// Push the initial text sub-panel.
	// @todo: allocate std::function for unravelling the map with "push initial parchment sub-panel" on finished,
	// setting the std::function to empty at that time?
	std::unique_ptr<Panel> textSubPanel = ChooseRacePanel::getInitialSubPanel(game);
	game.pushSubPanel(std::move(textSubPanel));

	return true;
}

std::unique_ptr<Panel> ChooseRacePanel::getInitialSubPanel(Game &game)
{
	const std::string text = ChooseRaceUiModel::getTitleText(game);
	const TextBox::InitInfo textBoxInitInfo = ChooseRaceUiView::getInitialPopUpTextBoxInitInfo(text, game);

	// @todo: change to UiTextureID probably and put in ChooseRaceUiView once TextSubPanel gets revised.
	auto &renderer = game.getRenderer();
	Surface surface = TextureUtils::generate(
		ChooseRaceUiView::InitialPopUpPatternType,
		ChooseRaceUiView::InitialPopUpTextureWidth,
		ChooseRaceUiView::InitialPopUpTextureHeight,
		game.getTextureManager(),
		renderer);
	Texture texture = renderer.createTextureFromSurface(surface);

	std::unique_ptr<TextSubPanel> subPanel = std::make_unique<TextSubPanel>(game);
	if (!subPanel->init(textBoxInitInfo, text, ChooseRaceUiController::onInitialPopUpButtonSelected,
		std::move(texture), ChooseRaceUiView::InitialPopUpTextureCenterPoint))
	{
		DebugCrash("Couldn't init choose race initial sub-panel.");
	}

	return subPanel;
}
