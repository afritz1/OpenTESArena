#include "CommonUiView.h"
#include "MainQuestSplashPanel.h"
#include "MainQuestSplashUiController.h"
#include "MainQuestSplashUiModel.h"
#include "MainQuestSplashUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/FontLibrary.h"

#include "components/utilities/String.h"

MainQuestSplashPanel::MainQuestSplashPanel(Game &game)
	: Panel(game) { }

bool MainQuestSplashPanel::init(int provinceID)
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const std::string descriptionText = MainQuestSplashUiModel::getDungeonText(game, provinceID);
	const TextBoxInitInfo descriptionTextBoxInitInfo =
		MainQuestSplashUiView::getDescriptionTextBoxInitInfo(descriptionText, fontLibrary);
	if (!this->textBox.init(descriptionTextBoxInitInfo, descriptionText, renderer))
	{
		DebugLogError("Couldn't init description text box.");
		return false;
	}

	this->exitButton = Button<Game&>(
		MainQuestSplashUiView::ExitButtonX,
		MainQuestSplashUiView::ExitButtonY,
		MainQuestSplashUiView::ExitButtonWidth,
		MainQuestSplashUiView::ExitButtonHeight,
		MainQuestSplashUiController::onExitButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->exitButton.getRect(),
		[this, &game]() { this->exitButton.click(game); });

	this->addInputActionListener(InputActionName::Back,
		[this, &game](const InputActionCallbackValues &values)
	{
		if (values.performed)
		{
			this->exitButton.click(game);
		}
	});

	auto &textureManager = game.textureManager;
	UiTextureID splashTextureID = MainQuestSplashUiView::allocSplashTextureID(game, provinceID);
	this->splashTextureRef.init(splashTextureID, renderer);

	UiDrawCallInitInfo splashDrawCallInitInfo;
	splashDrawCallInitInfo.textureID = this->splashTextureRef.get();
	splashDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(splashDrawCallInitInfo);

	const Rect textBoxRect = this->textBox.getRect();
	UiDrawCallInitInfo textDrawCallInitInfo;
	textDrawCallInitInfo.textureID = this->textBox.getTextureID();
	textDrawCallInitInfo.position = textBoxRect.getTopLeft();
	textDrawCallInitInfo.size = textBoxRect.getSize();
	this->addDrawCall(textDrawCallInitInfo);

	UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
