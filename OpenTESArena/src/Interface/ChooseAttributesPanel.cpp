#include <algorithm>
#include <array>

#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiView.h"
#include "ChooseAttributesPanel.h"
#include "CommonUiView.h"
#include "TextSubPanel.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../UI/CursorData.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game) { }

bool ChooseAttributesPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setPortraitIndex(0);

	this->attributesAreSaved = false;

	const std::string playerNameText = CharacterCreationUiModel::getPlayerName(game);
	const TextBox::InitInfo playerNameTextBoxInitInfo =
		CharacterSheetUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->nameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const std::string playerRaceText = CharacterCreationUiModel::getPlayerRaceName(game);
	const TextBox::InitInfo playerRaceTextBoxInitInfo =
		CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(playerRaceText, fontLibrary);
	if (!this->raceTextBox.init(playerRaceTextBoxInitInfo, playerRaceText, renderer))
	{
		DebugLogError("Couldn't init player race text box.");
		return false;
	}

	const std::string playerClassText = CharacterCreationUiModel::getPlayerClassName(game);
	const TextBox::InitInfo playerClassTextBoxInitInfo =
		CharacterSheetUiView::getPlayerClassTextBoxInitInfo(playerClassText, fontLibrary);
	if (!this->classTextBox.init(playerClassTextBoxInitInfo, playerClassText, renderer))
	{
		DebugLogError("Couldn't init player class text box.");
		return false;
	}

	this->doneButton = Button<Game&, bool*>(
		CharacterSheetUiView::DoneButtonCenterPoint,
		CharacterSheetUiView::DoneButtonWidth,
		CharacterSheetUiView::DoneButtonHeight,
		ChooseAttributesUiController::onDoneButtonSelected);
	this->portraitButton = Button<Game&, bool>(
		ChooseAttributesUiView::PortraitButtonCenterPoint,
		ChooseAttributesUiView::PortraitButtonWidth,
		ChooseAttributesUiView::PortraitButtonHeight,
		ChooseAttributesUiController::onPortraitButtonSelected);

	this->addButtonProxy(MouseButtonType::Left, this->doneButton.getRect(),
		[this, &game]() { this->doneButton.click(game, &this->attributesAreSaved); });
	this->addButtonProxy(MouseButtonType::Left, this->portraitButton.getRect(),
		[this, &game]()
	{
		if (this->attributesAreSaved)
		{
			// Increment the portrait ID.
			this->portraitButton.click(game, true);
		}
	});

	this->addButtonProxy(MouseButtonType::Right, this->portraitButton.getRect(),
		[this, &game]()
	{
		if (this->attributesAreSaved)
		{
			// Decrement the portrait ID.
			this->portraitButton.click(game, false);
		}
	});

	this->addInputActionListener(InputActionName::Back, ChooseAttributesUiController::onBackToRaceSelectionInputAction);

	auto &textureManager = game.getTextureManager();
	const UiTextureID bodyTextureID = ChooseAttributesUiView::allocBodyTexture(game);
	const UiTextureID pantsTextureID = ChooseAttributesUiView::allocPantsTexture(game);
	const UiTextureID shirtTextureID = ChooseAttributesUiView::allocShirtTexture(game);
	const UiTextureID statsBgTextureID = ChooseAttributesUiView::allocStatsBgTexture(textureManager, renderer);
	this->bodyTextureRef.init(bodyTextureID, renderer);
	this->pantsTextureRef.init(pantsTextureID, renderer);
	this->shirtTextureRef.init(shirtTextureID, renderer);
	this->statsBgTextureRef.init(statsBgTextureID, renderer);

	const Buffer<TextureAssetReference> headTextureAssetRefs = ChooseAttributesUiView::getHeadTextureAssetRefs(game);
	this->headTextureRefs.init(headTextureAssetRefs.getCount());
	for (int i = 0; i < headTextureAssetRefs.getCount(); i++)
	{
		const TextureAssetReference &headTextureAssetRef = headTextureAssetRefs.get(i);
		const UiTextureID headTextureID = ChooseAttributesUiView::allocHeadTexture(headTextureAssetRef, textureManager, renderer);
		this->headTextureRefs.set(i, ScopedUiTextureRef(headTextureID, renderer));
	}

	const Int2 bodyTextureDims = *renderer.tryGetUiTextureDims(bodyTextureID);
	const Int2 pantsTextureDims = *renderer.tryGetUiTextureDims(pantsTextureID);
	const Int2 shirtTextureDims = *renderer.tryGetUiTextureDims(shirtTextureID);
	const Int2 statsBgTextureDims = *renderer.tryGetUiTextureDims(statsBgTextureID);

	UiDrawCall::TextureFunc headTextureFunc = [this, &game]()
	{
		const auto &charCreationState = game.getCharacterCreationState();
		const int portraitIndex = charCreationState.getPortraitIndex();
		const ScopedUiTextureRef &headTextureRef = this->headTextureRefs.get(portraitIndex);
		return headTextureRef.get();
	};

	UiDrawCall::PositionFunc headPositionFunc = [&game]()
	{
		return ChooseAttributesUiView::getHeadOffset(game);
	};

	UiDrawCall::SizeFunc headSizeFunc = [this, &game]()
	{
		const auto &charCreationState = game.getCharacterCreationState();
		const int portraitIndex = charCreationState.getPortraitIndex();
		const ScopedUiTextureRef &headTextureRef = this->headTextureRefs.get(portraitIndex);
		return Int2(headTextureRef.getWidth(), headTextureRef.getHeight());
	};

	UiDrawCall::PivotFunc headPivotFunc = [this]()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		bodyTextureID,
		ChooseAttributesUiView::getBodyOffset(game),
		bodyTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		pantsTextureID,
		ChooseAttributesUiView::getPantsOffset(game),
		pantsTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		headTextureFunc,
		headPositionFunc,
		headSizeFunc,
		headPivotFunc,
		UiDrawCall::defaultActiveFunc);
	this->addDrawCall(
		shirtTextureID,
		ChooseAttributesUiView::getShirtOffset(game),
		shirtTextureDims,
		PivotType::TopLeft);
	this->addDrawCall(
		statsBgTextureID,
		Int2::Zero,
		statsBgTextureDims,
		PivotType::TopLeft);

	const Rect &nameTextBoxRect = this->nameTextBox.getRect();
	this->addDrawCall(
		this->nameTextBox.getTextureID(),
		nameTextBoxRect.getTopLeft(),
		Int2(nameTextBoxRect.getWidth(), nameTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect &raceTextBoxRect = this->raceTextBox.getRect();
	this->addDrawCall(
		this->raceTextBox.getTextureID(),
		raceTextBoxRect.getTopLeft(),
		Int2(raceTextBoxRect.getWidth(), raceTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect &classTextBoxRect = this->classTextBox.getRect();
	this->addDrawCall(
		this->classTextBox.getTextureID(),
		classTextBoxRect.getTopLeft(),
		Int2(classTextBoxRect.getWidth(), classTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	// Push the initial text pop-up onto the sub-panel stack.
	const std::string initialPopUpText = ChooseAttributesUiModel::getInitialText(game);
	const TextBox::InitInfo initialPopUpTextBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		initialPopUpText,
		ChooseAttributesUiView::InitialTextCenterPoint,
		ChooseAttributesUiView::InitialTextFontName,
		ChooseAttributesUiView::InitialTextColor,
		ChooseAttributesUiView::InitialTextAlignment,
		std::nullopt,
		ChooseAttributesUiView::InitialTextLineSpacing,
		fontLibrary);

	Texture initialTexture = TextureUtils::generate(
		ChooseAttributesUiView::InitialTextPatternType,
		ChooseAttributesUiView::getInitialTextureWidth(),
		ChooseAttributesUiView::getInitialTextureHeight(),
		game.getTextureManager(),
		renderer);

	game.pushSubPanel<TextSubPanel>(initialPopUpTextBoxInitInfo, initialPopUpText,
		ChooseAttributesUiController::onInitialPopUpSelected, std::move(initialTexture),
		ChooseAttributesUiView::InitialTextureCenterPoint);

	return true;
}
