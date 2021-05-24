#include "CharacterCreationUiController.h"
#include "CharacterCreationUiModel.h"
#include "CharacterCreationUiView.h"
#include "ChooseClassCreationPanel.h"
#include "ChooseClassPanel.h"
#include "ChooseNamePanel.h"
#include "ChooseRacePanel.h"
#include "GameWorldPanel.h"
#include "MainMenuPanel.h"
#include "MessageBoxSubPanel.h"
#include "TextCinematicPanel.h"
#include "TextSubPanel.h"
#include "../Game/CardinalDirection.h"
#include "../Game/Game.h"
#include "../World/SkyUtils.h"
#include "../WorldMap/LocationUtils.h"

#include "components/utilities/String.h"

void CharacterCreationUiController::onBackToMainMenuButtonSelected(Game &game)
{
	game.setCharacterCreationState(nullptr);
	game.setPanel<MainMenuPanel>(game);

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
	game.setPanel<ChooseClassPanel>(game);
}

void CharacterCreationUiController::onBackToChooseClassCreationButtonSelected(Game &game)
{
	game.setPanel<ChooseClassCreationPanel>(game);
}

void CharacterCreationUiController::onChooseClassListBoxUpButtonSelected(ListBox &listBox)
{
	// Scroll the list box up one if able.
	if (listBox.getScrollIndex() > 0)
	{
		listBox.scrollUp();
	}
}

void CharacterCreationUiController::onChooseClassListBoxDownButtonSelected(ListBox &listBox)
{
	// Scroll the list box down one if able.
	const int scrollIndex = listBox.getScrollIndex();
	const int elementCount = listBox.getElementCount();
	const int maxDisplayedCount = listBox.getMaxDisplayedCount();
	if (scrollIndex < (elementCount - maxDisplayedCount))
	{
		listBox.scrollDown();
	}
}

void CharacterCreationUiController::onChooseClassListBoxAcceptButtonSelected(Game &game, int charClassDefID)
{
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setClassDefID(charClassDefID);

	game.setPanel<ChooseNamePanel>(game);
}

void CharacterCreationUiController::onBackToChooseNameButtonSelected(Game &game)
{
	game.setPanel<ChooseNamePanel>(game);
}

void CharacterCreationUiController::onChooseGenderMaleButtonSelected(Game &game)
{
	constexpr bool male = true;
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setGender(male);

	game.setPanel<ChooseRacePanel>(game);
}

void CharacterCreationUiController::onChooseGenderFemaleButtonSelected(Game &game)
{
	constexpr bool male = false;
	auto &charCreationState = game.getCharacterCreationState();
	charCreationState.setGender(male);

	game.setPanel<ChooseRacePanel>(game);
}

