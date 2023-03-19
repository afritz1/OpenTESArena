#include <optional>

#include "SDL.h"

#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseAttributesPanel.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "ChooseGenderPanel.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "GameWorldPanel.h"
#include "MainMenuPanel.h"
#include "MessageBoxSubPanel.h"
#include "TextCinematicPanel.h"
#include "TextSubPanel.h"
#include "WorldMapUiModel.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Game/CardinalDirection.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Sky/SkyUtils.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../UI/TextEntry.h"
#include "../WorldMap/ArenaLocationUtils.h"

#include "components/utilities/String.h"

void ChooseClassCreationUiController::onBackToMainMenuInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setCharacterCreationState(nullptr);
		game.setPanel<MainMenuPanel>();

		const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
		const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
			MusicDefinition::Type::MainMenu, game.getRandom());

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing main menu music.");
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef);
	}
}

void ChooseClassCreationUiController::onGenerateButtonSelected(Game &game)
{
	// @todo: eventually go to a "ChooseQuestionsPanel" with "pop-up" message
}

void ChooseClassCreationUiController::onSelectButtonSelected(Game &game)
{
	game.setPanel<ChooseClassPanel>();
}

void ChooseClassUiController::onBackToChooseClassCreationInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setPanel<ChooseClassCreationPanel>();
	}
}

void ChooseClassUiController::onUpButtonSelected(ListBox &listBox)
{
	listBox.scrollUp();
}

void ChooseClassUiController::onDownButtonSelected(ListBox &listBox)
{
	listBox.scrollDown();
}

void ChooseClassUiController::onItemButtonSelected(Game &game, int charClassDefID)
{
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setClassDefID(charClassDefID);

	game.setPanel<ChooseNamePanel>();
}

void ChooseGenderUiController::onBackToChooseNameInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setPanel<ChooseNamePanel>();
	}
}

void ChooseGenderUiController::onMaleButtonSelected(Game &game)
{
	constexpr bool male = true;
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setGender(male);

	game.setPanel<ChooseRacePanel>();
}

void ChooseGenderUiController::onFemaleButtonSelected(Game &game)
{
	constexpr bool male = false;
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setGender(male);

	game.setPanel<ChooseRacePanel>();
}

void ChooseNameUiController::onBackToChooseClassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		auto &inputManager = game.getInputManager();
		inputManager.setTextInputMode(false);

		auto &charCreationState = game.getCharacterCreationState();
		charCreationState.setName(nullptr);

		game.setPanel<ChooseClassPanel>();
	}
}

void ChooseNameUiController::onTextInput(const std::string_view &text, std::string &name, bool *outDirty)
{
	DebugAssert(outDirty != nullptr);

	*outDirty = TextEntry::append(name, text, ChooseNameUiModel::isCharacterAccepted,
		CharacterCreationState::MAX_NAME_LENGTH);
}

void ChooseNameUiController::onBackspaceInputAction(const InputActionCallbackValues &values, std::string &name, bool *outDirty)
{
	DebugAssert(outDirty != nullptr);

	if (values.performed)
	{
		*outDirty = TextEntry::backspace(name);
	}
}

void ChooseNameUiController::onAcceptInputAction(const InputActionCallbackValues &values, const std::string &name)
{
	if (values.performed)
	{
		if (name.size() > 0)
		{
			auto &game = values.game;
			auto &inputManager = game.getInputManager();
			inputManager.setTextInputMode(false);

			auto &charCreationState = game.getCharacterCreationState();
			charCreationState.setName(name.c_str());

			game.setPanel<ChooseGenderPanel>();
		}
	}
}

void ChooseRaceUiController::onBackToChooseGenderInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setPanel<ChooseGenderPanel>();
	}
}

void ChooseRaceUiController::onInitialPopUpButtonSelected(Game &game)
{
	game.popSubPanel();
}

