#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "CommonUiView.h"
#include "MainMenuPanel.h"
#include "../Assets/TextureManager.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Input/InputActionName.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../Utilities/Color.h"

#include "components/utilities/String.h"

ChooseClassCreationPanel::ChooseClassCreationPanel(Game &game)
	: Panel(game) { }

bool ChooseClassCreationPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;

	const auto &fontLibrary = FontLibrary::getInstance();
	const std::string titleText = ChooseClassCreationUiModel::getTitleText(game);
	const TextBoxInitInfo titleTextBoxInitInfo =
		ChooseClassCreationUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const std::string generateText = ChooseClassCreationUiModel::getGenerateButtonText(game);
	const TextBoxInitInfo generateTextBoxInitInfo =
		ChooseClassCreationUiView::getGenerateTextBoxInitInfo(generateText, fontLibrary);
	if (!this->generateTextBox.init(generateTextBoxInitInfo, generateText, renderer))
	{
		DebugLogError("Couldn't init generate class text box.");
		return false;
	}

	const std::string selectText = ChooseClassCreationUiModel::getSelectButtonText(game);
	const TextBoxInitInfo selectTextBoxInitInfo =
		ChooseClassCreationUiView::getSelectTextBoxInitInfo(selectText, fontLibrary);
	if (!this->selectTextBox.init(selectTextBoxInitInfo, selectText, renderer))
	{
		DebugLogError("Couldn't init select class text box.");
		return false;
	}

	this->generateButton = Button<Game&>(
		ChooseClassCreationUiView::GenerateButtonCenterPoint,
		ChooseClassCreationUiView::GenerateButtonWidth,
		ChooseClassCreationUiView::GenerateButtonHeight,
		ChooseClassCreationUiController::onGenerateButtonSelected);
	this->selectButton = Button<Game&>(
		ChooseClassCreationUiView::SelectButtonCenterPoint,
		ChooseClassCreationUiView::SelectButtonWidth,
		ChooseClassCreationUiView::SelectButtonHeight,
		ChooseClassCreationUiController::onSelectButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->generateButton.getRect(),
		[this, &game]() { this->generateButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->selectButton.getRect(),
		[this, &game]() { this->selectButton.click(game); });

	this->addInputActionListener(InputActionName::Back, ChooseClassCreationUiController::onBackToMainMenuInputAction);

	auto &textureManager = game.textureManager;
	const UiTextureID nightSkyTextureID = CharacterCreationUiView::allocNightSkyTexture(textureManager, renderer);
	const UiTextureID parchmentTextureID = ChooseClassCreationUiView::allocParchmentTexture(textureManager, renderer);
	this->nightSkyTextureRef.init(nightSkyTextureID, renderer);
	this->parchmentTextureRef.init(parchmentTextureID, renderer);

	UiDrawCallInitInfo nightSkyDrawCallInitInfo;
	nightSkyDrawCallInitInfo.textureID = this->nightSkyTextureRef.get();
	nightSkyDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(nightSkyDrawCallInitInfo);

	UiDrawCallInitInfo parchmentDrawCallInitInfo;
	parchmentDrawCallInitInfo.textureID = this->parchmentTextureRef.get();
	parchmentDrawCallInitInfo.size = this->parchmentTextureRef.getDimensions();
	parchmentDrawCallInitInfo.pivotType = PivotType::Middle;

	UiDrawCallInitInfo titleParchmentDrawCallInitInfo = parchmentDrawCallInitInfo;
	titleParchmentDrawCallInitInfo.position = ChooseClassCreationUiView::getTitleTextureCenter();
	this->addDrawCall(titleParchmentDrawCallInitInfo);

	UiDrawCallInitInfo generateParchmentDrawCallInitInfo = parchmentDrawCallInitInfo;
	generateParchmentDrawCallInitInfo.position = ChooseClassCreationUiView::getGenerateTextureCenter();
	this->addDrawCall(generateParchmentDrawCallInitInfo);

	UiDrawCallInitInfo selectParchmentDrawCallInitInfo = parchmentDrawCallInitInfo;
	selectParchmentDrawCallInitInfo.position = ChooseClassCreationUiView::getSelectTextureCenter();
	this->addDrawCall(selectParchmentDrawCallInitInfo);

	const Rect titleTextBoxRect = this->titleTextBox.getRect();
	UiDrawCallInitInfo titleTextDrawCallInitInfo;
	titleTextDrawCallInitInfo.textureID = this->titleTextBox.getTextureID();
	titleTextDrawCallInitInfo.position = titleTextBoxRect.getCenter();
	titleTextDrawCallInitInfo.size = titleTextBoxRect.getSize();
	titleTextDrawCallInitInfo.pivotType = PivotType::Middle;
	this->addDrawCall(titleTextDrawCallInitInfo);

	const Rect generateTextBoxRect = this->generateTextBox.getRect();
	UiDrawCallInitInfo generateTextDrawCallInitInfo;
	generateTextDrawCallInitInfo.textureID = this->generateTextBox.getTextureID();
	generateTextDrawCallInitInfo.position = generateTextBoxRect.getCenter();
	generateTextDrawCallInitInfo.size = generateTextBoxRect.getSize();
	generateTextDrawCallInitInfo.pivotType = PivotType::Middle;
	this->addDrawCall(generateTextDrawCallInitInfo);

	const Rect selectTextBoxRect = this->selectTextBox.getRect();
	UiDrawCallInitInfo selectTextDrawCallInitInfo;
	selectTextDrawCallInitInfo.textureID = this->selectTextBox.getTextureID();
	selectTextDrawCallInitInfo.position = selectTextBoxRect.getCenter();
	selectTextDrawCallInitInfo.size = selectTextBoxRect.getSize();
	selectTextDrawCallInitInfo.pivotType = PivotType::Middle;
	this->addDrawCall(selectTextDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
