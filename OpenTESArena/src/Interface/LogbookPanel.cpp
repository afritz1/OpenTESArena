#include "CommonUiView.h"
#include "LogbookPanel.h"
#include "LogbookUiController.h"
#include "LogbookUiModel.h"
#include "LogbookUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"

LogbookPanel::LogbookPanel(Game &game)
	: Panel(game) { }

LogbookPanel::~LogbookPanel()
{
	auto &inputManager = this->getGame().getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Logbook, false);
}

bool LogbookPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	const std::string titleText = LogbookUiModel::getTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo = LogbookUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::Logbook, true);

	this->backButton = Button<Game&>(
		LogbookUiView::BackButtonCenterPoint,
		LogbookUiView::BackButtonWidth,
		LogbookUiView::BackButtonHeight,
		LogbookUiController::onBackButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->backButton.getRect(),
		[this, &game]() { this->backButton.click(game); });

	auto backInputActionFunc = [this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->backButton.click(game);
		}
	};

	this->addInputActionListener(InputActionName::Back, backInputActionFunc);
	this->addInputActionListener(InputActionName::Logbook, backInputActionFunc);

	auto &textureManager = game.getTextureManager();
	const UiTextureID backgroundTextureID = LogbookUiView::allocBackgroundTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);
	this->addDrawCall(
		this->backgroundTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);

	const Rect &titleTextBox = this->titleTextBox.getRect();
	this->addDrawCall(
		this->titleTextBox.getTextureID(),
		titleTextBox.getCenter(),
		Int2(titleTextBox.getWidth(), titleTextBox.getHeight()),
		PivotType::Middle);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), PivotType::TopLeft);

	return true;
}
