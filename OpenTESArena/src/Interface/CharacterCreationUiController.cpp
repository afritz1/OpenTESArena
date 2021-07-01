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
#include "../Game/CardinalDirection.h"
#include "../Game/Game.h"
#include "../UI/TextBox.h"
#include "../World/SkyUtils.h"
#include "../WorldMap/LocationUtils.h"

#include "components/utilities/String.h"

void CharacterCreationUiController::onBackToMainMenuButtonSelected(Game &game)
{
	game.setCharacterCreationState(nullptr);
	game.setPanel<MainMenuPanel>();

	const MusicLibrary &musicLibrary = game.getMusicLibrary();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
		MusicDefinition::Type::MainMenu, game.getRandom());

	if (musicDef == nullptr)
	{
		DebugLogWarning("Missing main menu music.");
	}

	AudioManager &audioManager = game.getAudioManager();
	audioManager.setMusic(musicDef);
}

void CharacterCreationUiController::onGenerateClassButtonSelected(Game &game)
{
	// @todo: eventually go to a "ChooseQuestionsPanel" with "pop-up" message
}

void CharacterCreationUiController::onSelectClassButtonSelected(Game &game)
{
	game.setPanel<ChooseClassPanel>();
}

void CharacterCreationUiController::onBackToChooseClassCreationButtonSelected(Game &game)
{
	game.setPanel<ChooseClassCreationPanel>();
}

void CharacterCreationUiController::onChooseClassListBoxUpButtonSelected(ListBox &listBox)
{
	listBox.scrollUp();
}

void CharacterCreationUiController::onChooseClassListBoxDownButtonSelected(ListBox &listBox)
{
	listBox.scrollDown();
}

void CharacterCreationUiController::onChooseClassListBoxItemButtonSelected(Game &game, int charClassDefID)
{
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setClassDefID(charClassDefID);

	game.setPanel<ChooseNamePanel>();
}

void CharacterCreationUiController::onBackToChooseNameButtonSelected(Game &game)
{
	game.setPanel<ChooseNamePanel>();
}

void CharacterCreationUiController::onChooseGenderMaleButtonSelected(Game &game)
{
	constexpr bool male = true;
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setGender(male);

	game.setPanel<ChooseRacePanel>();
}

void CharacterCreationUiController::onChooseGenderFemaleButtonSelected(Game &game)
{
	constexpr bool male = false;
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setGender(male);

	game.setPanel<ChooseRacePanel>();
}

void CharacterCreationUiController::onBackToChooseClassButtonSelected(Game &game)
{
	SDL_StopTextInput();

	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setName(nullptr);

	game.setPanel<ChooseClassPanel>();
}

void CharacterCreationUiController::onChooseNameAcceptButtonSelected(Game &game, const std::string &acceptedName)
{
	SDL_StopTextInput();

	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setName(acceptedName.c_str());

	game.setPanel<ChooseGenderPanel>();
}

void CharacterCreationUiController::onBackToChooseGenderButtonSelected(Game &game)
{
	game.setPanel<ChooseGenderPanel>();
}

void CharacterCreationUiController::onChooseRaceInitialPopUpButtonSelected(Game &game)
{
	game.popSubPanel();
}

