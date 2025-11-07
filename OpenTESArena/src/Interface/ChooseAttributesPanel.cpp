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

#include "components/utilities/Span.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game)
{
	this->selectedAttributeIndex = 0;
	this->attributesAreSaved = false;
}

void ChooseAttributesPanel::populateBaseAttributesRandomly(CharacterCreationState &charCreationState, ArenaRandom &random)
{
	charCreationState.populateBaseAttributes();

	Span<PrimaryAttribute> attributes = charCreationState.attributes.getView();
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

	Random &random = game.random;
	const PrimaryAttributes &primaryAttributes = charCreationState.attributes;
	charCreationState.derivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(primaryAttributes);
	charCreationState.maxHealth = ArenaPlayerUtils::calculateMaxHealthPoints(charCreationState.classDefID, random);
	charCreationState.maxStamina = ArenaPlayerUtils::calculateMaxStamina(primaryAttributes.strength.maxValue, primaryAttributes.endurance.maxValue);
	charCreationState.maxSpellPoints = ArenaPlayerUtils::calculateMaxSpellPoints(charCreationState.classDefID, primaryAttributes.intelligence.maxValue);
	charCreationState.gold = ArenaPlayerUtils::calculateStartingGold(random);
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
	const Span<const PrimaryAttribute> playerAttributesView = primaryAttributes.getView();
	for (int i = 0; i < playerAttributesView.getCount(); i++)
	{
		const PrimaryAttribute &attribute = playerAttributesView[i];
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
	Span<const int> playerDerivedAttributesView = charCreationState.derivedAttributes.getView();
	for (int i = 0; i < playerDerivedAttributesView.getCount(); i++)
	{
		const int derivedAttributeValue = playerDerivedAttributesView[i];
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

	const TextBoxInitInfo playerGoldTextBoxInitInfo = CharacterSheetUiView::getPlayerGoldTextBoxInitInfo(fontLibrary);
	const std::string playerGoldText = ChooseAttributesUiModel::getPlayerGold(game);
	if (!this->goldTextBox.init(playerGoldTextBoxInitInfo, playerGoldText, renderer))
	{
		DebugLogError("Couldn't init player gold text box.");
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

	TextureManager &textureManager = game.textureManager;
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

	UiDrawCallInitInfo bodyDrawCallInitInfo;
	bodyDrawCallInitInfo.textureID = bodyTextureID;
	bodyDrawCallInitInfo.position = ChooseAttributesUiView::getBodyOffset(game);
	bodyDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(bodyTextureID);
	this->addDrawCall(bodyDrawCallInitInfo);

	UiDrawCallInitInfo pantsDrawCallInitInfo;
	pantsDrawCallInitInfo.textureID = pantsTextureID;
	pantsDrawCallInitInfo.position = ChooseAttributesUiView::getPantsOffset(game);
	pantsDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(pantsTextureID);
	this->addDrawCall(pantsDrawCallInitInfo);

	UiDrawCallInitInfo headDrawCallInitInfo;
	headDrawCallInitInfo.textureFunc = [this, &game]()
	{
		const auto &charCreationState = game.getCharacterCreationState();
		const int portraitIndex = charCreationState.portraitIndex;
		const ScopedUiTextureRef &headTextureRef = this->headTextureRefs.get(portraitIndex);
		return headTextureRef.get();
	};

	headDrawCallInitInfo.positionFunc = [&game]() { return ChooseAttributesUiView::getHeadOffset(game); };
	headDrawCallInitInfo.sizeFunc = [this, &game]()
	{
		const auto &charCreationState = game.getCharacterCreationState();
		const int portraitIndex = charCreationState.portraitIndex;
		const ScopedUiTextureRef &headTextureRef = this->headTextureRefs.get(portraitIndex);
		return headTextureRef.getDimensions();
	};

	this->addDrawCall(headDrawCallInitInfo);

	UiDrawCallInitInfo shirtDrawCallInitInfo;
	shirtDrawCallInitInfo.textureID = shirtTextureID;
	shirtDrawCallInitInfo.position = ChooseAttributesUiView::getShirtOffset(game);
	shirtDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(shirtTextureID);
	this->addDrawCall(shirtDrawCallInitInfo);

	UiDrawCallInitInfo statsBgDrawCallInitInfo;
	statsBgDrawCallInitInfo.textureID = statsBgTextureID;
	statsBgDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(statsBgTextureID);
	this->addDrawCall(statsBgDrawCallInitInfo);

	UiDrawCallInitInfo bonusPointsTextureDrawCallInitInfo;
	bonusPointsTextureDrawCallInitInfo.textureID = bonusPointsTextureID;
	bonusPointsTextureDrawCallInitInfo.position = ChooseAttributesUiView::BonusPointsTextureTopLeftPosition;
	bonusPointsTextureDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(bonusPointsTextureID);
	this->addDrawCall(bonusPointsTextureDrawCallInitInfo);

	UiDrawCallInitInfo upDownArrowDrawCallInitInfo;
	upDownArrowDrawCallInitInfo.textureID = this->upDownTextureRef.get();
	upDownArrowDrawCallInitInfo.positionFunc = [this]()
	{
		const Rect selectedAttributeTextBoxRect = this->attributeTextBoxes[this->selectedAttributeIndex].getRect();
		const int centerX = ChooseAttributesUiView::UpDownButtonFirstTopLeftPosition.x + (this->upDownTextureRef.getWidth() / 2);
		const int centerY = selectedAttributeTextBoxRect.getCenter().y;
		return Int2(centerX, centerY);
	};

	upDownArrowDrawCallInitInfo.size = *renderer.tryGetUiTextureDims(this->upDownTextureRef.get());
	upDownArrowDrawCallInitInfo.pivotType = UiPivotType::Middle;
	this->addDrawCall(upDownArrowDrawCallInitInfo);

	const Rect playerNameTextBoxRect = this->nameTextBox.getRect();
	UiDrawCallInitInfo playerNameDrawCallInitInfo;
	playerNameDrawCallInitInfo.textureID = this->nameTextBox.getTextureID();
	playerNameDrawCallInitInfo.position = playerNameTextBoxRect.getTopLeft();
	playerNameDrawCallInitInfo.size = playerNameTextBoxRect.getSize();
	this->addDrawCall(playerNameDrawCallInitInfo);

	const Rect playerRaceTextBoxRect = this->raceTextBox.getRect();
	UiDrawCallInitInfo playerRaceDrawCallInitInfo;
	playerRaceDrawCallInitInfo.textureID = this->raceTextBox.getTextureID();
	playerRaceDrawCallInitInfo.position = playerRaceTextBoxRect.getTopLeft();
	playerRaceDrawCallInitInfo.size = playerRaceTextBoxRect.getSize();
	this->addDrawCall(playerRaceDrawCallInitInfo);

	const Rect playerClassTextBoxRect = this->classTextBox.getRect();
	UiDrawCallInitInfo playerClassDrawCallInitInfo;
	playerClassDrawCallInitInfo.textureID = this->classTextBox.getTextureID();
	playerClassDrawCallInitInfo.position = playerClassTextBoxRect.getTopLeft();
	playerClassDrawCallInitInfo.size = playerClassTextBoxRect.getSize();
	this->addDrawCall(playerClassDrawCallInitInfo);

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
			Span<int> changedPoints = charCreationState.changedPoints;
			changedPoints[attributeIndex]++;
			charCreationState.bonusPoints--;

			PrimaryAttributes &attributes = charCreationState.attributes;
			Span<PrimaryAttribute> attributesView = attributes.getView();
			PrimaryAttribute &attribute = attributesView[attributeIndex];
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
			Span<PrimaryAttribute> attributesView = attributes.getView();
			PrimaryAttribute &attribute = attributesView[attributeIndex];
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
			Span<const int> changedPoints = charCreationState.changedPoints;
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

	const Rect bonusPointsTextBoxRect = this->bonusPointsTextBox.getRect();
	UiDrawCallInitInfo bonusPointsTextDrawCallInitInfo;
	bonusPointsTextDrawCallInitInfo.textureFunc = [this]() { return this->bonusPointsTextBox.getTextureID(); };
	bonusPointsTextDrawCallInitInfo.position = bonusPointsTextBoxRect.getTopLeft();
	bonusPointsTextDrawCallInitInfo.size = bonusPointsTextBoxRect.getSize();
	this->addDrawCall(bonusPointsTextDrawCallInitInfo);

	for (int attributeIndex = 0; attributeIndex < PrimaryAttributes::COUNT; attributeIndex++)
	{
		const Rect attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();

		UiDrawCallInitInfo primaryAttributeDrawCallInitInfo;
		primaryAttributeDrawCallInitInfo.textureFunc = [this, attributeIndex]()
		{
			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			return attributeTextBox.getTextureID();
		};

		primaryAttributeDrawCallInitInfo.position = attributeTextBoxRect.getTopLeft();
		primaryAttributeDrawCallInitInfo.size = attributeTextBoxRect.getSize();
		this->addDrawCall(primaryAttributeDrawCallInitInfo);
	}

	for (int derivedAttributeIndex = 0; derivedAttributeIndex < DerivedAttributes::COUNT; derivedAttributeIndex++)
	{
		const Rect derivedAttributeTextBoxRect = this->derivedAttributeTextBoxes[derivedAttributeIndex].getRect();

		UiDrawCallInitInfo derivedAttributeDrawCallInitInfo;
		derivedAttributeDrawCallInitInfo.textureFunc = [this, derivedAttributeIndex]()
		{
			TextBox &derivedAttributeTextBox = this->derivedAttributeTextBoxes[derivedAttributeIndex];
			return derivedAttributeTextBox.getTextureID();
		};

		derivedAttributeDrawCallInitInfo.position = derivedAttributeTextBoxRect.getTopLeft();
		derivedAttributeDrawCallInitInfo.size = derivedAttributeTextBoxRect.getSize();
		this->addDrawCall(derivedAttributeDrawCallInitInfo);
	}

	const Rect playerExperienceTextBoxRect = this->experienceTextBox.getRect();
	UiDrawCallInitInfo experienceDrawCallInitInfo;
	experienceDrawCallInitInfo.textureFunc = [this]() { return this->experienceTextBox.getTextureID(); };
	experienceDrawCallInitInfo.position = playerExperienceTextBoxRect.getTopLeft();
	experienceDrawCallInitInfo.size = playerExperienceTextBoxRect.getSize();
	this->addDrawCall(experienceDrawCallInitInfo);

	const Rect playerLevelTextBoxRect = this->levelTextBox.getRect();
	UiDrawCallInitInfo levelDrawCallInitInfo;
	levelDrawCallInitInfo.textureID = this->levelTextBox.getTextureID();
	levelDrawCallInitInfo.position = playerLevelTextBoxRect.getTopLeft();
	levelDrawCallInitInfo.size = playerLevelTextBoxRect.getSize();
	this->addDrawCall(levelDrawCallInitInfo);

	const Rect playerHealthTextBoxRect = this->healthTextBox.getRect();
	UiDrawCallInitInfo playerHealthDrawCallInitInfo;
	playerHealthDrawCallInitInfo.textureFunc = [this]() { return this->healthTextBox.getTextureID(); };
	playerHealthDrawCallInitInfo.position = playerHealthTextBoxRect.getTopLeft();
	playerHealthDrawCallInitInfo.size = playerHealthTextBoxRect.getSize();
	this->addDrawCall(playerHealthDrawCallInitInfo);

	const Rect playerStaminaTextBoxRect = this->staminaTextBox.getRect();
	UiDrawCallInitInfo playerStaminaDrawCallInitInfo;
	playerStaminaDrawCallInitInfo.textureFunc = [this]() { return this->staminaTextBox.getTextureID(); };
	playerStaminaDrawCallInitInfo.position = playerStaminaTextBoxRect.getTopLeft();
	playerStaminaDrawCallInitInfo.size = playerStaminaTextBoxRect.getSize();
	this->addDrawCall(playerStaminaDrawCallInitInfo);

	const Rect playerSpellPointsTextBoxRect = this->spellPointsTextBox.getRect();
	UiDrawCallInitInfo playerSpellPointsDrawCallInitInfo;
	playerSpellPointsDrawCallInitInfo.textureFunc = [this]() { return this->spellPointsTextBox.getTextureID(); };
	playerSpellPointsDrawCallInitInfo.position = playerSpellPointsTextBoxRect.getTopLeft();
	playerSpellPointsDrawCallInitInfo.size = playerSpellPointsTextBoxRect.getSize();
	this->addDrawCall(playerSpellPointsDrawCallInitInfo);

	const Rect playerGoldTextBoxRect = this->goldTextBox.getRect();
	UiDrawCallInitInfo playerGoldDrawCallInitInfo;
	playerGoldDrawCallInitInfo.textureFunc = [this]() { return this->goldTextBox.getTextureID(); };
	playerGoldDrawCallInitInfo.position = playerGoldTextBoxRect.getTopLeft();
	playerGoldDrawCallInitInfo.size = playerGoldTextBoxRect.getSize();
	this->addDrawCall(playerGoldDrawCallInitInfo);

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
	const PrimaryAttributes &primaryAttributes = charCreationState.attributes;
	charCreationState.maxStamina = ArenaPlayerUtils::calculateMaxStamina(primaryAttributes.strength.maxValue, primaryAttributes.endurance.maxValue);
	charCreationState.maxSpellPoints = ArenaPlayerUtils::calculateMaxSpellPoints(charCreationState.classDefID, primaryAttributes.intelligence.maxValue);
	charCreationState.derivedAttributes = ArenaPlayerUtils::calculateTotalDerivedBonuses(primaryAttributes);

	const Span<const int> derivedAttributesView = charCreationState.derivedAttributes.getView();
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