void ChooseRaceUiController::onMouseButtonChanged(Game &game, MouseButtonType buttonType,
	const Int2 &position, bool pressed)
{
	// Listen for clicks on the map, checking if the mouse is over a province mask.
	if ((buttonType == MouseButtonType::Left) && pressed)
	{
		const std::optional<int> provinceID = WorldMapUiModel::getMaskID(game, position, true, true);
		if (provinceID.has_value())
		{
			ChooseRaceUiController::onProvinceButtonSelected(game, *provinceID);
		}
	}
}

void ChooseRaceUiController::onProvinceButtonSelected(Game &game, int raceID)
{
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setRaceIndex(raceID);
	charCreationState.rollAttributes(game.getRandom());

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	// Populate and display province confirm message box.
	const MessageBoxSubPanel::BackgroundProperties backgroundProperties =
		ChooseRaceUiView::getProvinceConfirmMessageBoxBackgroundProperties();

	const std::string titleText = ChooseRaceUiModel::getProvinceConfirmTitleText(game);
	const Rect titleRect = ChooseRaceUiView::getProvinceConfirmTitleTextBoxRect(titleText, fontLibrary);
	const MessageBoxSubPanel::TitleProperties titleProperties =
		ChooseRaceUiView::getProvinceConfirmMessageBoxTitleProperties(titleText, fontLibrary);
	const MessageBoxSubPanel::ItemsProperties itemsProperties =
		ChooseRaceUiView::getProvinceConfirmMessageBoxItemsProperties(fontLibrary);
	
	std::unique_ptr<MessageBoxSubPanel> panel = std::make_unique<MessageBoxSubPanel>(game);
	if (!panel->init(backgroundProperties, titleRect, titleProperties, itemsProperties))
	{
		DebugCrash("Couldn't init province confirm message box sub-panel.");
	}

	panel->setTitleText(titleText);

	const std::string yesText = ChooseRaceUiModel::getProvinceConfirmYesText(game);
	panel->setItemText(0, yesText);
	panel->setItemCallback(0, [&game, raceID]()
	{
		ChooseRaceUiController::onProvinceConfirmButtonSelected(game, raceID);
	}, false);

	const std::string noText = ChooseRaceUiModel::getProvinceConfirmNoText(game);
	panel->setItemText(1, noText);
	panel->setItemCallback(1, [&game]()
	{
		ChooseRaceUiController::onProvinceCancelButtonSelected(game);
	}, true);

	game.pushSubPanel(std::move(panel));
}

void ChooseRaceUiController::onProvinceConfirmButtonSelected(Game &game, int raceID)
{
	game.popSubPanel();

	const std::string text = ChooseRaceUiModel::getProvinceConfirmedFirstText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedFirstTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedFirstTextFontName,
		ChooseRaceUiView::ProvinceConfirmedFirstTextColor,
		ChooseRaceUiView::ProvinceConfirmedFirstTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedFirstTextLineSpacing,
		game.getFontLibrary());

	const Rect textureRect = ChooseRaceUiView::getProvinceConfirmedFirstTextureRect(
		textBoxInitInfo.rect.getWidth(), textBoxInitInfo.rect.getHeight());

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const Surface surface = TextureUtils::generate(ChooseRaceUiView::ProvinceConfirmedFirstTextPatternType,
		textureRect.getWidth(), textureRect.getHeight(), game.getTextureManager(), renderer);
	
	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create province confirmed #1 pop-up texture.");
	}

	ScopedUiTextureRef textureRef(textureID, renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, ChooseRaceUiController::onProvinceConfirmedFirstButtonSelected,
		std::move(textureRef), textureRect.getCenter());
}

void ChooseRaceUiController::onProvinceCancelButtonSelected(Game &game)
{
	game.popSubPanel();

	// Push the initial text sub-panel.
	std::unique_ptr<Panel> textSubPanel = ChooseRacePanel::getInitialSubPanel(game);
	game.pushSubPanel(std::move(textSubPanel));
}