void CharacterCreationUiController::onBackToRaceSelectionButtonSelected(Game &game)
{
	game.setPanel<ChooseRacePanel>(game);
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

	// @todo: MessageBoxSubPanel feels over-specified in general. It should be really easy to pass values to it like
	// title text, button count, button font, button alignment, and it figures out everything behind the scenes.
	MessageBoxSubPanel::Title messageBoxTitle;
	messageBoxTitle.textBox = [&game, &renderer]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getAttributesMessageBoxTitleText(game),
			CharacterCreationUiView::AttributesMessageBoxTitleFontName,
			CharacterCreationUiView::AttributesMessageBoxTitleColor,
			CharacterCreationUiView::AttributesMessageBoxTitleAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::AttributesMessageBoxCenterPoint,
			richText,
			fontLibrary,
			renderer);
	}();

	messageBoxTitle.texture = TextureUtils::generate(
		CharacterCreationUiView::AttributesMessageBoxPatternType,
		CharacterCreationUiView::getAttributesMessageBoxTitleTextureWidth(messageBoxTitle.textBox->getRect().getWidth()),
		CharacterCreationUiView::getAttributesMessageBoxTitleTextureHeight(),
		textureManager,
		renderer);

	messageBoxTitle.textureX = CharacterCreationUiView::getAttributesMessageBoxTitleTextureX(messageBoxTitle.texture.getWidth());
	messageBoxTitle.textureY = CharacterCreationUiView::getAttributesMessageBoxTitleTextureY(messageBoxTitle.texture.getHeight());

	MessageBoxSubPanel::Element messageBoxSave;
	messageBoxSave.textBox = [&game, &renderer]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getAttributesMessageBoxSaveText(game),
			CharacterCreationUiView::AttributesMessageBoxSaveFontName,
			CharacterCreationUiView::AttributesMessageBoxSaveColor,
			CharacterCreationUiView::AttributesMessageBoxSaveAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::AttributesMessageBoxSaveCenterPoint,
			richText,
			fontLibrary,
			renderer);
	}();

	// @todo: this is over-specified -- need MessageBoxSubPanel to do a lot of this automatically for its elements.
	messageBoxSave.texture = TextureUtils::generate(
		CharacterCreationUiView::AttributesMessageBoxPatternType,
		messageBoxTitle.texture.getWidth(),
		messageBoxTitle.texture.getHeight(),
		textureManager,
		renderer);

	messageBoxSave.function = [attributesAreSaved](Game &game)
	{
		CharacterCreationUiController::onSaveAttributesButtonSelected(game, attributesAreSaved);
	};

	messageBoxSave.textureX = messageBoxTitle.textureX;
	messageBoxSave.textureY = messageBoxTitle.textureY + messageBoxTitle.texture.getHeight();

	MessageBoxSubPanel::Element messageBoxReroll;
	messageBoxReroll.textBox = [&game, &renderer]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			CharacterCreationUiModel::getAttributesMessageBoxRerollText(game),
			CharacterCreationUiView::AttributesMessageBoxRerollFontName,
			CharacterCreationUiView::AttributesMessageBoxRerollColor,
			CharacterCreationUiView::AttributesMessageBoxRerollAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			CharacterCreationUiView::AttributesMessageBoxRerollCenterPoint,
			richText,
			fontLibrary,
			renderer);
	}();

	// @todo: this is over-specified -- need MessageBoxSubPanel to do a lot of this automatically for its elements.
	messageBoxReroll.texture = TextureUtils::generate(
		CharacterCreationUiView::AttributesMessageBoxPatternType,
		messageBoxSave.texture.getWidth(),
		messageBoxSave.texture.getHeight(),
		textureManager,
		renderer);

	messageBoxReroll.function = CharacterCreationUiController::onRerollAttributesButtonSelected;
	messageBoxReroll.textureX = messageBoxSave.textureX;
	messageBoxReroll.textureY = messageBoxSave.textureY + messageBoxSave.texture.getHeight();

	auto cancelFunction = messageBoxReroll.function;

	// Push message box sub panel.
	std::vector<MessageBoxSubPanel::Element> messageBoxElements;
	messageBoxElements.emplace_back(std::move(messageBoxSave));
	messageBoxElements.emplace_back(std::move(messageBoxReroll));

	auto messageBox = std::make_unique<MessageBoxSubPanel>(
		game,
		std::move(messageBoxTitle),
		std::move(messageBoxElements),
		cancelFunction);

	game.pushSubPanel(std::move(messageBox));
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

		std::unique_ptr<GameState> gameState = [&game, &binaryAssetLibrary]()
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
		game,
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

	const RichTextString richText(
		CharacterCreationUiModel::getAppearanceMessageBoxText(game),
		CharacterCreationUiView::AppearanceMessageBoxFontName,
		CharacterCreationUiView::AppearanceMessageBoxColor,
		CharacterCreationUiView::AppearanceMessageBoxAlignment,
		CharacterCreationUiView::AppearanceMessageBoxLineSpacing,
		game.getFontLibrary());

	Texture texture = TextureUtils::generate(
		CharacterCreationUiView::AppearanceMessageBoxPatternType,
		CharacterCreationUiView::getAppearanceMessageBoxTextureWidth(richText.getDimensions().x),
		CharacterCreationUiView::getAppearanceMessageBoxTextureHeight(richText.getDimensions().y),
		game.getTextureManager(),
		game.getRenderer());

	// The done button is replaced after the player confirms their stats, and it then leads to the main quest
	// opening cinematic.
	auto appearanceSubPanel = std::make_unique<TextSubPanel>(
		game,
		CharacterCreationUiView::AppearanceMessageBoxCenterPoint,
		richText,
		CharacterCreationUiController::onAppearanceMessageBoxSelected,
		std::move(texture),
		CharacterCreationUiView::AppearanceMessageBoxCenterPoint);
	game.pushSubPanel(std::move(appearanceSubPanel));

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
	auto gameWorldPanel = std::make_unique<GameWorldPanel>(game);
	game.setPanel(std::move(gameWorldPanel));

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
