#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "CommonUiView.h"
#include "MainMenuPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ExeData.h"
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
	const TextBox::InitInfo titleTextBoxInitInfo =
		ChooseClassCreationUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const std::string generateText = ChooseClassCreationUiModel::getGenerateButtonText(game);
	const TextBox::InitInfo generateTextBoxInitInfo =
		ChooseClassCreationUiView::getGenerateTextBoxInitInfo(generateText, fontLibrary);
	if (!this->generateTextBox.init(generateTextBoxInitInfo, generateText, renderer))
	{
		DebugLogError("Couldn't init generate class text box.");
		return false;
	}

	const std::string selectText = ChooseClassCreationUiModel::getSelectButtonText(game);
	const TextBox::InitInfo selectTextBoxInitInfo =
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

	this->addDrawCall(
		this->nightSkyTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);

	const Int2 parchmentSize(
		this->parchmentTextureRef.getWidth(),
		this->parchmentTextureRef.getHeight());
	this->addDrawCall(
		this->parchmentTextureRef.get(),
		ChooseClassCreationUiView::getTitleTextureCenter(),
		parchmentSize,
		PivotType::Middle);
	this->addDrawCall(
		this->parchmentTextureRef.get(),
		ChooseClassCreationUiView::getGenerateTextureCenter(),
		parchmentSize,
		PivotType::Middle);
	this->addDrawCall(
		this->parchmentTextureRef.get(),
		ChooseClassCreationUiView::getSelectTextureCenter(),
		parchmentSize,
		PivotType::Middle);

	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	this->addDrawCall(
		this->titleTextBox.getTextureID(),
		titleTextBoxRect.getCenter(),
		Int2(titleTextBoxRect.getWidth(), titleTextBoxRect.getHeight()),
		PivotType::Middle);

	const Rect &generateTextBoxRect = this->generateTextBox.getRect();
	this->addDrawCall(
		this->generateTextBox.getTextureID(),
		generateTextBoxRect.getCenter(),
		Int2(generateTextBoxRect.getWidth(), generateTextBoxRect.getHeight()),
		PivotType::Middle);

	const Rect &selectTextBoxRect = this->selectTextBox.getRect();
	this->addDrawCall(
		this->selectTextBox.getTextureID(),
		selectTextBoxRect.getCenter(),
		Int2(selectTextBoxRect.getWidth(), selectTextBoxRect.getHeight()),
		PivotType::Middle);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