void ChooseRaceUiController::onProvinceConfirmedFirstButtonSelected(Game &game)
{
	game.popSubPanel();

	const std::string text = ChooseRaceUiModel::getProvinceConfirmedSecondText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedSecondTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedSecondTextFontName,
		ChooseRaceUiView::ProvinceConfirmedSecondTextColor,
		ChooseRaceUiView::ProvinceConfirmedSecondTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedSecondTextLineSpacing,
		game.getFontLibrary());

	const Rect textureRect = ChooseRaceUiView::getProvinceConfirmedSecondTextureRect(
		textBoxInitInfo.rect.getWidth(), textBoxInitInfo.rect.getHeight());

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const Surface surface = TextureUtils::generate(ChooseRaceUiView::ProvinceConfirmedSecondTextPatternType,
		textureRect.getWidth(), textureRect.getHeight(), game.getTextureManager(), renderer);
	
	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create province confirmed #2 pop-up texture.");
	}

	ScopedUiTextureRef textureRef(textureID, renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, ChooseRaceUiController::onProvinceConfirmedSecondButtonSelected,
		std::move(textureRef), textureRect.getCenter());
}

void ChooseRaceUiController::onProvinceConfirmedSecondButtonSelected(Game &game)
{
	game.popSubPanel();

	const std::string text = ChooseRaceUiModel::getProvinceConfirmedThirdText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedThirdTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedThirdTextFontName,
		ChooseRaceUiView::ProvinceConfirmedThirdTextColor,
		ChooseRaceUiView::ProvinceConfirmedThirdTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedThirdTextLineSpacing,
		game.getFontLibrary());

	const Rect textureRect = ChooseRaceUiView::getProvinceConfirmedThirdTextureRect(
		textBoxInitInfo.rect.getWidth(), textBoxInitInfo.rect.getHeight());

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const Surface surface = TextureUtils::generate(ChooseRaceUiView::ProvinceConfirmedThirdTextPatternType,
		textureRect.getWidth(), textureRect.getHeight(), game.getTextureManager(), renderer);
	
	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create province confirmed #3 pop-up texture.");
	}

	ScopedUiTextureRef textureRef(textureID, renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, ChooseRaceUiController::onProvinceConfirmedThirdButtonSelected,
		std::move(textureRef), textureRect.getCenter());
}

void ChooseRaceUiController::onProvinceConfirmedThirdButtonSelected(Game &game)
{
	game.popSubPanel();

	const std::string text = ChooseRaceUiModel::getProvinceConfirmedFourthText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedFourthTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedFourthTextFontName,
		ChooseRaceUiView::ProvinceConfirmedFourthTextColor,
		ChooseRaceUiView::ProvinceConfirmedFourthTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedFourthTextLineSpacing,
		game.getFontLibrary());

	const Rect textureRect = ChooseRaceUiView::getProvinceConfirmedFourthTextureRect(
		textBoxInitInfo.rect.getWidth(), textBoxInitInfo.rect.getHeight());

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	Surface surface = TextureUtils::generate(
		ChooseRaceUiView::ProvinceConfirmedFourthTextPatternType,
		textureRect.getWidth(),
		textureRect.getHeight(),
		game.getTextureManager(),
		renderer);
	
	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create province confirmed #4 pop-up texture.");
	}

	ScopedUiTextureRef textureRef(textureID, renderer);
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, ChooseRaceUiController::onProvinceConfirmedFourthButtonSelected,
		std::move(textureRef), textureRect.getCenter());
}

void ChooseRaceUiController::onProvinceConfirmedFourthButtonSelected(Game &game)
{
	game.popSubPanel();
	game.setPanel<ChooseAttributesPanel>();
}

void ChooseAttributesUiController::onBackToRaceSelectionInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		game.setPanel<ChooseRacePanel>();
	}
}

void ChooseAttributesUiController::onInitialPopUpSelected(Game &game)
{
	game.popSubPanel();
}