void CharacterCreationUiController::onChooseRaceProvinceButtonSelected(Game &game, int raceID)
{
	// Set character creation race.
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setRaceIndex(raceID);

	// Generate the race selection message box.
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

	const auto &fontLibrary = game.getFontLibrary();
	const std::string titleText = CharacterCreationUiModel::getChooseRaceProvinceConfirmTitleText(game);
	const TextBox::InitInfo titleTextBoxInitInfo =
		CharacterCreationUiView::getChooseRaceProvinceConfirmTitleTextBoxInitInfo(titleText, fontLibrary);

	// @todo: MessageBoxSubPanel feels over-specified in general. It should be really easy to pass values to it like
	// title text, button count, button font, button alignment, and it figures out everything behind the scenes.
	MessageBoxSubPanel::Title messageBoxTitle;
	if (!messageBoxTitle.textBox.init(titleTextBoxInitInfo, renderer))
	{
		DebugCrash("Couldn't init province confirm title text box.");
	}

	messageBoxTitle.textBox.setText(titleText);

	const Rect &titleTextBoxRect = messageBoxTitle.textBox.getRect();
	const Rect titleTextureRect = CharacterCreationUiView::getChooseRaceProvinceConfirmTitleTextureRect(
		titleTextBoxRect.getWidth(), titleTextBoxRect.getHeight());
	messageBoxTitle.texture = TextureUtils::generate(
		CharacterCreationUiView::ChooseRaceProvinceConfirmTitleTexturePatternType,
		titleTextureRect.getWidth(),
		titleTextureRect.getHeight(),
		textureManager,
		renderer);
	messageBoxTitle.textureX = titleTextureRect.getLeft();
	messageBoxTitle.textureY = titleTextureRect.getTop();

	const std::string yesText = CharacterCreationUiModel::getChooseRaceProvinceConfirmYesText(game);
	const TextBox::InitInfo yesTextBoxInitInfo =
		CharacterCreationUiView::getChooseRaceProvinceConfirmYesTextBoxInitInfo(yesText, fontLibrary);

	MessageBoxSubPanel::Element messageBoxYes;
	if (!messageBoxYes.textBox.init(yesTextBoxInitInfo, renderer))
	{
		DebugCrash("Couldn't init province confirm yes text box.");
	}

	messageBoxYes.textBox.setText(yesText);

	const Rect yesTextureRect = CharacterCreationUiView::getChooseRaceProvinceConfirmYesTextureRect(titleTextureRect);
	messageBoxYes.texture = TextureUtils::generate(
		CharacterCreationUiView::ChooseRaceProvinceConfirmYesTexturePatternType,
		yesTextureRect.getWidth(),
		yesTextureRect.getHeight(),
		textureManager,
		renderer);

	messageBoxYes.function = [raceID](Game &game)
	{
		CharacterCreationUiController::onChooseRaceProvinceConfirmButtonSelected(game, raceID);
	};

	messageBoxYes.textureX = yesTextureRect.getLeft();
	messageBoxYes.textureY = yesTextureRect.getTop();

	const std::string noText = CharacterCreationUiModel::getChooseRaceProvinceConfirmNoText(game);
	const TextBox::InitInfo noTextBoxInitInfo =
		CharacterCreationUiView::getChooseRaceProvinceConfirmNoTextBoxInitInfo(noText, fontLibrary);

	MessageBoxSubPanel::Element messageBoxNo;
	if (!messageBoxNo.textBox.init(noTextBoxInitInfo, renderer))
	{
		DebugCrash("Couldn't init province confirm no text box.");
	}

	messageBoxNo.textBox.setText(noText);

	const Rect noTextureRect = CharacterCreationUiView::getChooseRaceProvinceConfirmNoTextureRect(yesTextureRect);
	messageBoxNo.texture = TextureUtils::generate(
		CharacterCreationUiView::ChooseRaceProvinceConfirmNoTexturePatternType,
		noTextureRect.getWidth(),
		noTextureRect.getHeight(),
		textureManager,
		renderer);

	messageBoxNo.function = CharacterCreationUiController::onChooseRaceProvinceCancelButtonSelected;
	messageBoxNo.textureX = noTextureRect.getLeft();
	messageBoxNo.textureY = noTextureRect.getTop();

	auto cancelFunction = messageBoxNo.function;

	std::vector<MessageBoxSubPanel::Element> messageBoxElements;
	messageBoxElements.emplace_back(std::move(messageBoxYes));
	messageBoxElements.emplace_back(std::move(messageBoxNo));

	game.pushSubPanel<MessageBoxSubPanel>(std::move(messageBoxTitle), std::move(messageBoxElements), cancelFunction);
}

