#include <algorithm>

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
#include "../Math/Random.h"
#include "../Stats/PrimaryAttribute.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextRenderUtils.h"
#include "../UI/TextAlignment.h"
#include "../UI/ArenaFontName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/TextureUtils.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game)
{
	this->bonusPoints = 0;
	this->selectedAttributeIndex = 0;
	this->lastSelectedAttributeIndex = 0;
	this->arrowDrawCallIndex = -1;
}

int ChooseAttributesPanel::calculateInitialBonusPoints(Random &random) const
{
	int totalPoints = 0;
	for (int i = 0; i < 4; i++)
	{
		totalPoints += 1 + random.next(6);
	}

	return totalPoints;
}

bool ChooseAttributesPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.portraitIndex = 0;
	charCreationState.clearChangedPoints();

	this->bonusPoints = this->calculateInitialBonusPoints(game.random);
	this->selectedAttributeIndex = 0;
	this->lastSelectedAttributeIndex = 0;

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
	const Int2 upDownTextureDims = *renderer.tryGetUiTextureDims(upDownTextureID);

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

	// code arrow
	for (int attributeIndex = 0; attributeIndex < PrimaryAttributes::COUNT; attributeIndex++)
	{
		const Rect &attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();

		this->addButtonProxy(MouseButtonType::Left, attributeTextBoxRect,
			[this, attributeIndex]()
		{
			this->selectedAttributeIndex = attributeIndex;
		});

		const Int2 upDownButtonFirstTopLeftPos = ChooseAttributesUiView::UpDownButtonFirstTopLeftPosition;

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

			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			attributeTextBox.setText(std::to_string(attribute.maxValue));

			const Rect &attributeTextBoxRect = attributeTextBox.getRect();
			this->addDrawCall(
				attributeTextBox.getTextureID(),
				attributeTextBoxRect.getTopLeft(),
				Int2(attributeTextBoxRect.getWidth(), attributeTextBoxRect.getHeight()),
				PivotType::TopLeft);

			this->bonusPointsTextBox.setText(std::to_string(this->bonusPoints));
			const Rect &bonusTextBoxRect = this->bonusPointsTextBox.getRect();
			this->addDrawCall(
				this->bonusPointsTextBox.getTextureID(),
				bonusTextBoxRect.getTopLeft(),
				Int2(bonusTextBoxRect.getWidth(), bonusTextBoxRect.getHeight()),
				PivotType::TopLeft);

			DebugLogFormat("%s increased by %d, %d bonus points remaining.", attribute.name, attribute.maxValue, this->bonusPoints);
		},
			Rect(),
			[this, attributeIndex]() { return attributeIndex == this->selectedAttributeIndex && this->bonusPoints > 0; });

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

			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			attributeTextBox.setText(std::to_string(attribute.maxValue));

			const Rect &attributeTextBoxRect = attributeTextBox.getRect();
			this->addDrawCall(
				attributeTextBox.getTextureID(),
				attributeTextBoxRect.getTopLeft(),
				Int2(attributeTextBoxRect.getWidth(), attributeTextBoxRect.getHeight()),
				PivotType::TopLeft);

			this->bonusPointsTextBox.setText(std::to_string(this->bonusPoints));
			const Rect &bonusTextBoxRect = this->bonusPointsTextBox.getRect();
			this->addDrawCall(
				this->bonusPointsTextBox.getTextureID(),
				bonusTextBoxRect.getTopLeft(),
				Int2(bonusTextBoxRect.getWidth(), bonusTextBoxRect.getHeight()),
				PivotType::TopLeft);

			DebugLogFormat("%s decremented to %d, %d remaining bonus points.", attribute.name, attribute.maxValue, this->bonusPoints);
		},
			Rect(),
			[this, attributeIndex]()
		{
			Game &game = this->getGame();
			CharacterCreationState &charCreationState = game.getCharacterCreationState();
			const int *changedPoints = std::begin(charCreationState.changedPoints);
			return attributeIndex == this->selectedAttributeIndex && changedPoints[attributeIndex] > 0;
		});
	}

	const Int2 bonusPointsTextureTopLeftPosition = ChooseAttributesUiView::BonusPointsTextureTopLeftPosition;
	const Int2 bonusPointsTextureDims = *renderer.tryGetUiTextureDims(bonusPointsTextureID);
	constexpr Int2 bonusPointsTextBoxTopLeftPosition(92, 113);

	this->addDrawCall(
		bonusPointsTextureID,
		bonusPointsTextureTopLeftPosition,
		bonusPointsTextureDims,
		PivotType::TopLeft);

	const TextBox::InitInfo bonusPointsTextBoxInitInfo = TextBox::InitInfo::makeWithXY(
		std::to_string(bonusPoints),
		bonusPointsTextBoxTopLeftPosition.x,
		bonusPointsTextBoxTopLeftPosition.y,
		ArenaFontName::Arena,
		Color::Yellow,
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
		this->bonusPointsTextBox.getTextureID(),
		bonusPointsTextBoxRect.getTopLeft(),
		Int2(bonusPointsTextBoxRect.getWidth(), bonusPointsTextBoxRect.getHeight()),
		PivotType::TopLeft);

	for (int attributeIndex = 0; attributeIndex < PrimaryAttributes::COUNT; attributeIndex++)
	{
		UiDrawCall::TextureFunc attributeTextureFunc = [this, &game, attributeIndex]()
		{
			const CharacterCreationState &charCreationState = game.getCharacterCreationState();
			const PrimaryAttributes &attributes = charCreationState.attributes;
			const PrimaryAttribute &attribute = attributes.getAttributes()[attributeIndex];
			const int attributeValue = attribute.maxValue;
			const std::string attributeValueText = std::to_string(attributeValue);
			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			attributeTextBox.setText(attributeValueText);
			return attributeTextBox.getTextureID();
		};

		UiDrawCall::PositionFunc attributePositionFunc = [this, attributeIndex]()
		{
			const Rect &attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();
			return attributeTextBoxRect.getTopLeft();
		};

		UiDrawCall::SizeFunc attributeSizeFunc = [this, attributeIndex]()
		{
			const Rect &attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();
			return Int2(attributeTextBoxRect.getWidth(), attributeTextBoxRect.getHeight());
		};

		UiDrawCall::PivotFunc attributePivotFunc = []()
		{
			return PivotType::TopLeft;
		};

		this->addDrawCall(
			attributeTextureFunc,
			attributePositionFunc,
			attributeSizeFunc,
			attributePivotFunc,
			UiDrawCall::defaultActiveFunc);
	}

	const Rect &playerExperienceTextBoxRect = this->experienceTextBox.getRect();
	this->addDrawCall(
		this->experienceTextBox.getTextureID(),
		playerExperienceTextBoxRect.getTopLeft(),
		Int2(playerExperienceTextBoxRect.getWidth(), playerExperienceTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect &playerLevelTextBoxRect = this->levelTextBox.getRect();
	this->addDrawCall(
		this->levelTextBox.getTextureID(),
		playerLevelTextBoxRect.getTopLeft(),
		Int2(playerLevelTextBoxRect.getWidth(), playerLevelTextBoxRect.getHeight()),
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
		ChooseAttributesUiView::getInitialTextureWidth(),
		ChooseAttributesUiView::getInitialTextureHeight(),
		game.textureManager,
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

	this->redrawAttributeArrows();

	return true;
}

void ChooseAttributesPanel::redrawAttributeArrows()
{
	auto &renderer = this->getGame().renderer;
	const UiTextureID upDownTextureID = this->upDownTextureRef.get();
	const Int2 upDownTextureDims = *renderer.tryGetUiTextureDims(upDownTextureID);

	this->clearDrawCalls();

	auto &game = this->getGame();
	this->addDrawCall(
		this->bodyTextureRef.get(),
		ChooseAttributesUiView::getBodyOffset(game),
		*renderer.tryGetUiTextureDims(this->bodyTextureRef.get()),
		PivotType::TopLeft);

	this->addDrawCall(
		this->pantsTextureRef.get(),
		ChooseAttributesUiView::getPantsOffset(game),
		*renderer.tryGetUiTextureDims(this->pantsTextureRef.get()),
		PivotType::TopLeft);

	this->addDrawCall(
		this->statsBgTextureRef.get(),
		Int2::Zero,
		*renderer.tryGetUiTextureDims(this->statsBgTextureRef.get()),
		PivotType::TopLeft);

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
		headTextureFunc,
		headPositionFunc,
		headSizeFunc,
		[]() { return PivotType::TopLeft; },
		UiDrawCall::defaultActiveFunc);

	this->addDrawCall(
		this->shirtTextureRef.get(),
		ChooseAttributesUiView::getShirtOffset(game),
		*renderer.tryGetUiTextureDims(this->shirtTextureRef.get()),
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

	for (int attributeIndex = 0; attributeIndex < PrimaryAttributes::COUNT; attributeIndex++)
	{
		const Rect &attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();

		UiDrawCall::TextureFunc attributeTextureFunc = [this, &game, attributeIndex]()
		{
			const CharacterCreationState &charCreationState = game.getCharacterCreationState();
			const PrimaryAttributes &attributes = charCreationState.attributes;
			const PrimaryAttribute &attribute = attributes.getAttributes()[attributeIndex];
			const int attributeValue = attribute.maxValue;
			const std::string attributeValueText = std::to_string(attributeValue);
			TextBox &attributeTextBox = this->attributeTextBoxes[attributeIndex];
			attributeTextBox.setText(attributeValueText); // @todo: this seems gross, dirtying the text every frame?
			return attributeTextBox.getTextureID();
		};

		this->addDrawCall(
			attributeTextureFunc,
			[this, attributeIndex]() { return this->attributeTextBoxes[attributeIndex].getRect().getTopLeft(); },
			[this, attributeIndex]()
		{
			const Rect &r = this->attributeTextBoxes[attributeIndex].getRect();
			return Int2(r.getWidth(), r.getHeight());
		}, []() { return PivotType::TopLeft; },
			UiDrawCall::defaultActiveFunc);

		if (attributeIndex == this->selectedAttributeIndex)
		{
			const Int2 buttonCenter(
				ChooseAttributesUiView::UpDownButtonFirstTopLeftPosition.x + (this->upDownTextureRef.getWidth() / 2),
				attributeTextBoxRect.getCenter().y);

			this->addDrawCall(
				upDownTextureID,
				buttonCenter,
				Int2(this->upDownTextureRef.getWidth(), this->upDownTextureRef.getHeight()),
				PivotType::Middle);
		}
	}

	const Rect &experienceTextBoxRect = this->experienceTextBox.getRect();
	this->addDrawCall(
		this->experienceTextBox.getTextureID(),
		experienceTextBoxRect.getTopLeft(),
		Int2(experienceTextBoxRect.getWidth(), experienceTextBoxRect.getHeight()),
		PivotType::TopLeft);

	const Rect &levelTextBoxRect = this->levelTextBox.getRect();
	this->addDrawCall(
		this->levelTextBox.getTextureID(),
		levelTextBoxRect.getTopLeft(),
		Int2(levelTextBoxRect.getWidth(), levelTextBoxRect.getHeight()),
		PivotType::TopLeft);

	this->addDrawCall(
		this->bonusPointsTextureRef.get(),
		ChooseAttributesUiView::BonusPointsTextureTopLeftPosition,
		*renderer.tryGetUiTextureDims(this->bonusPointsTextureRef.get()),
		PivotType::TopLeft);

	const Rect &bonusPointsTextBoxRect = this->bonusPointsTextBox.getRect();
	this->addDrawCall(
		this->bonusPointsTextBox.getTextureID(),
		bonusPointsTextBoxRect.getTopLeft(),
		Int2(bonusPointsTextBoxRect.getWidth(), bonusPointsTextBoxRect.getHeight()),
		PivotType::TopLeft);

	this->addCursorDrawCall(this->cursorTextureRef.get(), CommonUiView::DefaultCursorPivotType);
}

void ChooseAttributesPanel::tick(double dt)
{
	Panel::tick(dt);

	if (this->selectedAttributeIndex != this->lastSelectedAttributeIndex)
	{
		this->redrawAttributeArrows();

		this->lastSelectedAttributeIndex = this->selectedAttributeIndex;
	}
}