void ChooseAttributesUiController::onUnsavedDoneButtonSelected(Game &game, bool *attributesAreSaved)
{
	// Show message box to save or reroll.
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	const MessageBoxSubPanel::BackgroundProperties backgroundProperties =
		ChooseAttributesUiView::getMessageBoxBackgroundProperties();

	const std::string titleText = ChooseAttributesUiModel::getMessageBoxTitleText(game);
	const Rect titleRect = ChooseAttributesUiView::getMessageBoxTitleTextBoxRect(titleText, fontLibrary);
	const MessageBoxSubPanel::TitleProperties titleProperties =
		ChooseAttributesUiView::getMessageBoxTitleProperties(titleText, fontLibrary);
	const MessageBoxSubPanel::ItemsProperties itemsProperties =
		ChooseAttributesUiView::getMessageBoxItemsProperties(fontLibrary);

	auto onClosed = [&game]()
	{
		auto &inputManager = game.getInputManager();
		inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, false);
	};

	std::unique_ptr<MessageBoxSubPanel> panel = std::make_unique<MessageBoxSubPanel>(game);
	if (!panel->init(backgroundProperties, titleRect, titleProperties, itemsProperties, onClosed))
	{
		DebugCrash("Couldn't init save/reroll message box sub-panel.");
	}

	panel->setTitleText(titleText);

	const std::string saveText = ChooseAttributesUiModel::getMessageBoxSaveText(game);
	panel->setItemText(0, saveText);
	panel->setItemCallback(0, [&game, attributesAreSaved]()
	{
		ChooseAttributesUiController::onSaveButtonSelected(game, attributesAreSaved);
	}, false);

	const std::vector<TextRenderUtils::ColorOverrideInfo::Entry> saveTextColorOverrides =
		ChooseAttributesUiModel::getMessageBoxSaveColorOverrides(game);
	for (const TextRenderUtils::ColorOverrideInfo::Entry &entry : saveTextColorOverrides)
	{
		panel->addOverrideColor(0, entry.charIndex, entry.color);
	}

	panel->setItemInputAction(0, InputActionName::SaveAttributes);

	const std::string rerollText = ChooseAttributesUiModel::getMessageBoxRerollText(game);
	panel->setItemText(1, rerollText);
	panel->setItemCallback(1, [&game]()
	{
		ChooseAttributesUiController::onRerollButtonSelected(game);
	}, true);

	const std::vector<TextRenderUtils::ColorOverrideInfo::Entry> rerollTextColorOverrides =
		ChooseAttributesUiModel::getMessageBoxRerollColorOverrides(game);
	for (const TextRenderUtils::ColorOverrideInfo::Entry &entry : rerollTextColorOverrides)
	{
		panel->addOverrideColor(1, entry.charIndex, entry.color);
	}

	panel->setItemInputAction(1, InputActionName::RerollAttributes);

	auto &inputManager = game.getInputManager();
	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, true);

	game.pushSubPanel(std::move(panel));
}