void CharacterCreationUiController::onChooseRaceProvinceConfirmButtonSelected(Game &game, int raceID)
{
	game.popSubPanel();

	const std::string text = CharacterCreationUiModel::getChooseRaceProvinceConfirmedFirstText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextLineSpacing,
		game.getFontLibrary());

	const Rect textureRect = CharacterCreationUiView::getChooseRaceProvinceConfirmedFirstTextureRect(
		textBoxInitInfo.rect.getWidth(), textBoxInitInfo.rect.getHeight());
	Texture texture = TextureUtils::generate(CharacterCreationUiView::ChooseRaceProvinceConfirmedFirstTextPatternType,
		textureRect.getWidth(), textureRect.getHeight(), game.getTextureManager(), game.getRenderer());

	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text,
		CharacterCreationUiController::onChooseRaceProvinceConfirmedFirstButtonSelected,
		std::move(texture), textureRect.getCenter());
}

void CharacterCreationUiController::onChooseRaceProvinceCancelButtonSelected(Game &game)
{
	game.popSubPanel();

	// Push the initial text sub-panel.
	std::unique_ptr<Panel> textSubPanel = ChooseRacePanel::getInitialSubPanel(game);
	game.pushSubPanel(std::move(textSubPanel));
}

void CharacterCreationUiController::onChooseRaceProvinceConfirmedFirstButtonSelected(Game &game)
{
	game.popSubPanel();

	const std::string text = CharacterCreationUiModel::getChooseRaceProvinceConfirmedSecondText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextLineSpacing,
		game.getFontLibrary());

	const Rect textureRect = CharacterCreationUiView::getChooseRaceProvinceConfirmedSecondTextureRect(
		textBoxInitInfo.rect.getWidth(), textBoxInitInfo.rect.getHeight());
	Texture texture = TextureUtils::generate(CharacterCreationUiView::ChooseRaceProvinceConfirmedSecondTextPatternType,
		textureRect.getWidth(), textureRect.getHeight(), game.getTextureManager(), game.getRenderer());

	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text,
		CharacterCreationUiController::onChooseRaceProvinceConfirmedSecondButtonSelected,
		std::move(texture), textureRect.getCenter());
}

void CharacterCreationUiController::onChooseRaceProvinceConfirmedSecondButtonSelected(Game &game)
{
	game.popSubPanel();

	const std::string text = CharacterCreationUiModel::getChooseRaceProvinceConfirmedThirdText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextLineSpacing,
		game.getFontLibrary());

	const Rect textureRect = CharacterCreationUiView::getChooseRaceProvinceConfirmedThirdTextureRect(
		textBoxInitInfo.rect.getWidth(), textBoxInitInfo.rect.getHeight());
	Texture texture = TextureUtils::generate(CharacterCreationUiView::ChooseRaceProvinceConfirmedThirdTextPatternType,
		textureRect.getWidth(), textureRect.getHeight(), game.getTextureManager(), game.getRenderer());

	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text,
		CharacterCreationUiController::onChooseRaceProvinceConfirmedThirdButtonSelected,
		std::move(texture), textureRect.getCenter());
}

void CharacterCreationUiController::onChooseRaceProvinceConfirmedThirdButtonSelected(Game &game)
{
	game.popSubPanel();

	const std::string text = CharacterCreationUiModel::getChooseRaceProvinceConfirmedFourthText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextCenterPoint,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextFontName,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextColor,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextAlignment,
		std::nullopt,
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextLineSpacing,
		game.getFontLibrary());

	const Rect textureRect = CharacterCreationUiView::getChooseRaceProvinceConfirmedFourthTextureRect(
		textBoxInitInfo.rect.getWidth(), textBoxInitInfo.rect.getHeight());
	Texture texture = TextureUtils::generate(
		CharacterCreationUiView::ChooseRaceProvinceConfirmedFourthTextPatternType,
		textureRect.getWidth(),
		textureRect.getHeight(),
		game.getTextureManager(),
		game.getRenderer());

	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text,
		CharacterCreationUiController::onChooseRaceProvinceConfirmedFourthButtonSelected,
		std::move(texture), textureRect.getCenter());
}

