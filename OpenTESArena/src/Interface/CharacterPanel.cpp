#include <map>
#include <vector>

#include "CharacterPanel.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "CommonUiView.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/FontLibrary.h"

#include "components/debug/Debug.h"

CharacterPanel::CharacterPanel(Game &game)
	: Panel(game) { }

CharacterPanel::~CharacterPanel()
{
	auto &inputManager = this->getGame().inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, false);
}

bool CharacterPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const std::string playerNameText = CharacterSheetUiModel::getPlayerName(game);
	const TextBox::InitInfo playerNameTextBoxInitInfo =
		CharacterSheetUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->playerNameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const std::string playerRaceText = CharacterSheetUiModel::getPlayerRaceName(game);
	const TextBox::InitInfo playerRaceTextBoxInitInfo =
		CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(playerRaceText, fontLibrary);
	if (!this->playerRaceTextBox.init(playerRaceTextBoxInitInfo, playerRaceText, renderer))
	{
		DebugLogError("Couldn't init player race text box.");
		return false;
	}

	const std::string playerClassText = CharacterSheetUiModel::getPlayerClassName(game);
	const TextBox::InitInfo playerClassTextBoxInitInfo =
		CharacterSheetUiView::getPlayerClassTextBoxInitInfo(playerClassText, fontLibrary);
	if (!this->playerClassTextBox.init(playerClassTextBoxInitInfo, playerClassText, renderer))
	{
		DebugLogError("Couldn't init player class text box.");
		return false;
	}

	const PrimaryAttributes &playerAttributes = CharacterSheetUiModel::getPlayerAttributes(game);
	const BufferView<const PrimaryAttribute> playerAttributesView = playerAttributes.getAttributes();
	const std::vector<TextBox::InitInfo> playerAttributesTextBoxInitInfos = CharacterSheetUiView::getPlayerAttributeTextBoxInitInfos(playerAttributesView, fontLibrary);
	for (int i = 0; i < playerAttributesView.getCount(); i++)
	{
		const PrimaryAttribute &attribute = playerAttributesView.get(i);
		const int attributeValue = attribute.maxValue;
		const std::string attributeValueText = std::to_string(attributeValue);
		DebugAssertIndex(playerAttributesTextBoxInitInfos, i);
		const TextBox::InitInfo &attributeTextBoxInitInfo = playerAttributesTextBoxInitInfos[i];
		if (!this->playerAttributeTextBoxes[i].init(attributeTextBoxInitInfo, attributeValueText, renderer))
		{
			DebugLogError("Couldn't init player attribute " + std::string(attribute.name) + " text box.");
			return false;
		}
	}

	this->doneButton = Button<Game&>(
		CharacterSheetUiView::DoneButtonCenterPoint,
		CharacterSheetUiView::DoneButtonWidth,
		CharacterSheetUiView::DoneButtonHeight,
		CharacterSheetUiController::onDoneButtonSelected);
	this->nextPageButton = Button<Game&>(
		CharacterSheetUiView::NextPageButtonX,
		CharacterSheetUiView::NextPageButtonY,
		CharacterSheetUiView::NextPageButtonWidth,
		CharacterSheetUiView::NextPageButtonHeight,
		CharacterSheetUiController::onNextPageButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->doneButton.getRect(),
		[this, &game]() { this->doneButton.click(game); });
	this->addButtonProxy(MouseButtonType::Left, this->nextPageButton.getRect(),
		[this, &game]() { this->nextPageButton.click(game); });

	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterSheet, true);

	auto doneInputActionFunc = CharacterSheetUiController::onDoneInputAction;
	this->addInputActionListener(InputActionName::Back, doneInputActionFunc);
	this->addInputActionListener(InputActionName::CharacterSheet, doneInputActionFunc);

	auto &textureManager = game.textureManager;
	const UiTextureID bodyTextureID = CharacterSheetUiView::allocBodyTexture(game);
	const UiTextureID pantsTextureID = CharacterSheetUiView::allocPantsTexture(game);
	const UiTextureID headTextureID = CharacterSheetUiView::allocHeadTexture(game);
	const UiTextureID shirtTextureID = CharacterSheetUiView::allocShirtTexture(game);
	const UiTextureID statsBgTextureID = CharacterSheetUiView::allocStatsBgTexture(textureManager, renderer);
	const UiTextureID nextPageTextureID = CharacterSheetUiView::allocNextPageTexture(textureManager, renderer);
	this->bodyTextureRef.init(bodyTextureID, renderer);
	this->pantsTextureRef.init(pantsTextureID, renderer);
	this->headTextureRef.init(headTextureID, renderer);
	this->shirtTextureRef.init(shirtTextureID, renderer);
	this->statsBgTextureRef.init(statsBgTextureID, renderer);
	this->nextPageTextureRef.init(nextPageTextureID, renderer);

	const Int2 bodyTextureDims = *renderer.tryGetUiTextureDims(bodyTextureID);
	const Int2 pantsTextureDims = *renderer.tryGetUiTextureDims(pantsTextureID);
	const Int2 headTextureDims = *renderer.tryGetUiTextureDims(headTextureID);
	const Int2 shirtTextureDims = *renderer.tryGetUiTextureDims(shirtTextureID);
	const Int2 statsBgTextureDims = *renderer.tryGetUiTextureDims(statsBgTextureID);
	const Int2 nextPageTextureDims = *renderer.tryGetUiTextureDims(nextPageTextureID);

	this->addDrawCall(
		bodyTextureID,
		CharacterSheetUiView::getBodyOffset(game),
		bodyTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		pantsTextureID,
		CharacterSheetUiView::getPantsOffset(game),
		pantsTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		headTextureID,
		CharacterSheetUiView::getHeadOffset(game),
		headTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		shirtTextureID,
		CharacterSheetUiView::getShirtOffset(game),
		shirtTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		statsBgTextureID,
		Int2::Zero,
		statsBgTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		nextPageTextureID,
		CharacterSheetUiView::getNextPageOffset(),
		nextPageTextureDims,
		PivotType::TopLeft);

	const Rect &playerNameTextBoxRect = this->playerNameTextBox.getRect();
	this->addDrawCall(
		this->playerNameTextBox.getTextureID(),
		playerNameTextBoxRect.getTopLeft(),
		Int2(playerNameTextBoxRect.getWidth(), playerNameTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect &playerRaceTextBoxRect = this->playerRaceTextBox.getRect();
	this->addDrawCall(
		this->playerRaceTextBox.getTextureID(),
		playerRaceTextBoxRect.getTopLeft(),
		Int2(playerRaceTextBoxRect.getWidth(), playerRaceTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect &playerClassTextBoxRect = this->playerClassTextBox.getRect();
	this->addDrawCall(
		this->playerClassTextBox.getTextureID(),
		playerClassTextBoxRect.getTopLeft(),
		Int2(playerClassTextBoxRect.getWidth(), playerClassTextBoxRect.getHeight()),
		PivotType::TopLeft);

	for (TextBox &playerAttributeTextBox : this->playerAttributeTextBoxes)
	{
		const Rect &playerAttributeTextBoxRect = playerAttributeTextBox.getRect();
		this->addDrawCall(
			playerAttributeTextBox.getTextureID(),
			playerAttributeTextBoxRect.getTopLeft(),
			Int2(playerAttributeTextBoxRect.getWidth(), playerAttributeTextBoxRect.getHeight()),
			PivotType::TopLeft);
	}

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	return true;
}
