#include <algorithm>
#include <array>

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
#include "../UI/Surface.h"

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

	const std::string playerStrengthText = CharacterCreationUiModel::getPlayerStrengthText(game);
	const TextBox::InitInfo playerStrengthTextBoxInitInfo =
		CharacterSheetUiView::getPlayerStrengthTextBoxInitInfo(playerStrengthText, fontLibrary);
	if (!this->strengthTextBox.init(playerStrengthTextBoxInitInfo, playerStrengthText, renderer))
	{
		DebugLogError("Couldn't init player strength text box.");
		return false;
	}

	const std::string playerIntelligenceText = CharacterCreationUiModel::getPlayerIntelligenceText(game);
	const TextBox::InitInfo playerIntelligenceTextBoxInitInfo =
		CharacterSheetUiView::getPlayerIntelligenceTextBoxInitInfo(playerIntelligenceText, fontLibrary);
	if (!this->intelligenceTextBox.init(playerIntelligenceTextBoxInitInfo, playerIntelligenceText, renderer))
	{
		DebugLogError("Couldn't init player intelligence text box.");
		return false;
	}

	const std::string playerWillpowerText = CharacterCreationUiModel::getPlayerWillpowerText(game);
	const TextBox::InitInfo playerWillpowerTextBoxInitInfo =
		CharacterSheetUiView::getPlayerWillpowerTextBoxInitInfo(playerWillpowerText, fontLibrary);
	if (!this->willpowerTextBox.init(playerWillpowerTextBoxInitInfo, playerWillpowerText, renderer))
	{
		DebugLogError("Couldn't init player willpower text box.");
		return false;
	}

	const std::string playerAgilityText = CharacterCreationUiModel::getPlayerAgilityText(game);
	const TextBox::InitInfo playerAgilityTextBoxInitInfo =
		CharacterSheetUiView::getPlayerAgilityTextBoxInitInfo(playerAgilityText, fontLibrary);
	if (!this->agilityTextBox.init(playerAgilityTextBoxInitInfo, playerAgilityText, renderer))
	{
		DebugLogError("Couldn't init player agility text box.");
		return false;
	}

	const std::string playerSpeedText = CharacterCreationUiModel::getPlayerSpeedText(game);
	const TextBox::InitInfo playerSpeedTextBoxInitInfo =
		CharacterSheetUiView::getPlayerSpeedTextBoxInitInfo(playerSpeedText, fontLibrary);
	if (!this->speedTextBox.init(playerSpeedTextBoxInitInfo, playerSpeedText, renderer))
	{
		DebugLogError("Couldn't init player speed text box.");
		return false;
	}

	const std::string playerEnduranceText = CharacterCreationUiModel::getPlayerEnduranceText(game);
	const TextBox::InitInfo playerEnduranceTextBoxInitInfo =
		CharacterSheetUiView::getPlayerEnduranceTextBoxInitInfo(playerEnduranceText, fontLibrary);
	if (!this->enduranceTextBox.init(playerEnduranceTextBoxInitInfo, playerEnduranceText, renderer))
	{
		DebugLogError("Couldn't init player endurance text box.");
		return false;
	}

	const std::string playerPersonalityText = CharacterCreationUiModel::getPlayerPersonalityText(game);
	const TextBox::InitInfo playerPersonalityTextBoxInitInfo =
		CharacterSheetUiView::getPlayerPersonalityTextBoxInitInfo(playerPersonalityText, fontLibrary);
	if (!this->personalityTextBox.init(playerPersonalityTextBoxInitInfo, playerPersonalityText, renderer))
	{
		DebugLogError("Couldn't init player personality text box.");
		return false;
	}

	const std::string playerLuckText = CharacterCreationUiModel::getPlayerLuckText(game);
	const TextBox::InitInfo playerLuckTextBoxInitInfo =
		CharacterSheetUiView::getPlayerLuckTextBoxInitInfo(playerLuckText, fontLibrary);
	if (!this->luckTextBox.init(playerLuckTextBoxInitInfo, playerLuckText, renderer))
	{
		DebugLogError("Couldn't init player luck text box.");
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

	UiDrawCall::TextureFunc strengthTextureFunc = [this, &game]()
	{
		auto& charCreationState = game.getCharacterCreationState();
		const int strength = charCreationState.getStrength();
		const std::string strengthText = std::to_string(strength);
		this->strengthTextBox.setText(strengthText);
		return this->strengthTextBox.getTextureID();
	};

	UiDrawCall::PositionFunc strengthPositionFunc = [this]()
	{
		const Rect& strengthTextBoxRect = this->strengthTextBox.getRect();
		return strengthTextBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc strengthSizeFunc = [this]()
	{
		const Rect& strengthTextBoxRect = this->strengthTextBox.getRect();
		return Int2(strengthTextBoxRect.getWidth(), strengthTextBoxRect.getHeight());
	};

	UiDrawCall::PivotFunc strengthPivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		strengthTextureFunc,
		strengthPositionFunc,
		strengthSizeFunc,
		strengthPivotFunc,
		UiDrawCall::defaultActiveFunc);
	
	UiDrawCall::TextureFunc intelligenceTextureFunc = [this, &game]()
	{
		auto& charCreationState = game.getCharacterCreationState();
		const int intelligence = charCreationState.getIntelligence();
		const std::string intelligenceText = std::to_string(intelligence);
		this->intelligenceTextBox.setText(intelligenceText);
		return this->intelligenceTextBox.getTextureID();
	};

	UiDrawCall::PositionFunc intelligencePositionFunc = [this]()
	{
		const Rect& intelligenceTextBoxRect = this->intelligenceTextBox.getRect();
		return intelligenceTextBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc intelligenceSizeFunc = [this]()
	{
		const Rect& intelligenceTextBoxRect = this->intelligenceTextBox.getRect();
		return Int2(intelligenceTextBoxRect.getWidth(), intelligenceTextBoxRect.getHeight());
	};

	UiDrawCall::PivotFunc intelligencePivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		intelligenceTextureFunc,
		intelligencePositionFunc,
		intelligenceSizeFunc,
		intelligencePivotFunc,
		UiDrawCall::defaultActiveFunc);

	UiDrawCall::TextureFunc willpowerTextureFunc = [this, &game]()
	{
		auto& charCreationState = game.getCharacterCreationState();
		const int willpower = charCreationState.getWillpower();
		const std::string willpowerText = std::to_string(willpower);
		this->willpowerTextBox.setText(willpowerText);
		return this->willpowerTextBox.getTextureID();
	};

	UiDrawCall::PositionFunc willpowerPositionFunc = [this]()
	{
		const Rect& willpowerTextBoxRect = this->willpowerTextBox.getRect();
		return willpowerTextBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc willpowerSizeFunc = [this]()
	{
		const Rect& willpowerTextBoxRect = this->willpowerTextBox.getRect();
		return Int2(willpowerTextBoxRect.getWidth(), willpowerTextBoxRect.getHeight());
	};

	UiDrawCall::PivotFunc willpowerPivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		willpowerTextureFunc,
		willpowerPositionFunc,
		willpowerSizeFunc,
		willpowerPivotFunc,
		UiDrawCall::defaultActiveFunc);

	UiDrawCall::TextureFunc agilityTextureFunc = [this, &game]()
	{
		auto& charCreationState = game.getCharacterCreationState();
		const int agility = charCreationState.getAgility();
		const std::string agilityText = std::to_string(agility);
		this->agilityTextBox.setText(agilityText);
		return this->agilityTextBox.getTextureID();
	};

	UiDrawCall::PositionFunc agilityPositionFunc = [this]()
	{
		const Rect& agilityTextBoxRect = this->agilityTextBox.getRect();
		return agilityTextBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc agilitySizeFunc = [this]()
	{
		const Rect& agilityTextBoxRect = this->agilityTextBox.getRect();
		return Int2(agilityTextBoxRect.getWidth(), agilityTextBoxRect.getHeight());
	};

	UiDrawCall::PivotFunc agilityPivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		agilityTextureFunc,
		agilityPositionFunc,
		agilitySizeFunc,
		agilityPivotFunc,
		UiDrawCall::defaultActiveFunc);

	UiDrawCall::TextureFunc speedTextureFunc = [this, &game]()
	{
		auto& charCreationState = game.getCharacterCreationState();
		const int speed = charCreationState.getSpeed();
		const std::string speedText = std::to_string(speed);
		this->speedTextBox.setText(speedText);
		return this->speedTextBox.getTextureID();
	};

	UiDrawCall::PositionFunc speedPositionFunc = [this]()
	{
		const Rect& speedTextBoxRect = this->speedTextBox.getRect();
		return speedTextBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc speedSizeFunc = [this]()
	{
		const Rect& speedTextBoxRect = this->speedTextBox.getRect();
		return Int2(speedTextBoxRect.getWidth(), speedTextBoxRect.getHeight());
	};

	UiDrawCall::PivotFunc speedPivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		speedTextureFunc,
		speedPositionFunc,
		speedSizeFunc,
		speedPivotFunc,
		UiDrawCall::defaultActiveFunc);

	UiDrawCall::TextureFunc enduranceTextureFunc = [this, &game]()
	{
		auto& charCreationState = game.getCharacterCreationState();
		const int endurance = charCreationState.getEndurance();
		const std::string enduranceText = std::to_string(endurance);
		this->enduranceTextBox.setText(enduranceText);
		return this->enduranceTextBox.getTextureID();
	};

	UiDrawCall::PositionFunc endurancePositionFunc = [this]()
	{
		const Rect& enduranceTextBoxRect = this->enduranceTextBox.getRect();
		return enduranceTextBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc enduranceSizeFunc = [this]()
	{
		const Rect& enduranceTextBoxRect = this->enduranceTextBox.getRect();
		return Int2(enduranceTextBoxRect.getWidth(), enduranceTextBoxRect.getHeight());
	};

	UiDrawCall::PivotFunc endurancePivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		enduranceTextureFunc,
		endurancePositionFunc,
		enduranceSizeFunc,
		endurancePivotFunc,
		UiDrawCall::defaultActiveFunc);

	UiDrawCall::TextureFunc personalityTextureFunc = [this, &game]()
	{
		auto& charCreationState = game.getCharacterCreationState();
		const int personality = charCreationState.getPersonality();
		const std::string personalityText = std::to_string(personality);
		this->personalityTextBox.setText(personalityText);
		return this->personalityTextBox.getTextureID();
	};

	UiDrawCall::PositionFunc personalityPositionFunc = [this]()
	{
		const Rect& personalityTextBoxRect = this->personalityTextBox.getRect();
		return personalityTextBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc personalitySizeFunc = [this]()
	{
		const Rect& personalityTextBoxRect = this->personalityTextBox.getRect();
		return Int2(personalityTextBoxRect.getWidth(), personalityTextBoxRect.getHeight());
	};

	UiDrawCall::PivotFunc personalityPivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		personalityTextureFunc,
		personalityPositionFunc,
		personalitySizeFunc,
		personalityPivotFunc,
		UiDrawCall::defaultActiveFunc);

	UiDrawCall::TextureFunc luckTextureFunc = [this, &game]()
	{
		auto& charCreationState = game.getCharacterCreationState();
		const int luck = charCreationState.getLuck();
		const std::string luckText = std::to_string(luck);
		this->luckTextBox.setText(luckText);
		return this->luckTextBox.getTextureID();
	};

	UiDrawCall::PositionFunc luckPositionFunc = [this]()
	{
		const Rect& luckTextBoxRect = this->luckTextBox.getRect();
		return luckTextBoxRect.getTopLeft();
	};

	UiDrawCall::SizeFunc luckSizeFunc = [this]()
	{
		const Rect& luckTextBoxRect = this->luckTextBox.getRect();
		return Int2(luckTextBoxRect.getWidth(), luckTextBoxRect.getHeight());
	};

	UiDrawCall::PivotFunc luckPivotFunc = []()
	{
		return PivotType::TopLeft;
	};

	this->addDrawCall(
		luckTextureFunc,
		luckPositionFunc,
		luckSizeFunc,
		luckPivotFunc,
		UiDrawCall::defaultActiveFunc);

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
		game.getTextureManager(),
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
