#include <algorithm>
#include <array>
#include <map>

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
#include "../Stats/PrimaryAttribute.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/TextureUtils.h"

ChooseAttributesPanel::ChooseAttributesPanel(Game &game)
	: Panel(game) { }

bool ChooseAttributesPanel::init()
{
	auto &game = this->getGame();
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setPortraitIndex(0);

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

	// text for Arrows 
	const TextureAsset upDownTextureAsset = TextureAsset(std::string(ArenaTextureName::UpDown));
	UiTextureID upDownTextureID;
	const TextureAsset paletteTextureAsset = TextureAsset(std::string(ArenaPaletteName::CharSheet));
	if (!TextureUtils::tryAllocUiTexture(upDownTextureAsset, paletteTextureAsset, textureManager, renderer, &upDownTextureID))
	{
		DebugLogError("Couldn't get texture ID for up/down arrows.");
		return false;
	}
	this->upDownTextureRef.init(upDownTextureID, renderer);

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

	// code arrow
	for (int attributeIndex = 0; attributeIndex < PrimaryAttributes::COUNT; attributeIndex++)
	{
		const Rect &attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();
		const Int2 buttonCenter(
			attributeTextBoxRect.getRight() + 5,
			attributeTextBoxRect.getCenter().y);
		
		this->upDownButtons[attributeIndex] = Button<>();
		this->upDownButtons[attributeIndex].setX(buttonCenter.x - (upDownTextureDims.x / 2));
		this->upDownButtons[attributeIndex].setY(buttonCenter.y - (upDownTextureDims.y / 2));
		this->upDownButtons[attributeIndex].setWidth(upDownTextureDims.x);
		this->upDownButtons[attributeIndex].setHeight(upDownTextureDims.y);
			
		this->addDrawCall(
			upDownTextureID,
			buttonCenter,
			upDownTextureDims,
			PivotType::Middle);
			
		this->addButtonProxy(MouseButtonType::Left, this->upDownButtons[attributeIndex].getRect(),
			[this, attributeIndex, &game]()
			{

				const CharacterCreationState& charCreationState = game.getCharacterCreationState();
				BufferView<PrimaryAttribute> attributesView = const_cast<PrimaryAttributes&>(charCreationState.getAttributes()).getAttributes();
				PrimaryAttribute& modifiableAttribute = attributesView.get(attributeIndex);
				modifiableAttribute.maxValue += 1;
				DebugLog("Selected attribute: " + std::string(modifiableAttribute.name));

				// update the  TextBox for atribute
				std::string updatedValueText = std::to_string(modifiableAttribute.maxValue);
				this->attributeTextBoxes[attributeIndex].setText(updatedValueText);
				const Rect& attributeTextBoxRect = this->attributeTextBoxes[attributeIndex].getRect();
				this->addDrawCall(
					[this, attributeIndex]() { return this->attributeTextBoxes[attributeIndex].getTextureID(); },
					[attributeTextBoxRect]() { return attributeTextBoxRect.getTopLeft(); },
					[attributeTextBoxRect]() { return Int2(attributeTextBoxRect.getWidth(), attributeTextBoxRect.getHeight()); },
					[]() { return PivotType::TopLeft; },
					UiDrawCall::defaultActiveFunc);			
			});
	}

	for (int attributeIndex = 0; attributeIndex < PrimaryAttributes::COUNT; attributeIndex++)
	{
		UiDrawCall::TextureFunc attributeTextureFunc = [this, &game, attributeIndex]()
		{
			const CharacterCreationState &charCreationState = game.getCharacterCreationState();
			const PrimaryAttributes &attributes = charCreationState.getAttributes();
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

	return true;
}
