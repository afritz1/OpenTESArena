#include "CommonUiView.h"
#include "LogbookPanel.h"
#include "LogbookUiController.h"
#include "LogbookUiModel.h"
#include "LogbookUiView.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/FontLibrary.h"

LogbookPanel::LogbookPanel(Game &game)
	: Panel(game) { }

LogbookPanel::~LogbookPanel()
{
	auto &inputManager = this->getGame().inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::Logbook, false);
}

bool LogbookPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const std::string titleText = LogbookUiModel::getTitleText(game);
	const TextBoxInitInfo titleTextBoxInitInfo = LogbookUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	auto &inputManager = game.inputManager;
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

	TextureManager &textureManager = game.textureManager;
	const UiTextureID backgroundTextureID = LogbookUiView::allocBackgroundTexture(textureManager, renderer);
	this->backgroundTextureRef.init(backgroundTextureID, renderer);

	UiDrawCallInitInfo bgDrawCallInitInfo;
	bgDrawCallInitInfo.textureID = this->backgroundTextureRef.get();
	bgDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(bgDrawCallInitInfo);

	const Rect titleTextBoxRect = this->titleTextBox.getRect();
	UiDrawCallInitInfo titleDrawCallInitInfo;
	titleDrawCallInitInfo.textureID = this->titleTextBox.getTextureID();
	titleDrawCallInitInfo.position = titleTextBoxRect.getCenter();
	titleDrawCallInitInfo.size = titleTextBoxRect.getSize();
	titleDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(titleDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), UiPivotType::TopLeft);

	return true;
}