void ChooseAttributesUiController::onSavedDoneButtonSelected(Game &game)
{
	auto gameStateFunction = [](Game &game)
	{
		GameState &gameState = game.getGameState();
		const auto &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		gameState.init(binaryAssetLibrary);

		// Find starting dungeon location definition.
		constexpr int provinceIndex = ArenaLocationUtils::CENTER_PROVINCE_ID;
		const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
		const std::optional<int> locationIndex = [&provinceDef]() -> std::optional<int>
		{
			for (int i = 0; i < provinceDef.getLocationCount(); i++)
			{
				const LocationDefinition &locationDef = provinceDef.getLocationDef(i);
				if (locationDef.getType() == LocationDefinition::Type::MainQuestDungeon)
				{
					const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef =
						locationDef.getMainQuestDungeonDefinition();

					if (mainQuestDungeonDef.type == LocationDefinition::MainQuestDungeonDefinition::Type::Start)
					{
						return i;
					}
				}
			}

			return std::nullopt;
		}();

		DebugAssertMsg(locationIndex.has_value(), "Couldn't find start dungeon location definition.");

		// Load starting dungeon.
		const LocationDefinition &locationDef = provinceDef.getLocationDef(*locationIndex);
		const LocationDefinition::MainQuestDungeonDefinition &mainQuestDungeonDef = locationDef.getMainQuestDungeonDefinition();
		const std::string mifName = mainQuestDungeonDef.mapFilename;

		constexpr std::optional<bool> rulerIsMale; // Not needed.

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initPrefab(std::string(mifName), ArenaTypes::InteriorType::Dungeon, rulerIsMale);

		const std::optional<VoxelInt2> playerStartOffset; // Unused for start dungeon.

		const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);

		gameState.init(binaryAssetLibrary); // @todo: not sure about this; should we init really early in the engine?
		if (!gameState.trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
			BinaryAssetLibrary::getInstance(), game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load start dungeon \"" + mifName + "\".");
		}

		// Initialize player.
		const auto &exeData = binaryAssetLibrary.getExeData();
		const CoordDouble3 dummyPosition(ChunkInt2::Zero, VoxelDouble3::Zero);
		const Double3 direction(
			CardinalDirection::North.x,
			0.0,
			CardinalDirection::North.y);
		const Double3 velocity = Double3::Zero;

		const auto &charCreationState = game.getCharacterCreationState();
		const std::string_view name = charCreationState.getName();
		const bool male = charCreationState.isMale();
		const int raceIndex = charCreationState.getRaceIndex();

		const auto &charClassLibrary = game.getCharacterClassLibrary();
		const int charClassDefID = charCreationState.getClassDefID();
		const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);

		PrimaryAttributeSet attributes = charCreationState.getAttributes();

		const int portraitIndex = charCreationState.getPortraitIndex();

		const int allowedWeaponCount = charClassDef.getAllowedWeaponCount();
		const int weaponID = charClassDef.getAllowedWeapon(game.getRandom().next(allowedWeaponCount));

		Player &player = game.getPlayer();
		player.init(std::string(name), male, raceIndex, charClassDefID, std::move(attributes), portraitIndex,
			dummyPosition, direction, velocity, Player::DEFAULT_WALK_SPEED, weaponID, exeData);
	};

	gameStateFunction(game);

	const auto &cinematicLibrary = game.getCinematicLibrary();
	int textCinematicDefIndex;
	const TextCinematicDefinition *defPtr = nullptr;
	const bool success = cinematicLibrary.findTextDefinitionIndexIf(
		[&defPtr](const TextCinematicDefinition &def)
	{
		if (def.getType() == TextCinematicDefinition::Type::MainQuest)
		{
			const auto &mainQuestCinematicDef = def.getMainQuestDefinition();
			const bool isMainQuestStartCinematic = mainQuestCinematicDef.progress == 0;
			if (isMainQuestStartCinematic)
			{
				defPtr = &def;
				return true;
			}
		}

		return false;
	}, &textCinematicDefIndex);

	if (!success)
	{
		DebugCrash("Couldn't find main quest start text cinematic definition.");
	}

	game.setCharacterCreationState(nullptr);

	TextureManager &textureManager = game.getTextureManager();
	const std::string &cinematicFilename = defPtr->getAnimationFilename();
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(cinematicFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogError("Couldn't get texture file metadata for main quest start cinematic \"" + cinematicFilename + "\".");
		return;
	}

	const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);
	const double secondsPerFrame = metadata.getSecondsPerFrame();
	game.setPanel<TextCinematicPanel>(textCinematicDefIndex, secondsPerFrame, ChooseAttributesUiController::onPostCharacterCreationCinematicFinished);

	// Play dream music.
	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
		MusicDefinition::Type::Cinematic, game.getRandom(), [](const MusicDefinition &def)
	{
		DebugAssert(def.getType() == MusicDefinition::Type::Cinematic);
		const auto &cinematicMusicDef = def.getCinematicMusicDefinition();
		return cinematicMusicDef.type == MusicDefinition::CinematicMusicDefinition::Type::DreamGood;
	});

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing vision music.");
	}

	AudioManager &audioManager = game.getAudioManager();
	audioManager.setMusic(musicDef);
}

