#include <algorithm>

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiView.h"
#include "ChooseAttributesPanel.h"
#include "CommonUiView.h"
#include "TextSubPanel.h"
#include "../Assets/TextureUtils.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Math/Random.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include <OpenTESArena/src/Player/ArenaPlayerUtils.h>

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game)
{
	this->bonusPoints = 0;
	this->selectedAttributeIndex = 0;
}

void ChooseAttributesPanel::populateBaseAttributesRandomly(CharacterCreationState &charCreationState, ArenaRandom &random)
{
	charCreationState.populateBaseAttributes();

	BufferView<PrimaryAttribute> attributes = charCreationState.attributes.getAttributes();
	for (int i = 0; i < PrimaryAttributes::COUNT; i++)
	{
		PrimaryAttribute &attribute = attributes[i];
		const int addedValue = ChooseAttributesUiModel::rollClassic(ChooseAttributesUiModel::PrimaryAttributeRandomMax, random);
		attribute.maxValue += addedValue;	
	}
}

bool ChooseAttributesPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.portraitIndex = 0;
	charCreationState.clearChangedPoints();

	ArenaRandom &arenaRandom = game.arenaRandom;
	this->populateBaseAttributesRandomly(charCreationState, arenaRandom);
	this->bonusPoints = ChooseAttributesUiModel::rollClassic(ChooseAttributesUiModel::BonusPointsRandomMax, arenaRandom);

	BufferView<PrimaryAttribute> attributes = charCreationState.attributes.getAttributes();
	for (int i = 0; i < PrimaryAttributes::COUNT; i++)
	{
		const PrimaryAttribute &attribute = attributes[i];
		if (strcmp(attribute.name, "Agility") == 0) {
			this->bonusToHitValue = ArenaPlayerUtils::calculateBonusToHit(attribute.maxValue);
			break;
		}
	}

	this->selectedAttributeIndex = 0;
	this->attributesAreSaved = false;

	const std::string playerNameText = CharacterCreationUiModel::getPlayerName(game);
	const TextBox::InitInfo playerNameTextBoxInitInfo = CharacterSheetUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->nameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const std::string playerRaceText = CharacterCreationUiModel::getPlayerRaceName(game);
	const TextBox::InitInfo playerRaceTextBoxInitInfo = CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(playerRaceText, fontLibrary);
	if (!this->raceTextBox.init(playerRaceTextBoxInitInfo, playerRaceText, renderer))
	{
		DebugLogError("Couldn't init player race text box.");
		return false;
	}

	const std::string playerClassText = CharacterCreationUiModel::getPlayerClassName(game);
	const TextBox::InitInfo playerClassTextBoxInitInfo = CharacterSheetUiView::getPlayerClassTextBoxInitInfo(playerClassText, fontLibrary);
	if (!this->classTextBox.init(playerClassTextBoxInitInfo, playerClassText, renderer))
	{
		DebugLogError("Couldn't init player class text box.");
		return false;
	}

	const PrimaryAttributes &playerAttributes = CharacterCreationUiModel::getPlayerAttributes(game);
	const BufferView<const PrimaryAttribute> playerAttributesView = playerAttributes.getAttributes();
	const std::vector<TextBox::InitInfo> playerAttributesTextBoxInitInfos = CharacterSheetUiView::getPlayerAttributeTextBoxInitInfos(playerAttributesView, fontLibrary);
	for (int i = 0; i < playerAttributesView.getCount(); i++)
	{
		const PrimaryAttribute &attribute = playerAttributesView.get(i);
		const int attributeValue = attribute.maxValue;
		const std::string attributeValueText = std::to_string(attributeValue);
		DebugAssertIndex(playerAttributesTextBoxInitInfos, i);
		const TextBox::InitInfo &attributeTextBoxInitInfo = playerAttributesTextBoxInitInfos[i];
		if (!this->attributeTextBoxes[i].init(attributeTextBoxInitInfo, attributeValueText, renderer))
		{
			DebugLogError("Couldn't init player attribute " + std::string(attribute.name) + " text box.");
			return false;
		}
	}

	const std::string playerExperienceText = CharacterCreationUiModel::getPlayerExperience(game);
	const TextBox::InitInfo playerExperienceTextBoxInitInfo = CharacterSheetUiView::getPlayerExperienceTextBoxInitInfo(playerExperienceText, fontLibrary);
	if (!this->experienceTextBox.init(playerExperienceTextBoxInitInfo, playerExperienceText, renderer))
	{
		DebugLogError("Couldn't init player experience text box.");
		return false;
	}

	const std::string playerLevelText = CharacterCreationUiModel::getPlayerLevel(game);
	const TextBox::InitInfo playerLevelTextBoxInitInfo = CharacterSheetUiView::getPlayerLevelTextBoxInitInfo(playerLevelText, fontLibrary);
	if (!this->levelTextBox.init(playerLevelTextBoxInitInfo, playerLevelText, renderer))
	{
		DebugLogError("Couldn't init player level text box.");
		return false;
	}

	this->doneButton = Button<Game&, int, bool*>(
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
		[this, &game]() { this->doneButton.click(game, this->bonusPoints, &this->attributesAreSaved); });
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

	auto &textureManager = game.textureManager;
	const UiTextureID bodyTextureID = ChooseAttributesUiView::allocBodyTexture(game);
	const UiTextureID pantsTextureID = ChooseAttributesUiView::allocPantsTexture(game);
	const UiTextureID shirtTextureID = ChooseAttributesUiView::allocShirtTexture(game);
	const UiTextureID statsBgTextureID = ChooseAttributesUiView::allocStatsBgTexture(textureManager, renderer);
	this->bodyTextureRef.init(bodyTextureID, renderer);
	this->pantsTextureRef.init(pantsTextureID, renderer);
	this->shirtTextureRef.init(shirtTextureID, renderer);
	this->statsBgTextureRef.init(statsBgTextureID, renderer);

	const UiTextureID upDownTextureID = ChooseAttributesUiView::allocUpDownButtonTexture(textureManager, renderer);
	this->upDownTextureRef.init(upDownTextureID, renderer);

	const UiTextureID bonusPointsTextureID = ChooseAttributesUiView::allocBonusPointsTexture(textureManager, renderer);
	this->bonusPointsTextureRef.init(bonusPointsTextureID, renderer);

	const Buffer<TextureAsset> headTextureAssets = ChooseAttributesUiView::getHeadTextureAssets(game);
	this->headTextureRefs.init(headTextureAssets.getCount());
	for (int i = 0; i < headTextureAssets.getCount(); i++)
	{
		const TextureAsset &headTextureAsset = headTextureAssets.get(i);
		const UiTextureID headTextureID = ChooseAttributesUiView::allocHeadTexture(headTextureAsset, textureManager, renderer);
		this->headTextureRefs.set(i, ScopedUiTextureRef(headTextureID, renderer));
	}

	const Int2 bodyTextureDims = *renderer.tryGetUiTextureDims(bodyTextureID);
	const Int2 pantsTextureDims = *renderer.tryGetUiTextureDims(pantsTextureID);
	const Int2 shirtTextureDims = *renderer.tryGetUiTextureDims(shirtTextureID);
	const Int2 statsBgTextureDims = *renderer.tryGetUiTextureDims(statsBgTextureID);

	UiDrawCall::TextureFunc headTextureFunc = [this, &game]()
	{
		const auto &charCreationState = game.getCharacterCreationState();
		const int portraitIndex = charCreationState.portraitIndex;
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
		const int portraitIndex = charCreationState.portraitIndex;
		const ScopedUiTextureRef &headTextureRef = this->headTextureRefs.get(portraitIndex);
		return Int2(headTextureRef.getWidth(), headTextureRef.getHeight());
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
		UiDrawCall::makePivotFunc(PivotType::TopLeft),
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

	const Int2 bonusPointsTextureTopLeftPosition = ChooseAttributesUiView::BonusPointsTextureTopLeftPosition;
	const Int2 bonusPointsTextureDims = *renderer.tryGetUiTextureDims(bonusPointsTextureID);
	this->addDrawCall(
		bonusPointsTextureID,
		bonusPointsTextureTopLeftPosition,
		bonusPointsTextureDims,
		PivotType::TopLeft);

	UiDrawCall::PositionFunc upDownArrowPositionFunc = [this]()
	{
		const Rect &selectedAttributeTextBoxRect = this->attributeTextBoxes[this->selectedAttributeIndex].getRect();
		return Int2(
			ChooseAttributesUiView::UpDownButtonFirstTopLeftPosition.x + (this->upDownTextureRef.getWidth() / 2),
			selectedAttributeTextBoxRect.getCenter().y);
	};

	this->addDrawCall(
		UiDrawCall::makeTextureFunc(this->upDownTextureRef.get()),
		upDownArrowPositionFunc,
		UiDrawCall::makeSizeFunc(*renderer.tryGetUiTextureDims(this->upDownTextureRef.get())),
		UiDrawCall::makePivotFunc(PivotType::Middle),
		UiDrawCall::defaultActiveFunc);

	const Rect &nameTextBoxRect = this->nameTextBox.getRect();
	this->addDrawCall(
		this->nameTextBox.getTextureID(),
		nameTextBoxRect.getTopLeft(),
		nameTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &raceTextBoxRect = this->raceTextBox.getRect();
	this->addDrawCall(
		this->raceTextBox.getTextureID(),
		raceTextBoxRect.getTopLeft(),
		raceTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &classTextBoxRect = this->classTextBox.getRect();
	this->addDrawCall(
		this->classTextBox.getTextureID(),
		classTextBoxRect.getTopLeft(),
		classTextBoxRect.getSize(),
		PivotType::TopLeft);

	// code arrow
	for (int attributeIndex = 0; attributeIndex < PrimaryAttributes::COUNT; attributeIndex++)
	{
		const Rect attributeFirstButtonRect = ChooseAttributesUiView::AttributeButtonFirstRect;
		const Rect attributeButtonRect(
			attributeFirstButtonRect.getLeft(),
			attributeFirstButtonRect.getTop() + (attributeFirstButtonRect.height * attributeIndex),
			attributeFirstButtonRect.width,
			attributeFirstButtonRect.height);

		this->addButtonProxy(MouseButtonType::Left, attributeButtonRect,
			[this, attributeIndex]() { this->selectedAttributeIndex = attributeIndex; },
			Rect(),
			[this]() { return !this->attributesAreSaved; });

		const Int2 upDownButtonFirstTopLeftPos = ChooseAttributesUiView::UpDownButtonFirstTopLeftPosition;
		const Rect &attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();

		Button<> &upButton = this->increasePointButtons[attributeIndex];
		upButton = Button<>();
		upButton.setX(upDownButtonFirstTopLeftPos.x);
		upButton.setY(attributeTextBoxRect.getCenter().y - (this->upDownTextureRef.getHeight() / 2));
		upButton.setWidth(this->upDownTextureRef.getWidth());
		upButton.setHeight(this->upDownTextureRef.getHeight() / 2);

		Button<> &downButton = this->decreasePointButtons[attributeIndex];
		downButton = Button<>();
		downButton.setX(upDownButtonFirstTopLeftPos.x);
		downButton.setY(attributeTextBoxRect.getCenter().y);
		downButton.setWidth(this->upDownTextureRef.getWidth());
		downButton.setHeight(this->upDownTextureRef.getHeight() / 2);

		// Click handler for up arrow
		this->addButtonProxy(MouseButtonType::Left, upButton.getRect(),
			[this, attributeIndex]()
		{
			Game &game = this->getGame();
			CharacterCreationState &charCreationState = game.getCharacterCreationState();
			int *changedPoints = std::begin(charCreationState.changedPoints);
			changedPoints[attributeIndex]++;
			this->bonusPoints--;

			PrimaryAttributes &attributes = charCreationState.attributes;
			BufferView<PrimaryAttribute> attributesView = attributes.getAttributes();
			PrimaryAttribute &attribute = attributesView.get(attributeIndex);
			attribute.maxValue += 1;
			if (strcmp(attribute.name, "Agility") == 0) {
				this->bonusToHitValue = ArenaPlayerUtils::calculateBonusToHit(attribute.maxValue);
				this->bonusToHitTextBox.setText(std::to_string(this->bonusToHitValue));
				this->bonusToDefendTextBox.setText(std::to_string(this->bonusToHitValue));
			}
			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			attributeTextBox.setText(std::to_string(attribute.maxValue));
			this->bonusPointsTextBox.setText(std::to_string(this->bonusPoints));
		},
			Rect(),
			[this, attributeIndex]() { return !this->attributesAreSaved && (attributeIndex == this->selectedAttributeIndex) && this->bonusPoints > 0; });

		// Click handler for down arrow
		this->addButtonProxy(MouseButtonType::Left, downButton.getRect(),
			[this, attributeIndex]()
		{
			Game &game = this->getGame();
			CharacterCreationState &charCreationState = game.getCharacterCreationState();
			int *changedPoints = std::begin(charCreationState.changedPoints);
			changedPoints[attributeIndex]--;
			this->bonusPoints++;

			PrimaryAttributes &attributes = charCreationState.attributes;
			BufferView<PrimaryAttribute> attributesView = attributes.getAttributes();
			PrimaryAttribute &attribute = attributesView.get(attributeIndex);
			attribute.maxValue -= 1;

			if (strcmp(attribute.name, "Agility") == 0) {
				this->bonusToHitValue = ArenaPlayerUtils::calculateBonusToHit(attribute.maxValue);
				this->bonusToHitTextBox.setText(std::to_string(this->bonusToHitValue));
				this->bonusToDefendTextBox.setText(std::to_string(this->bonusToHitValue));
			}

			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			attributeTextBox.setText(std::to_string(attribute.maxValue));
			this->bonusPointsTextBox.setText(std::to_string(this->bonusPoints));
		},
			Rect(),
			[this, attributeIndex]()
		{
			Game &game = this->getGame();
			CharacterCreationState &charCreationState = game.getCharacterCreationState();
			const int *changedPoints = std::begin(charCreationState.changedPoints);
			return !this->attributesAreSaved && (attributeIndex == this->selectedAttributeIndex) && changedPoints[attributeIndex] > 0;
		});
	}
	constexpr Int2 bonusPointsTextBoxTopLeftPosition = ChooseAttributesUiView::BonusPointsTextBoxTopLeftPosition;
	const TextBox::InitInfo bonusPointsTextBoxInitInfo = TextBox::InitInfo::makeWithXY(
		std::string(3, TextRenderUtils::LARGEST_CHAR),
		bonusPointsTextBoxTopLeftPosition.x,
		bonusPointsTextBoxTopLeftPosition.y,
		ChooseAttributesUiView::BonusPointsFontName,
		ChooseAttributesUiView::BonusPointsTextColor,
		TextAlignment::TopLeft,
		std::nullopt,
		1,
		fontLibrary);

	if (!this->bonusPointsTextBox.init(bonusPointsTextBoxInitInfo, std::to_string(bonusPoints), renderer))
	{
		DebugLogError("Couldn't init bonus points text box.");
		return false;
	}

	const Rect &bonusPointsTextBoxRect = this->bonusPointsTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->bonusPointsTextBox.getTextureID(); },
		UiDrawCall::makePositionFunc(bonusPointsTextBoxRect.getTopLeft()),
		UiDrawCall::makeSizeFunc(bonusPointsTextBoxRect.getSize()),
		UiDrawCall::makePivotFunc(PivotType::TopLeft),
		UiDrawCall::defaultActiveFunc);

	// my code bonus atribute stat.............................................................................
	

	const Rect& lastAttributeTextBoxRect = this->attributeTextBoxes[PrimaryAttributes::COUNT - 6].getRect();
	const Int2 bonusToHitTextBoxTopLeftPosition(
		lastAttributeTextBoxRect.getLeft() + 60,
		lastAttributeTextBoxRect.getTop() + 8);
	const TextBox::InitInfo bonusToHitTextBoxInitInfo = TextBox::InitInfo::makeWithXY(
		std::string(3, TextRenderUtils::LARGEST_CHAR),
		bonusToHitTextBoxTopLeftPosition.x,
		bonusToHitTextBoxTopLeftPosition.y,
		ChooseAttributesUiView::BonusPointsFontName,
		ChooseAttributesUiView::BonusPointsTextColor,
		TextAlignment::TopLeft,
		std::nullopt,
		1,
		fontLibrary);

	if (!this->bonusToHitTextBox.init(bonusToHitTextBoxInitInfo,std::to_string(this->bonusToHitValue), renderer))
	{
		DebugLogError("Couldn't init bonus to hit text box.");
		return false;
	}

	const Rect &bonusToHitTextBoxRect = this->bonusToHitTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->bonusToHitTextBox.getTextureID(); },
		UiDrawCall::makePositionFunc(bonusToHitTextBoxRect.getTopLeft()),
		UiDrawCall::makeSizeFunc(bonusToHitTextBoxRect.getSize()),
		UiDrawCall::makePivotFunc(PivotType::TopLeft),
		UiDrawCall::defaultActiveFunc);

	// codigo de inicializacion de bonusToDefendTextBox
	const Int2 bonusToDefendTextBoxTopLeftPosition(
		lastAttributeTextBoxRect.getLeft() + 120,
		lastAttributeTextBoxRect.getTop() + 8);
	const TextBox::InitInfo bonusToDefendTextBoxInitInfo = TextBox::InitInfo::makeWithXY(
		std::string(3, TextRenderUtils::LARGEST_CHAR),
		bonusToDefendTextBoxTopLeftPosition.x,
		bonusToDefendTextBoxTopLeftPosition.y,
		ChooseAttributesUiView::BonusPointsFontName,
		ChooseAttributesUiView::BonusPointsTextColor,
		TextAlignment::TopLeft,
		std::nullopt,
		1,
		fontLibrary);
	if (!this->bonusToDefendTextBox.init(bonusToDefendTextBoxInitInfo, std::to_string(this->bonusToHitValue), renderer))
	{
		DebugLogError("Couldn't init bonus to defend text box.");
		return false;
	}

	const Rect &bonusToDefendTextBoxRect = this->bonusToDefendTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->bonusToDefendTextBox.getTextureID(); },
		UiDrawCall::makePositionFunc(bonusToDefendTextBoxRect.getTopLeft()),
		UiDrawCall::makeSizeFunc(bonusToDefendTextBoxRect.getSize()),
		UiDrawCall::makePivotFunc(PivotType::TopLeft),
		UiDrawCall::defaultActiveFunc);

	//  TextBox de Carisma
	const Int2 bonusToCharismaTextBoxTopLeftPosition(
		lastAttributeTextBoxRect.getLeft() + 60,
		lastAttributeTextBoxRect.getTop() + 32);
	const TextBox::InitInfo bonusToCharismaTextBoxInitInfo = TextBox::InitInfo::makeWithXY(
		std::string(3, TextRenderUtils::LARGEST_CHAR),
		bonusToCharismaTextBoxTopLeftPosition.x,
		bonusToCharismaTextBoxTopLeftPosition.y,
		ChooseAttributesUiView::BonusPointsFontName,
		ChooseAttributesUiView::BonusPointsTextColor,
		TextAlignment::TopLeft,
		std::nullopt,
		1,
		fontLibrary);

	if (!this->bonusToCharismaTextBox.init(bonusToCharismaTextBoxInitInfo, "400", renderer))
	{
		DebugLogError("Couldn't init bonus to charisma text box.");
		return false;
	}

	const Rect &bonusToCharismaTextBoxRect = this->bonusToCharismaTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->bonusToCharismaTextBox.getTextureID(); },
		UiDrawCall::makePositionFunc(bonusToCharismaTextBoxRect.getTopLeft()),
		UiDrawCall::makeSizeFunc(bonusToCharismaTextBoxRect.getSize()),
		UiDrawCall::makePivotFunc(PivotType::TopLeft),
		UiDrawCall::defaultActiveFunc);
	/............................................................................................................................................
	//My code
	for (int attributeIndex = 0; attributeIndex < PrimaryAttributes::COUNT; attributeIndex++)
	{
		UiDrawCall::TextureFunc attributeTextBoxTextureFunc = [this, attributeIndex]()
		{
			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			return attributeTextBox.getTextureID();
		};

		const Rect &attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();
		this->addDrawCall(
			attributeTextBoxTextureFunc,
			UiDrawCall::makePositionFunc(attributeTextBoxRect.getTopLeft()),
			UiDrawCall::makeSizeFunc(attributeTextBoxRect.getSize()),
			UiDrawCall::makePivotFunc(PivotType::TopLeft),
			UiDrawCall::defaultActiveFunc);
	}

	const Rect &playerExperienceTextBoxRect = this->experienceTextBox.getRect();
	this->addDrawCall(
		this->experienceTextBox.getTextureID(),
		playerExperienceTextBoxRect.getTopLeft(),
		playerExperienceTextBoxRect.getSize(),
		PivotType::TopLeft);

	const Rect &playerLevelTextBoxRect = this->levelTextBox.getRect();
	this->addDrawCall(
		this->levelTextBox.getTextureID(),
		playerLevelTextBoxRect.getTopLeft(),
		playerLevelTextBoxRect.getSize(),
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

	const Surface initialPopUpSurface = TextureUtils::generate(
		ChooseAttributesUiView::InitialTextPatternType,
		ChooseAttributesUiView::getDistributePointsTextBoxTextureWidth(initialPopUpTextBoxInitInfo.rect.width),
		ChooseAttributesUiView::getDistributePointsTextBoxTextureHeight(initialPopUpTextBoxInitInfo.rect.height),
		textureManager,
		renderer);

	UiTextureID initialPopUpTextureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(initialPopUpSurface, textureManager, renderer, &initialPopUpTextureID))
	{
		DebugCrash("Couldn't create initial pop-up texture.");
	}

	ScopedUiTextureRef initialPopUpTextureRef(initialPopUpTextureID, renderer);
	game.pushSubPanel<TextSubPanel>(initialPopUpTextBoxInitInfo, initialPopUpText,
		ChooseAttributesUiController::onInitialPopUpSelected, std::move(initialPopUpTextureRef),
		ChooseAttributesUiView::InitialTextureCenterPoint);

	return true;
}