void CharacterCreationUiController::onChooseRaceProvinceConfirmedFourthButtonSelected(Game &game)
{
	game.popSubPanel();
	game.setPanel<ChooseAttributesPanel>();
}

void CharacterCreationUiController::onBackToRaceSelectionButtonSelected(Game &game)
{
	game.setPanel<ChooseRacePanel>();
}

void CharacterCreationUiController::onChooseAttributesPopUpSelected(Game &game)
{
	game.popSubPanel();
}

void CharacterCreationUiController::onUnsavedAttributesDoneButtonSelected(Game &game, bool *attributesAreSaved)
{
	// Show message box to save or reroll.
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();

	// @todo: add InitInfos for this Save/Reroll message box sub-panel
	const auto &fontLibrary = game.getFontLibrary();
	const std::string titleText = CharacterCreationUiModel::getAttributesMessageBoxTitleText(game);
	const TextBox::InitInfo titleInitInfo =
		CharacterCreationUiView::getChooseAttributesUnsavedDoneTitleTextBoxInitInfo(titleText, fontLibrary);

	// @todo: MessageBoxSubPanel feels over-specified in general. It should be really easy to pass values to it like
	// title text, button count, button font, button alignment, and it figures out everything behind the scenes.
	MessageBoxSubPanel::Title messageBoxTitle;
	if (!messageBoxTitle.textBox.init(titleInitInfo, renderer))
	{
		DebugCrash("Couldn't init choose attributes unsaved done title text box.");
	}

	messageBoxTitle.textBox.setText(titleText);

	const Rect &titleTextBoxRect = messageBoxTitle.textBox.getRect();
	const Rect titleTextureRect = CharacterCreationUiView::getChooseAttributesUnsavedDoneTitleTextureRect(
		titleTextBoxRect.getWidth(), titleTextBoxRect.getHeight());
	messageBoxTitle.texture = TextureUtils::generate(
		CharacterCreationUiView::AttributesMessageBoxPatternType,
		titleTextureRect.getWidth(),
		titleTextureRect.getHeight(),
		textureManager,
		renderer);
	messageBoxTitle.textureX = titleTextureRect.getLeft();
	messageBoxTitle.textureY = titleTextureRect.getTop();

	const std::string saveText = CharacterCreationUiModel::getAttributesMessageBoxSaveText(game);
	const TextBox::InitInfo saveTextBoxInitInfo =
		CharacterCreationUiView::getChooseAttributesUnsavedDoneSaveTextBoxInitInfo(saveText, fontLibrary);

	MessageBoxSubPanel::Element messageBoxSave;
	if (!messageBoxSave.textBox.init(saveTextBoxInitInfo, renderer))
	{
		DebugCrash("Couldn't init choose attributes unsaved done save text box.");
	}

	messageBoxSave.textBox.setText(saveText);

	const Rect &saveTextBoxRect = messageBoxSave.textBox.getRect();
	const Rect saveTextureRect = CharacterCreationUiView::getChooseAttributesUnsavedDoneSaveTextureRect(titleTextureRect);
	messageBoxSave.texture = TextureUtils::generate(
		CharacterCreationUiView::AttributesMessageBoxPatternType,
		saveTextureRect.getWidth(),
		saveTextureRect.getHeight(),
		textureManager,
		renderer);

	messageBoxSave.function = [attributesAreSaved](Game &game)
	{
		CharacterCreationUiController::onSaveAttributesButtonSelected(game, attributesAreSaved);
	};

	messageBoxSave.textureX = saveTextureRect.getLeft();
	messageBoxSave.textureY = saveTextureRect.getTop();

	const std::string rerollText = CharacterCreationUiModel::getAttributesMessageBoxRerollText(game);
	const TextBox::InitInfo rerollTextBoxInitInfo =
		CharacterCreationUiView::getChooseAttributesUnsavedDoneRerollTextBoxInitInfo(rerollText, fontLibrary);

	MessageBoxSubPanel::Element messageBoxReroll;
	if (!messageBoxReroll.textBox.init(rerollTextBoxInitInfo, renderer))
	{
		DebugCrash("Couldn't init choose attributes unsaved done reroll text box.");
	}

	messageBoxReroll.textBox.setText(rerollText);

	const Rect &rerollTextBoxRect = messageBoxReroll.textBox.getRect();
	const Rect rerollTextureRect = CharacterCreationUiView::getChooseAttributesUnsavedDoneRerollTextureRect(saveTextureRect);
	messageBoxReroll.texture = TextureUtils::generate(
		CharacterCreationUiView::AttributesMessageBoxPatternType,
		rerollTextureRect.getWidth(),
		rerollTextureRect.getHeight(),
		textureManager,
		renderer);
	messageBoxReroll.function = CharacterCreationUiController::onRerollAttributesButtonSelected;
	messageBoxReroll.textureX = rerollTextureRect.getLeft();
	messageBoxReroll.textureY = rerollTextureRect.getTop();

	auto cancelFunction = messageBoxReroll.function;

	// Push message box sub panel.
	std::vector<MessageBoxSubPanel::Element> messageBoxElements;
	messageBoxElements.emplace_back(std::move(messageBoxSave));
	messageBoxElements.emplace_back(std::move(messageBoxReroll));

	game.pushSubPanel<MessageBoxSubPanel>(std::move(messageBoxTitle), std::move(messageBoxElements), cancelFunction);
}

