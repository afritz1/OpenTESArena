#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseGenderPanel.h"
#include "CommonUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/FontLibrary.h"

ChooseGenderPanel::ChooseGenderPanel(Game &game)
	: Panel(game) { }

bool ChooseGenderPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();

	const auto &fontLibrary = FontLibrary::getInstance();
	const std::string titleText = ChooseGenderUiModel::getTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo = ChooseGenderUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const std::string maleText = ChooseGenderUiModel::getMaleText(game);
	const TextBox::InitInfo maleTextBoxInitInfo = ChooseGenderUiView::getMaleTextBoxInitInfo(maleText, fontLibrary);
	if (!this->maleTextBox.init(maleTextBoxInitInfo, maleText, renderer))
	{
		DebugLogError("Couldn't init male text box.");
		return false;
	}

	const std::string femaleText = ChooseGenderUiModel::getFemaleText(game);
	const TextBox::InitInfo femaleTextBoxInitInfo = ChooseGenderUiView::getFemaleTextBoxInitInfo(femaleText, fontLibrary);
	if (!this->femaleTextBox.init(femaleTextBoxInitInfo, femaleText, renderer))
	{
		DebugLogError("Couldn't init female text box.");
		return false;
	}

	this->maleButton = Button<Game&>(
		ChooseGenderUiView::MaleButtonCenter,
		ChooseGenderUiView::MaleButtonWidth,
		ChooseGenderUiView::MaleButtonHeight,
		ChooseGenderUiController::onMaleButtonSelected);
	this->femaleButton = Button<Game&>(
		ChooseGenderUiView::FemaleButtonCenter,
		ChooseGenderUiView::FemaleButtonWidth,
		ChooseGenderUiView::FemaleButtonHeight,
		ChooseGenderUiController::onFemaleButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->maleButton.getRect(),
		[this, &game]() { this->maleButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->femaleButton.getRect(),
		[this, &game]() { this->femaleButton.click(game); });

	this->addInputActionListener(InputActionName::Back, ChooseGenderUiController::onBackToChooseNameInputAction);

	auto &textureManager = game.getTextureManager();
	const UiTextureID nightSkyTextureID = CharacterCreationUiView::allocNightSkyTexture(textureManager, renderer);
	const UiTextureID parchmentTextureID = ChooseGenderUiView::allocParchmentTexture(textureManager, renderer);
	this->nightSkyTextureRef.init(nightSkyTextureID, renderer);
	this->parchmentTextureRef.init(parchmentTextureID, renderer);

	this->addDrawCall(
		this->nightSkyTextureRef.get(),
		Int2::Zero,
		Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT),
		PivotType::TopLeft);

	const Int2 parchmentSize(this->parchmentTextureRef.getWidth(), this->parchmentTextureRef.getHeight());
	this->addDrawCall(
		this->parchmentTextureRef.get(),
		ChooseGenderUiView::getTitleTextureCenter(),
		parchmentSize,
		PivotType::Middle);
	this->addDrawCall(
		this->parchmentTextureRef.get(),
		ChooseGenderUiView::getMaleTextureCenter(),
		parchmentSize,
		PivotType::Middle);
	this->addDrawCall(
		this->parchmentTextureRef.get(),
		ChooseGenderUiView::getFemaleTextureCenter(),
		parchmentSize,
		PivotType::Middle);

	const Rect &titleTextBoxRect = this->titleTextBox.getRect();
	this->addDrawCall(
		this->titleTextBox.getTextureID(),
		titleTextBoxRect.getCenter(),
		Int2(titleTextBoxRect.getWidth(), titleTextBoxRect.getHeight()),
		PivotType::Middle);

	const Rect &maleTextBoxRect = this->maleTextBox.getRect();
	this->addDrawCall(
		this->maleTextBox.getTextureID(),
		maleTextBoxRect.getCenter(),
		Int2(maleTextBoxRect.getWidth(), maleTextBoxRect.getHeight()),
		PivotType::Middle);

	const Rect &femaleTextBoxRect = this->femaleTextBox.getRect();
	this->addDrawCall(
		this->femaleTextBox.getTextureID(),
		femaleTextBoxRect.getCenter(),
		Int2(femaleTextBoxRect.getWidth(), femaleTextBoxRect.getHeight()),
		PivotType::Middle);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
