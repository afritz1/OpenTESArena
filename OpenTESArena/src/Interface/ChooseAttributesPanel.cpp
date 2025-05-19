#include <algorithm>
#include <cstring>

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "CharacterSheetUiController.h"
#include "CharacterSheetUiModel.h"
#include "CharacterSheetUiView.h"
#include "ChooseAttributesPanel.h"
#include "CommonUiView.h"
#include "TextSubPanel.h"
#include "../Assets/TextureUtils.h"
#include "../Game/Game.h"
#include "../Input/InputActionName.h"
#include "../Math/Random.h"
#include "../Player/ArenaPlayerUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextRenderUtils.h"

#include "components/utilities/BufferView.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game)
{
	this->selectedAttributeIndex = 0;
}

void ChooseAttributesPanel::populateBaseAttributesRandomly(CharacterCreationState &charCreationState, ArenaRandom &random)
{
	charCreationState.populateBaseAttributes();

	BufferView<PrimaryAttribute> attributes = charCreationState.attributes.getView();
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

	const PrimaryAttributes &primaryAttributes = charCreationState.attributes;
	charCreationState.derivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(primaryAttributes);
	charCreationState.maxHealth = ArenaPlayerUtils::calculateMaxHealthPoints(charCreationState.classDefID, game.random);
	charCreationState.maxStamina = ArenaPlayerUtils::calculateMaxStamina(primaryAttributes.strength.maxValue, primaryAttributes.endurance.maxValue);
	charCreationState.maxSpellPoints = ArenaPlayerUtils::calculateMaxSpellPoints(charCreationState.classDefID, primaryAttributes.intelligence.maxValue);
	charCreationState.bonusPoints = ChooseAttributesUiModel::rollClassic(ChooseAttributesUiModel::BonusPointsRandomMax, arenaRandom);

	this->selectedAttributeIndex = 0;
	this->attributesAreSaved = false;

	const TextBoxInitInfo playerNameTextBoxInitInfo = CharacterSheetUiView::getPlayerNameTextBoxInitInfo(fontLibrary);
	const std::string playerNameText = CharacterCreationUiModel::getPlayerName(game);
	if (!this->nameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const TextBoxInitInfo playerRaceTextBoxInitInfo = CharacterSheetUiView::getPlayerRaceTextBoxInitInfo(fontLibrary);
	const std::string playerRaceText = CharacterCreationUiModel::getPlayerRaceName(game);
	if (!this->raceTextBox.init(playerRaceTextBoxInitInfo, playerRaceText, renderer))
	{
		DebugLogError("Couldn't init player race text box.");
		return false;
	}

	const TextBoxInitInfo playerClassTextBoxInitInfo = CharacterSheetUiView::getPlayerClassTextBoxInitInfo(fontLibrary);
	const std::string playerClassText = CharacterCreationUiModel::getPlayerClassName(game);
	if (!this->classTextBox.init(playerClassTextBoxInitInfo, playerClassText, renderer))
	{
		DebugLogError("Couldn't init player class text box.");
		return false;
	}

	const Buffer<TextBoxInitInfo> playerAttributesTextBoxInitInfos = CharacterSheetUiView::getPlayerAttributeTextBoxInitInfos(fontLibrary);
	const BufferView<const PrimaryAttribute> playerAttributesView = primaryAttributes.getView();
	for (int i = 0; i < playerAttributesView.getCount(); i++)
	{
		const PrimaryAttribute &attribute = playerAttributesView.get(i);
		const int attributeValue = attribute.maxValue;
		const std::string attributeValueText = std::to_string(attributeValue);
		const TextBoxInitInfo &attributeTextBoxInitInfo = playerAttributesTextBoxInitInfos[i];
		if (!this->attributeTextBoxes[i].init(attributeTextBoxInitInfo, attributeValueText, renderer))
		{
			DebugLogError("Couldn't init player attribute " + std::string(attribute.name) + " text box.");
			return false;
		}
	}

	const Buffer<TextBoxInitInfo> playerDerivedAttributesTextBoxInitInfos = CharacterSheetUiView::getPlayerDerivedAttributeTextBoxInitInfos(fontLibrary);
	BufferView<const int> playerDerivedAttributesView = charCreationState.derivedAttributes.getView();
	for (int i = 0; i < playerDerivedAttributesView.getCount(); i++)
	{
		const int derivedAttributeValue = playerDerivedAttributesView.get(i);
		const std::string derivedAttributeValueText = DerivedAttributes::isModifier(i) ?
			CharacterSheetUiModel::getDerivedAttributeDisplayString(derivedAttributeValue) : std::to_string(derivedAttributeValue);
		const TextBoxInitInfo &derivedAttributeTextBoxInitInfo = playerDerivedAttributesTextBoxInitInfos[i];
		if (!this->derivedAttributeTextBoxes[i].init(derivedAttributeTextBoxInitInfo, derivedAttributeValueText, renderer))
		{
			DebugLogErrorFormat("Couldn't init derived player attribute %d text box.", i);
			return false;
		}
	}

	const TextBoxInitInfo playerExperienceTextBoxInitInfo = CharacterSheetUiView::getPlayerExperienceTextBoxInitInfo(fontLibrary);
	const std::string playerExperienceText = CharacterCreationUiModel::getPlayerExperience(game);
	if (!this->experienceTextBox.init(playerExperienceTextBoxInitInfo, playerExperienceText, renderer))
	{
		DebugLogError("Couldn't init player experience text box.");
		return false;
	}

	const TextBoxInitInfo playerLevelTextBoxInitInfo = CharacterSheetUiView::getPlayerLevelTextBoxInitInfo(fontLibrary);
	const std::string playerLevelText = CharacterCreationUiModel::getPlayerLevel(game);
	if (!this->levelTextBox.init(playerLevelTextBoxInitInfo, playerLevelText, renderer))
	{
		DebugLogError("Couldn't init player level text box.");
		return false;
	}

	const TextBoxInitInfo playerHealthTextBoxInitInfo = CharacterSheetUiView::getPlayerHealthTextBoxInitInfo(fontLibrary);
	const std::string playerHealthText = ChooseAttributesUiModel::getPlayerHealth(game);
	if (!this->healthTextBox.init(playerHealthTextBoxInitInfo, playerHealthText, renderer))
	{
		DebugLogError("Couldn't init player health text box.");
		return false;
	}

	const TextBoxInitInfo playerStaminaTextBoxInitInfo = CharacterSheetUiView::getPlayerStaminaTextBoxInitInfo(fontLibrary);
	const std::string playerStaminaText = ChooseAttributesUiModel::getPlayerStamina(game);
	if (!this->staminaTextBox.init(playerStaminaTextBoxInitInfo, playerStaminaText, renderer))
	{
		DebugLogError("Couldn't init player stamina text box.");
		return false;
	}

	const TextBoxInitInfo playerSpellPointsTextBoxInitInfo = CharacterSheetUiView::getPlayerSpellPointsTextBoxInitInfo(fontLibrary);
	const std::string playerSpellPointsText = ChooseAttributesUiModel::getPlayerSpellPoints(game);
	if (!this->spellPointsTextBox.init(playerSpellPointsTextBoxInitInfo, playerSpellPointsText, renderer))
	{
		DebugLogError("Couldn't init player spell points text box.");
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
		[this, &game]()
	{
		const CharacterCreationState &charCreationState = game.getCharacterCreationState();
		this->doneButton.click(game, charCreationState.bonusPoints, &this->attributesAreSaved);
	});

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
			BufferView<int> changedPoints = charCreationState.changedPoints;
			changedPoints[attributeIndex]++;
			charCreationState.bonusPoints--;

			PrimaryAttributes &attributes = charCreationState.attributes;
			BufferView<PrimaryAttribute> attributesView = attributes.getView();
			PrimaryAttribute &attribute = attributesView.get(attributeIndex);
			attribute.maxValue += 1;

			this->updateDerivedAttributeValues();

			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			attributeTextBox.setText(std::to_string(attribute.maxValue));
			this->bonusPointsTextBox.setText(std::to_string(charCreationState.bonusPoints));
		},
			Rect(),
			[this, &game, attributeIndex]()
		{
			const CharacterCreationState &charCreationState = game.getCharacterCreationState();
			return !this->attributesAreSaved && (attributeIndex == this->selectedAttributeIndex) && charCreationState.bonusPoints > 0;
		});

		// Click handler for down arrow
		this->addButtonProxy(MouseButtonType::Left, downButton.getRect(),
			[this, attributeIndex]()
		{
			Game &game = this->getGame();
			CharacterCreationState &charCreationState = game.getCharacterCreationState();
			int *changedPoints = std::begin(charCreationState.changedPoints);
			changedPoints[attributeIndex]--;
			charCreationState.bonusPoints++;

			PrimaryAttributes &attributes = charCreationState.attributes;
			BufferView<PrimaryAttribute> attributesView = attributes.getView();
			PrimaryAttribute &attribute = attributesView.get(attributeIndex);
			attribute.maxValue -= 1;

			this->updateDerivedAttributeValues();

			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			attributeTextBox.setText(std::to_string(attribute.maxValue));
			this->bonusPointsTextBox.setText(std::to_string(charCreationState.bonusPoints));
		},
			Rect(),
			[this, attributeIndex]()
		{
			Game &game = this->getGame();
			CharacterCreationState &charCreationState = game.getCharacterCreationState();
			BufferView<const int> changedPoints = charCreationState.changedPoints;
			return !this->attributesAreSaved && (attributeIndex == this->selectedAttributeIndex) && changedPoints[attributeIndex] > 0;
		});
	}
	constexpr Int2 bonusPointsTextBoxTopLeftPosition = ChooseAttributesUiView::BonusPointsTextBoxTopLeftPosition;
	const TextBoxInitInfo bonusPointsTextBoxInitInfo = TextBoxInitInfo::makeWithXY(
		std::string(3, TextRenderUtils::LARGEST_CHAR),
		bonusPointsTextBoxTopLeftPosition.x,
		bonusPointsTextBoxTopLeftPosition.y,
		ChooseAttributesUiView::BonusPointsFontName,
		ChooseAttributesUiView::BonusPointsTextColor,
		TextAlignment::TopLeft,
		std::nullopt,
		1,
		fontLibrary);

	if (!this->bonusPointsTextBox.init(bonusPointsTextBoxInitInfo, std::to_string(charCreationState.bonusPoints), renderer))
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

	for (int derivedAttributeIndex = 0; derivedAttributeIndex < DerivedAttributes::COUNT; derivedAttributeIndex++)
	{
		UiDrawCall::TextureFunc derivedAttributeTextBoxTextureFunc = [this, derivedAttributeIndex]()
		{
			TextBox &derivedAttributeTextBox = this->derivedAttributeTextBoxes[derivedAttributeIndex];
			return derivedAttributeTextBox.getTextureID();
		};

		const Rect &derivedAttributeTextBoxRect = this->derivedAttributeTextBoxes[derivedAttributeIndex].getRect();
		this->addDrawCall(
			derivedAttributeTextBoxTextureFunc,
			UiDrawCall::makePositionFunc(derivedAttributeTextBoxRect.getTopLeft()),
			UiDrawCall::makeSizeFunc(derivedAttributeTextBoxRect.getSize()),
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

	const Rect &playerHealthTextBoxRect = this->healthTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->healthTextBox.getTextureID(); },
		UiDrawCall::makePositionFunc(playerHealthTextBoxRect.getTopLeft()),
		UiDrawCall::makeSizeFunc(playerHealthTextBoxRect.getSize()),
		UiDrawCall::makePivotFunc(PivotType::TopLeft),
		UiDrawCall::defaultActiveFunc);

	const Rect &playerStaminaTextBoxRect = this->staminaTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->staminaTextBox.getTextureID(); },
		UiDrawCall::makePositionFunc(playerStaminaTextBoxRect.getTopLeft()),
		UiDrawCall::makeSizeFunc(playerStaminaTextBoxRect.getSize()),
		UiDrawCall::makePivotFunc(PivotType::TopLeft),
		UiDrawCall::defaultActiveFunc);

	const Rect &playerSpellPointsTextBoxRect = this->spellPointsTextBox.getRect();
	this->addDrawCall(
		[this]() { return this->spellPointsTextBox.getTextureID(); },
		UiDrawCall::makePositionFunc(playerSpellPointsTextBoxRect.getTopLeft()),
		UiDrawCall::makeSizeFunc(playerSpellPointsTextBoxRect.getSize()),
		UiDrawCall::makePivotFunc(PivotType::TopLeft),
		UiDrawCall::defaultActiveFunc);

	const UiTextureID cursorTextureID = CommonUiView::allocDefaultCursorTexture(textureManager, renderer);
	this->cursorTextureRef.init(cursorTextureID, renderer);
	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);

	// Push the initial text pop-up onto the sub-panel stack.
	const std::string initialPopUpText = ChooseAttributesUiModel::getInitialText(game);
	const TextBoxInitInfo initialPopUpTextBoxInitInfo = TextBoxInitInfo::makeWithCenter(
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

void ChooseAttributesPanel::updateDerivedAttributeValues()
{
	Game &game = this->getGame();
	CharacterCreationState &charCreationState = game.getCharacterCreationState();
	charCreationState.derivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(charCreationState.attributes);

	const BufferView<const int> derivedAttributesView = charCreationState.derivedAttributes.getView();
	for (int i = 0; i < DerivedAttributes::COUNT; i++)
	{
		const int derivedAttributeValue = derivedAttributesView[i];
		const std::string derivedAttributeDisplayString = DerivedAttributes::isModifier(i) ?
			CharacterSheetUiModel::getDerivedAttributeDisplayString(derivedAttributeValue) : std::to_string(derivedAttributeValue);
		this->derivedAttributeTextBoxes[i].setText(derivedAttributeDisplayString);
	}

	this->healthTextBox.setText(ChooseAttributesUiModel::getPlayerHealth(game));
	this->staminaTextBox.setText(ChooseAttributesUiModel::getPlayerStamina(game));
	this->spellPointsTextBox.setText(ChooseAttributesUiModel::getPlayerSpellPoints(game));
}
