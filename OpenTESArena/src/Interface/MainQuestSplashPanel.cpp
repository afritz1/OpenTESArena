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
	const TextBox::InitInfo descriptionTextBoxInitInfo =
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

	this->addDrawCall(
		this->splashTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);

	const Rect &textBoxRect = this->textBox.getRect();
	this->addDrawCall(
		textBox.getTextureID(),
		textBoxRect.getTopLeft(),
		Int2(textBoxRect.width, textBoxRect.height),
		PivotType::TopLeft);

	UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
