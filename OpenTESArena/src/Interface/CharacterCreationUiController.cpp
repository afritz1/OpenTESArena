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
#include "CinematicLibrary.h"
#include "GameWorldPanel.h"
#include "MainMenuPanel.h"
#include "MessageBoxSubPanel.h"
#include "TextCinematicPanel.h"
#include "TextSubPanel.h"
#include "WorldMapUiModel.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Input/InputActionMapName.h"
#include "../Input/InputActionName.h"
#include "../Sky/SkyUtils.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../UI/FontLibrary.h"
#include "../UI/Surface.h"
#include "../UI/TextBox.h"
#include "../UI/TextEntry.h"
#include "../World/CardinalDirection.h"
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
			MusicType::MainMenu, game.random);

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing main menu music.");
		}

		AudioManager &audioManager = game.audioManager;
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
	charCreationState.classDefID = charClassDefID;

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
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.male = true;

	game.setPanel<ChooseRacePanel>();
}

void ChooseGenderUiController::onFemaleButtonSelected(Game &game)
{
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.male = false;

	game.setPanel<ChooseRacePanel>();
}

void ChooseNameUiController::onBackToChooseClassInputAction(const InputActionCallbackValues &values)
{
	if (values.performed)
	{
		auto &game = values.game;
		auto &inputManager = game.inputManager;
		inputManager.setTextInputMode(false);

		auto &charCreationState = game.getCharacterCreationState();
		charCreationState.setName(nullptr);

		game.setPanel<ChooseClassPanel>();
	}
}