void ChooseAttributesUiController::onSaveButtonSelected(Game &game, bool *attributesAreSaved)
{
	// Confirming the chosen stats will bring up a text sub-panel, and the next time the done button is clicked,
	// it starts the game.
	game.popSubPanel();

	const std::string text = ChooseAttributesUiModel::getAppearanceText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		ChooseAttributesUiView::AppearanceTextCenterPoint,
		ChooseAttributesUiView::AppearanceTextFontName,
		ChooseAttributesUiView::AppearanceTextColor,
		ChooseAttributesUiView::AppearanceTextAlignment,
		std::nullopt,
		ChooseAttributesUiView::AppearanceTextLineSpacing,
		game.getFontLibrary());

	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const Surface surface = TextureUtils::generate(
		ChooseAttributesUiView::AppearanceTextPatternType,
		ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(textBoxInitInfo.rect.getWidth()),
		ChooseAttributesUiView::getAppearanceTextBoxTextureHeight(textBoxInitInfo.rect.getHeight()),
		game.getTextureManager(),
		renderer);
	
	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create appearance pop-up texture.");
	}

	ScopedUiTextureRef textureRef(textureID, renderer);

	// The done button is replaced after the player confirms their stats, and it then leads to the main quest
	// opening cinematic.
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, ChooseAttributesUiController::onAppearanceTextBoxSelected,
		std::move(textureRef), ChooseAttributesUiView::AppearanceTextCenterPoint);

	*attributesAreSaved = true;
}

void ChooseAttributesUiController::onRerollButtonSelected(Game &game)
{
	auto& charCreationState = game.getCharacterCreationState();
	charCreationState.rollAttributes(game.getRandom());
	
	game.popSubPanel();
}

void ChooseAttributesUiController::onAppearanceTextBoxSelected(Game &game)
{
	game.popSubPanel();
}

void ChooseAttributesUiController::onPortraitButtonSelected(Game &game, bool incrementIndex)
{
	constexpr int minID = 0; // @todo: de-hardcode so it relies on portraits list
	constexpr int maxID = 9;

	auto &charCreationState = game.getCharacterCreationState();
	const int oldPortraitIndex = charCreationState.getPortraitIndex();
	const int newPortraitIndex = incrementIndex ?
		((oldPortraitIndex == maxID) ? minID : (oldPortraitIndex + 1)) :
		((oldPortraitIndex == minID) ? maxID : (oldPortraitIndex - 1));

	charCreationState.setPortraitIndex(newPortraitIndex);
}

void ChooseAttributesUiController::onDoneButtonSelected(Game &game, bool *attributesAreSaved)
{
	if (*attributesAreSaved)
	{
		ChooseAttributesUiController::onSavedDoneButtonSelected(game);
	}
	else
	{
		ChooseAttributesUiController::onUnsavedDoneButtonSelected(game, attributesAreSaved);
	}
}

