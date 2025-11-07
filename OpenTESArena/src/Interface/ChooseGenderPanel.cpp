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
	auto &renderer = game.renderer;

	const auto &fontLibrary = FontLibrary::getInstance();
	const std::string titleText = ChooseGenderUiModel::getTitleText(game);
	const TextBoxInitInfo titleTextBoxInitInfo = ChooseGenderUiView::getTitleTextBoxInitInfo(titleText, fontLibrary);
	if (!this->titleTextBox.init(titleTextBoxInitInfo, titleText, renderer))
	{
		DebugLogError("Couldn't init title text box.");
		return false;
	}

	const std::string maleText = ChooseGenderUiModel::getMaleText(game);
	const TextBoxInitInfo maleTextBoxInitInfo = ChooseGenderUiView::getMaleTextBoxInitInfo(maleText, fontLibrary);
	if (!this->maleTextBox.init(maleTextBoxInitInfo, maleText, renderer))
	{
		DebugLogError("Couldn't init male text box.");
		return false;
	}

	const std::string femaleText = ChooseGenderUiModel::getFemaleText(game);
	const TextBoxInitInfo femaleTextBoxInitInfo = ChooseGenderUiView::getFemaleTextBoxInitInfo(femaleText, fontLibrary);
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

	auto &textureManager = game.textureManager;
	const UiTextureID nightSkyTextureID = CharacterCreationUiView::allocNightSkyTexture(textureManager, renderer);
	const UiTextureID parchmentTextureID = ChooseGenderUiView::allocParchmentTexture(textureManager, renderer);
	this->nightSkyTextureRef.init(nightSkyTextureID, renderer);
	this->parchmentTextureRef.init(parchmentTextureID, renderer);

	UiDrawCallInitInfo nightSkyDrawCallInitInfo;
	nightSkyDrawCallInitInfo.textureID = this->nightSkyTextureRef.get();
	nightSkyDrawCallInitInfo.size = Int2(ArenaRenderUtils::SCREEN_WIDTH, ArenaRenderUtils::SCREEN_HEIGHT);
	this->addDrawCall(nightSkyDrawCallInitInfo);

	UiDrawCallInitInfo parchmentDrawCallInitInfo;
	parchmentDrawCallInitInfo.textureID = this->parchmentTextureRef.get();
	parchmentDrawCallInitInfo.size = this->parchmentTextureRef.getDimensions();
	parchmentDrawCallInitInfo.pivotType = UiPivotType::Middle;

	UiDrawCallInitInfo titleParchmentDrawCallInitInfo = parchmentDrawCallInitInfo;
	titleParchmentDrawCallInitInfo.position = ChooseGenderUiView::getTitleTextureCenter();
	this->addDrawCall(titleParchmentDrawCallInitInfo);

	UiDrawCallInitInfo maleParchmentDrawCallInitInfo = parchmentDrawCallInitInfo;
	maleParchmentDrawCallInitInfo.position = ChooseGenderUiView::getMaleTextureCenter();
	this->addDrawCall(maleParchmentDrawCallInitInfo);

	UiDrawCallInitInfo femaleParchmentDrawCallInitInfo = parchmentDrawCallInitInfo;
	femaleParchmentDrawCallInitInfo.position = ChooseGenderUiView::getFemaleTextureCenter();
	this->addDrawCall(femaleParchmentDrawCallInitInfo);

	const Rect titleTextBoxRect = this->titleTextBox.getRect();
	UiDrawCallInitInfo titleDrawCallInitInfo;
	titleDrawCallInitInfo.textureID = this->titleTextBox.getTextureID();
	titleDrawCallInitInfo.position = titleTextBoxRect.getCenter();
	titleDrawCallInitInfo.size = titleTextBoxRect.getSize();
	titleDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(titleDrawCallInitInfo);

	const Rect maleTextBoxRect = this->maleTextBox.getRect();
	UiDrawCallInitInfo maleTextDrawCallInitInfo;
	maleTextDrawCallInitInfo.textureID = this->maleTextBox.getTextureID();
	maleTextDrawCallInitInfo.position = maleTextBoxRect.getCenter();
	maleTextDrawCallInitInfo.size = maleTextBoxRect.getSize();
	maleTextDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(maleTextDrawCallInitInfo);

	const Rect femaleTextBoxRect = this->femaleTextBox.getRect();
	UiDrawCallInitInfo femaleTextDrawCallInitInfo;
	femaleTextDrawCallInitInfo.textureID = this->femaleTextBox.getTextureID();
	femaleTextDrawCallInitInfo.position = femaleTextBoxRect.getCenter();
	femaleTextDrawCallInitInfo.size = femaleTextBoxRect.getSize();
	femaleTextDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(femaleTextDrawCallInitInfo);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