void ChooseNameUiController::onTextInput(const std::string_view text, std::string &name, bool *outDirty)
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
			auto &inputManager = game.inputManager;
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
	charCreationState.raceIndex = raceID;

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	// Populate and display province confirm message box.
	const MessageBoxBackgroundProperties backgroundProperties = ChooseRaceUiView::getProvinceConfirmMessageBoxBackgroundProperties();

	const std::string titleText = ChooseRaceUiModel::getProvinceConfirmTitleText(game);
	const Rect titleRect = ChooseRaceUiView::getProvinceConfirmTitleTextBoxRect(titleText, fontLibrary);
	const MessageBoxTitleProperties titleProperties = ChooseRaceUiView::getProvinceConfirmMessageBoxTitleProperties(titleText, fontLibrary);
	const MessageBoxItemsProperties itemsProperties = ChooseRaceUiView::getProvinceConfirmMessageBoxItemsProperties(fontLibrary);
	
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
	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedFirstTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedFirstTextFontName,
		ChooseRaceUiView::ProvinceConfirmedFirstTextColor,
		ChooseRaceUiView::ProvinceConfirmedFirstTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedFirstTextLineSpacing,
		FontLibrary::getInstance());

	const Rect textureRect = ChooseRaceUiView::getProvinceConfirmedFirstTextureRect(
		textBoxInitInfo.rect.width, textBoxInitInfo.rect.height);

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const Surface surface = TextureUtils::generate(ChooseRaceUiView::ProvinceConfirmedFirstTextPatternType,
		textureRect.width, textureRect.height, game.textureManager, renderer);
	
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
	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedSecondTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedSecondTextFontName,
		ChooseRaceUiView::ProvinceConfirmedSecondTextColor,
		ChooseRaceUiView::ProvinceConfirmedSecondTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedSecondTextLineSpacing,
		FontLibrary::getInstance());

	const Rect textureRect = ChooseRaceUiView::getProvinceConfirmedSecondTextureRect(
		textBoxInitInfo.rect.width, textBoxInitInfo.rect.height);

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const Surface surface = TextureUtils::generate(ChooseRaceUiView::ProvinceConfirmedSecondTextPatternType,
		textureRect.width, textureRect.height, game.textureManager, renderer);
	
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
	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedThirdTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedThirdTextFontName,
		ChooseRaceUiView::ProvinceConfirmedThirdTextColor,
		ChooseRaceUiView::ProvinceConfirmedThirdTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedThirdTextLineSpacing,
		FontLibrary::getInstance());

	const Rect textureRect = ChooseRaceUiView::getProvinceConfirmedThirdTextureRect(
		textBoxInitInfo.rect.width, textBoxInitInfo.rect.height);

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const Surface surface = TextureUtils::generate(ChooseRaceUiView::ProvinceConfirmedThirdTextPatternType,
		textureRect.width, textureRect.height, game.textureManager, renderer);
	
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
	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		ChooseRaceUiView::ProvinceConfirmedFourthTextCenterPoint,
		ChooseRaceUiView::ProvinceConfirmedFourthTextFontName,
		ChooseRaceUiView::ProvinceConfirmedFourthTextColor,
		ChooseRaceUiView::ProvinceConfirmedFourthTextAlignment,
		std::nullopt,
		ChooseRaceUiView::ProvinceConfirmedFourthTextLineSpacing,
		FontLibrary::getInstance());

	const Rect textureRect = ChooseRaceUiView::getProvinceConfirmedFourthTextureRect(
		textBoxInitInfo.rect.width, textBoxInitInfo.rect.height);

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	Surface surface = TextureUtils::generate(
		ChooseRaceUiView::ProvinceConfirmedFourthTextPatternType,
		textureRect.width,
		textureRect.height,
		game.textureManager,
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

void ChooseAttributesUiController::onUnsavedDoneButtonSelected(Game &game, int bonusPointsRemaining, bool *attributesAreSaved)
{
	// Show message box to save or reroll.
	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const auto &fontLibrary = FontLibrary::getInstance();

	const MessageBoxBackgroundProperties backgroundProperties = ChooseAttributesUiView::getMessageBoxBackgroundProperties();

	const std::string titleText = ChooseAttributesUiModel::getMessageBoxTitleText(game);
	const Rect titleRect = ChooseAttributesUiView::getMessageBoxTitleTextBoxRect(titleText, fontLibrary);
	const MessageBoxTitleProperties titleProperties = ChooseAttributesUiView::getMessageBoxTitleProperties(titleText, fontLibrary);
	const MessageBoxItemsProperties itemsProperties = ChooseAttributesUiView::getMessageBoxItemsProperties(fontLibrary);

	auto onClosed = [&game]()
	{
		auto &inputManager = game.inputManager;
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
	panel->setItemCallback(0, [&game, bonusPointsRemaining, attributesAreSaved]()
	{
		if (bonusPointsRemaining == 0)
		{
			*attributesAreSaved = true;
			ChooseAttributesUiController::onSaveButtonSelectedWithNoBonusPoints(game);
		}
		else
		{
			ChooseAttributesUiController::onSaveButtonSelectedWithBonusPoints(game);
		}
	}, false);

	const std::vector<TextRenderColorOverrideInfoEntry> saveTextColorOverrides =
		ChooseAttributesUiModel::getMessageBoxSaveColorOverrides(game);
	for (const TextRenderColorOverrideInfoEntry &entry : saveTextColorOverrides)
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

	const std::vector<TextRenderColorOverrideInfoEntry> rerollTextColorOverrides =
		ChooseAttributesUiModel::getMessageBoxRerollColorOverrides(game);
	for (const TextRenderColorOverrideInfoEntry &entry : rerollTextColorOverrides)
	{
		panel->addOverrideColor(1, entry.charIndex, entry.color);
	}

	panel->setItemInputAction(1, InputActionName::RerollAttributes);

	auto &inputManager = game.inputManager;
	inputManager.setInputActionMapActive(InputActionMapName::CharacterCreation, true);

	game.pushSubPanel(std::move(panel));
}

void ChooseAttributesUiController::onSavedDoneButtonSelected(Game &game)
{
	auto gameStateFunction = [](Game &game)
	{
		GameState &gameState = game.gameState;
		gameState.init(game.arenaRandom);

		// Find starting dungeon location definition.
		constexpr int provinceIndex = ArenaLocationUtils::CENTER_PROVINCE_ID;
		const WorldMapDefinition &worldMapDef = gameState.getWorldMapDefinition();
		const ProvinceDefinition &provinceDef = worldMapDef.getProvinceDef(provinceIndex);
		const std::optional<int> locationIndex = [&provinceDef]() -> std::optional<int>
		{
			for (int i = 0; i < provinceDef.getLocationCount(); i++)
			{
				const LocationDefinition &locationDef = provinceDef.getLocationDef(i);
				if (locationDef.getType() == LocationDefinitionType::MainQuestDungeon)
				{
					const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = locationDef.getMainQuestDungeonDefinition();

					if (mainQuestDungeonDef.type == LocationMainQuestDungeonDefinitionType::Start)
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
		const LocationMainQuestDungeonDefinition &mainQuestDungeonDef = locationDef.getMainQuestDungeonDefinition();
		const std::string mifName = mainQuestDungeonDef.mapFilename;

		constexpr std::optional<bool> rulerIsMale; // Not needed.

		MapGeneration::InteriorGenInfo interiorGenInfo;
		interiorGenInfo.initPrefab(mifName, ArenaTypes::InteriorType::Dungeon, rulerIsMale);

		const GameState::WorldMapLocationIDs worldMapLocationIDs(provinceIndex, *locationIndex);

		MapDefinition mapDefinition;
		if (!mapDefinition.initInterior(interiorGenInfo, game.textureManager))
		{
			DebugLogError("Couldn't init MapDefinition for start dungeon \"" + mifName + "\".");
			return;
		}

		gameState.queueMapDefChange(std::move(mapDefinition), std::nullopt, std::nullopt, VoxelInt2::Zero, worldMapLocationIDs, true);

		// Initialize player.
		const CharacterCreationState &charCreationState = game.getCharacterCreationState();
		const std::string_view name = charCreationState.name;
		const bool male = charCreationState.male;
		const int raceIndex = charCreationState.raceIndex;

		const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const auto &exeData = binaryAssetLibrary.getExeData();

		const int charClassDefID = charCreationState.classDefID;
		const auto &charClassDef = charClassLibrary.getDefinition(charClassDefID);
		const int portraitIndex = charCreationState.portraitIndex;

		const PrimaryAttributes &attributes = charCreationState.attributes;
		const int maxHealth = charCreationState.maxHealth;
		const int maxStamina = charCreationState.maxStamina;
		const int maxSpellPoints = charCreationState.maxSpellPoints;
		const int gold = charCreationState.gold;

		const int allowedWeaponCount = charClassDef.getAllowedWeaponCount();
		const int weaponID = charClassDef.getAllowedWeapon(game.random.next(allowedWeaponCount));

		Player &player = game.player;
		player.init(std::string(name), male, raceIndex, charClassDefID, portraitIndex, attributes, maxHealth, maxStamina, maxSpellPoints,
			gold, weaponID, exeData, game.physicsSystem);

		// Face west so we don't start looking at a wall.
		player.setCameraFrameFromAngles(CardinalDirection::DegreesWest, 0.0);
	};

	gameStateFunction(game);

	const auto &cinematicLibrary = CinematicLibrary::getInstance();
	int textCinematicDefIndex;
	const TextCinematicDefinition *defPtr = nullptr;
	const bool success = cinematicLibrary.findTextDefinitionIndexIf(
		[&defPtr](const TextCinematicDefinition &def)
	{
		if (def.type == TextCinematicDefinitionType::MainQuest)
		{
			const MainQuestTextCinematicDefinition &mainQuestCinematicDef = def.mainQuest;
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

	TextureManager &textureManager = game.textureManager;
	const std::string &cinematicFilename = defPtr->animFilename;
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
		MusicType::Cinematic, game.random, [](const MusicDefinition &def)
	{
		DebugAssert(def.type == MusicType::Cinematic);
		const CinematicMusicDefinition &cinematicMusicDef = def.cinematic;
		return cinematicMusicDef.type == CinematicMusicType::DreamGood;
	});

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing vision music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);
}

void ChooseAttributesUiController::onSaveButtonSelectedWithNoBonusPoints(Game &game)
{
	// Confirming the chosen stats will bring up a text sub-panel, and the next time the done button is clicked,
	// it starts the game.
	game.popSubPanel();

	const std::string text = ChooseAttributesUiModel::getAppearanceText(game);
	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		ChooseAttributesUiView::AppearanceTextCenterPoint,
		ChooseAttributesUiView::AppearanceTextFontName,
		ChooseAttributesUiView::AppearanceTextColor,
		ChooseAttributesUiView::AppearanceTextAlignment,
		std::nullopt,
		ChooseAttributesUiView::AppearanceTextLineSpacing,
		FontLibrary::getInstance());

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const Surface surface = TextureUtils::generate(
		ChooseAttributesUiView::AppearanceTextPatternType,
		ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(textBoxInitInfo.rect.width),
		ChooseAttributesUiView::getAppearanceTextBoxTextureHeight(textBoxInitInfo.rect.height),
		textureManager,
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
}

void ChooseAttributesUiController::onSaveButtonSelectedWithBonusPoints(Game &game)
{
	// Pop the save/reroll sub-panel and tell the player to spend remaining points.
	game.popSubPanel();

	const std::string text = ChooseAttributesUiModel::getBonusPointsRemainingText(game);
	const TextBoxInitInfo textBoxInitInfo = TextBoxInitInfo::makeWithCenter(
		text,
		ChooseAttributesUiView::AppearanceTextCenterPoint,
		ChooseAttributesUiView::AppearanceTextFontName,
		ChooseAttributesUiView::AppearanceTextColor,
		ChooseAttributesUiView::AppearanceTextAlignment,
		std::nullopt,
		ChooseAttributesUiView::AppearanceTextLineSpacing,
		FontLibrary::getInstance());

	auto &textureManager = game.textureManager;
	auto &renderer = game.renderer;
	const Surface surface = TextureUtils::generate(
		ChooseAttributesUiView::AppearanceTextPatternType,
		ChooseAttributesUiView::getAppearanceTextBoxTextureWidth(textBoxInitInfo.rect.width),
		ChooseAttributesUiView::getAppearanceTextBoxTextureHeight(textBoxInitInfo.rect.height),
		textureManager,
		renderer);

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTextureFromSurface(surface, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create bonus points remaining pop-up texture.");
	}

	ScopedUiTextureRef textureRef(textureID, renderer);

	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text, ChooseAttributesUiController::onBonusPointsRemainingTextBoxSelected,
		std::move(textureRef), ChooseAttributesUiView::AppearanceTextCenterPoint);
}

void ChooseAttributesUiController::onRerollButtonSelected(Game &game)
{	
	game.popSubPanel();
	game.setPanel<ChooseAttributesPanel>();
}

void ChooseAttributesUiController::onBonusPointsRemainingTextBoxSelected(Game &game)
{
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
	const int oldPortraitIndex = charCreationState.portraitIndex;
	const int newPortraitIndex = incrementIndex ?
		((oldPortraitIndex == maxID) ? minID : (oldPortraitIndex + 1)) :
		((oldPortraitIndex == minID) ? maxID : (oldPortraitIndex - 1));

	charCreationState.portraitIndex = newPortraitIndex;
}

void ChooseAttributesUiController::onDoneButtonSelected(Game &game, int bonusPointsRemaining, bool *attributesAreSaved)
{
	if (*attributesAreSaved)
	{
		ChooseAttributesUiController::onSavedDoneButtonSelected(game);
	}
	else
	{
		ChooseAttributesUiController::onUnsavedDoneButtonSelected(game, bonusPointsRemaining, attributesAreSaved);
	}
}

void ChooseAttributesUiController::onPostCharacterCreationCinematicFinished(Game &game)
{
	// Initialize the game world panel.
	game.setPanel<GameWorldPanel>();

	const MusicDefinition *musicDef = MusicUtils::getRandomDungeonMusicDefinition(game.random);
	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing dungeon music.");
	}

	AudioManager &audioManager = game.audioManager;
	audioManager.setMusic(musicDef);
}