void ChooseAttributesUiController::onPostCharacterCreationCinematicFinished(Game &game)
{
	// Create the function that will be called when the player leaves the starting dungeon.
	// @todo: this should be in a game logic controller namespace, not UI controller.
	auto onLevelUpVoxelEnter = [](Game &game)
	{
		// Teleport the player to a random location based on their race.
		auto &player = game.getPlayer();
		player.setVelocityToZero();

		auto &gameState = game.getGameState();
		const int provinceID = player.getRaceID();
		const int locationID = game.getRandom().next(32);

		const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceID);
		const LocationDefinition &locationDef = provinceDef.getLocationDef(locationID);

		// Random weather for now.
		// - @todo: make it depend on the location (no need to prevent deserts from having snow
		//   since the climates are still hardcoded).
		const ArenaTypes::WeatherType weatherType = [&game]()
		{
			constexpr std::array<ArenaTypes::WeatherType, 8> Weathers =
			{
				ArenaTypes::WeatherType::Clear,
				ArenaTypes::WeatherType::Overcast,
				ArenaTypes::WeatherType::Rain,
				ArenaTypes::WeatherType::Snow,
				ArenaTypes::WeatherType::SnowOvercast,
				ArenaTypes::WeatherType::Rain2,
				ArenaTypes::WeatherType::Overcast2,
				ArenaTypes::WeatherType::SnowOvercast2
			};

			const int index = game.getRandom().next(static_cast<int>(Weathers.size()));
			DebugAssertIndex(Weathers, index);
			return Weathers[index];
		}();

		const int starCount = SkyUtils::getStarCountFromDensity(game.getOptions().getMisc_StarDensity());
		auto &renderer = game.getRenderer();

		const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
		Buffer<uint8_t> reservedBlocks = [&cityDef]()
		{
			const std::vector<uint8_t> *cityReservedBlocks = cityDef.reservedBlocks;
			DebugAssert(cityReservedBlocks != nullptr);
			Buffer<uint8_t> buffer(static_cast<int>(cityReservedBlocks->size()));
			std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.begin());
			return buffer;
		}();

		const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
			[&cityDef]() -> std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
		{
			if (cityDef.hasMainQuestTempleOverride)
			{
				return cityDef.mainQuestTempleOverride;
			}
			else
			{
				return std::nullopt;
			}
		}();

		MapGeneration::CityGenInfo cityGenInfo;
		cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName), cityDef.type,
			cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade, cityDef.coastal,
			cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
			mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY, cityDef.cityBlocksPerSide);

		const int currentDay = gameState.getDate().getDay();
		const WeatherDefinition overrideWeather = [&game, weatherType, currentDay]()
		{
			WeatherDefinition weatherDef;
			weatherDef.initFromClassic(weatherType, currentDay, game.getRandom());
			return weatherDef;
		}();

		SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
		skyGenInfo.init(cityDef.climateType, overrideWeather, currentDay, starCount, cityDef.citySeed,
			cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

		const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceID, locationID);
		if (!gameState.trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
			BinaryAssetLibrary::getInstance(), TextAssetLibrary::getInstance(), game.getTextureManager(),
			renderer))
		{
			DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
		}

		// Set music based on weather and time.
		const MusicDefinition *musicDef = [&game, &gameState]()
		{
			const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
			if (!gameState.nightMusicIsActive())
			{
				const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
				return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
					game.getRandom(), [&weatherDef](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Weather);
					const auto &weatherMusicDef = def.getWeatherMusicDefinition();
					return weatherMusicDef.weatherDef == weatherDef;
				});
			}
			else
			{
				return musicLibrary.getRandomMusicDefinition(
					MusicDefinition::Type::Night, game.getRandom());
			}
		}();

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing exterior music.");
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef);
	};

	// Set the *LEVELUP voxel enter event.
	auto &gameState = game.getGameState();
	gameState.getOnLevelUpVoxelEnter() = std::move(onLevelUpVoxelEnter);

	// Update game state so the pending map change can get applied (if any). This matters only when letting
	// the cinematic finish naturally in the CD version, which causes GameWorldPanel::tick() to not get run
	// before GameWorldPanel::gameWorldRenderCallback() this frame, which causes first-frame issues and a crash.
	// Not sure what a better fix would be other than this "first frame poke" method with dt=0.
	gameState.tick(0.0, game);

	// Initialize the game world panel.
	game.setPanel<GameWorldPanel>();

	// Choose random dungeon music.
	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
		MusicDefinition::Type::Interior, game.getRandom(), [](const MusicDefinition &def)
	{
		DebugAssert(def.getType() == MusicDefinition::Type::Interior);
		const auto &interiorMusicDef = def.getInteriorMusicDefinition();
		return interiorMusicDef.type == MusicDefinition::InteriorMusicDefinition::Type::Dungeon;
	});

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing dungeon music.");
	}

	AudioManager &audioManager = game.getAudioManager();
	audioManager.setMusic(musicDef);
}