void CharacterCreationUiController::onSavedAttributesDoneButtonSelected(Game &game)
{
	auto gameStateFunction = [](Game &game)
	{
		// Initialize 3D renderer.
		auto &renderer = game.getRenderer();
		const auto &options = game.getOptions();
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		const bool fullGameWindow = options.getGraphics_ModernInterface();
		renderer.initializeWorldRendering(
			options.getGraphics_ResolutionScale(),
			fullGameWindow,
			options.getGraphics_RenderThreadsMode());

		std::unique_ptr<GameState> gameState = [&game, &renderer, &binaryAssetLibrary]()
		{
			const auto &exeData = binaryAssetLibrary.getExeData();

			// Initialize player data (independent of the world).
			Player player = [&game, &exeData]()
			{
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

				const int portraitIndex = charCreationState.getPortraitIndex();

				const int allowedWeaponCount = charClassDef.getAllowedWeaponCount();
				const int weaponID = charClassDef.getAllowedWeapon(game.getRandom().next(allowedWeaponCount));

				return Player(std::string(name), male, raceIndex, charClassDefID, portraitIndex, dummyPosition,
					direction, velocity, Player::DEFAULT_WALK_SPEED, Player::DEFAULT_RUN_SPEED, weaponID, exeData);
			}();

			return std::make_unique<GameState>(std::move(player), binaryAssetLibrary);
		}();

		// Find starting dungeon location definition.
		const int provinceIndex = LocationUtils::CENTER_PROVINCE_ID;
		const WorldMapDefinition &worldMapDef = gameState->getWorldMapDefinition();
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
		if (!gameState->trySetInterior(interiorGenInfo, playerStartOffset, worldMapLocationIDs,
			game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
			game.getBinaryAssetLibrary(), game.getTextureManager(), game.getRenderer()))
		{
			DebugCrash("Couldn't load start dungeon \"" + mifName + "\".");
		}

		// Set the game state before constructing the game world panel.
		game.setGameState(std::move(gameState));
	};

	gameStateFunction(game);

	const auto &cinematicLibrary = game.getCinematicLibrary();
	int textCinematicDefIndex;
	const bool success = cinematicLibrary.findTextDefinitionIndexIf(
		[](const TextCinematicDefinition &def)
	{
		if (def.getType() == TextCinematicDefinition::Type::MainQuest)
		{
			const auto &mainQuestCinematicDef = def.getMainQuestDefinition();
			const bool isMainQuestStartCinematic = mainQuestCinematicDef.progress == 0;
			if (isMainQuestStartCinematic)
			{
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
	game.setPanel<TextCinematicPanel>(
		textCinematicDefIndex,
		0.171,
		CharacterCreationUiController::onPostCharacterCreationCinematicFinished);

	// Play dream music.
	const MusicLibrary &musicLibrary = game.getMusicLibrary();
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

void CharacterCreationUiController::onSaveAttributesButtonSelected(Game &game, bool *attributesAreSaved)
{
	// Confirming the chosen stats will bring up a text sub-panel, and the next time the done button is clicked,
	// it starts the game.
	game.popSubPanel();

	const std::string text = CharacterCreationUiModel::getAppearanceMessageBoxText(game);
	const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
		text,
		CharacterCreationUiView::AppearanceMessageBoxCenterPoint,
		CharacterCreationUiView::AppearanceMessageBoxFontName,
		CharacterCreationUiView::AppearanceMessageBoxColor,
		CharacterCreationUiView::AppearanceMessageBoxAlignment,
		std::nullopt,
		CharacterCreationUiView::AppearanceMessageBoxLineSpacing,
		game.getFontLibrary());

	Texture texture = TextureUtils::generate(
		CharacterCreationUiView::AppearanceMessageBoxPatternType,
		CharacterCreationUiView::getAppearanceMessageBoxTextureWidth(textBoxInitInfo.rect.getWidth()),
		CharacterCreationUiView::getAppearanceMessageBoxTextureHeight(textBoxInitInfo.rect.getHeight()),
		game.getTextureManager(),
		game.getRenderer());

	// The done button is replaced after the player confirms their stats, and it then leads to the main quest
	// opening cinematic.
	game.pushSubPanel<TextSubPanel>(textBoxInitInfo, text,
		CharacterCreationUiController::onAppearanceMessageBoxSelected,
		std::move(texture), CharacterCreationUiView::AppearanceMessageBoxCenterPoint);

	*attributesAreSaved = true;
}

void CharacterCreationUiController::onRerollAttributesButtonSelected(Game &game)
{
	// @todo: reroll attributes.
	game.popSubPanel();
}

void CharacterCreationUiController::onAppearanceMessageBoxSelected(Game &game)
{
	game.popSubPanel();
}

void CharacterCreationUiController::onAppearancePortraitButtonSelected(Game &game, bool incrementIndex)
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

void CharacterCreationUiController::onAttributesDoneButtonSelected(Game &game, bool *attributesAreSaved)
{
	if (*attributesAreSaved)
	{
		CharacterCreationUiController::onSavedAttributesDoneButtonSelected(game);
	}
	else
	{
		CharacterCreationUiController::onUnsavedAttributesDoneButtonSelected(game, attributesAreSaved);
	}
}

void CharacterCreationUiController::onPostCharacterCreationCinematicFinished(Game &game)
{
	// Create the function that will be called when the player leaves the starting dungeon.
	// @todo: this should be in a game logic controller namespace, not UI controller.
	auto onLevelUpVoxelEnter = [](Game &game)
	{
		// Teleport the player to a random location based on their race.
		auto &gameState = game.getGameState();
		auto &player = gameState.getPlayer();
		player.setVelocityToZero();

		const int provinceID = gameState.getPlayer().getRaceID();
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
			std::copy(cityReservedBlocks->begin(), cityReservedBlocks->end(), buffer.get());
			return buffer;
		}();

		const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
			[&cityDef]() ->std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
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
			game.getBinaryAssetLibrary(), game.getTextAssetLibrary(), game.getTextureManager(),
			renderer))
		{
			DebugCrash("Couldn't load city \"" + locationDef.getName() + "\".");
		}

		// Set music based on weather and time.
		const MusicDefinition *musicDef = [&game, &gameState]()
		{
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
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

	// Initialize the game world panel.
	game.setPanel<GameWorldPanel>();

	// Choose random dungeon music.
	const MusicLibrary &musicLibrary = game.getMusicLibrary();
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
