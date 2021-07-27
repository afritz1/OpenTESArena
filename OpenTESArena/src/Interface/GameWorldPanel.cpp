#include <algorithm>
#include <cmath>

#include "SDL.h"

#include "GameWorldPanel.h"
#include "GameWorldUiController.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/Game.h"
#include "../GameLogic/MapLogicController.h"
#include "../GameLogic/PlayerLogicController.h"
#include "../Media/PortraitFile.h"
#include "../UI/CursorData.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"

GameWorldPanel::GameWorldPanel(Game &game)
	: Panel(game) { }

bool GameWorldPanel::init()
{
	auto &game = this->getGame();
	DebugAssert(game.gameStateIsActive());

	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();
	const std::string playerNameText = GameWorldUiModel::getPlayerNameText(game);
	const TextBox::InitInfo playerNameTextBoxInitInfo =
		GameWorldUiView::getPlayerNameTextBoxInitInfo(playerNameText, fontLibrary);
	if (!this->playerNameTextBox.init(playerNameTextBoxInitInfo, playerNameText, renderer))
	{
		DebugLogError("Couldn't init player name text box.");
		return false;
	}

	const TextBox::InitInfo triggerTextBoxInitInfo = GameWorldUiView::getTriggerTextBoxInitInfo(fontLibrary);
	if (!this->triggerText.init(triggerTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init trigger text box.");
		return false;
	}

	const TextBox::InitInfo actionTextBoxInitInfo = GameWorldUiView::getActionTextBoxInitInfo(fontLibrary);
	if (!this->actionText.init(actionTextBoxInitInfo, renderer))
	{
		DebugLogError("Couldn't init action text box.");
		return false;
	}

	// @todo: effect text box initialization

	this->characterSheetButton = Button<Game&>(
		GameWorldUiView::CharacterSheetButtonX,
		GameWorldUiView::CharacterSheetButtonY,
		GameWorldUiView::CharacterSheetButtonWidth,
		GameWorldUiView::CharacterSheetButtonHeight,
		GameWorldUiController::onCharacterSheetButtonSelected);
	this->drawWeaponButton = Button<Player&>(
		GameWorldUiView::WeaponSheathButtonX,
		GameWorldUiView::WeaponSheathButtonY,
		GameWorldUiView::WeaponSheathButtonWidth,
		GameWorldUiView::WeaponSheathButtonHeight,
		GameWorldUiController::onWeaponButtonSelected);
	this->stealButton = Button<>(
		GameWorldUiView::StealButtonX,
		GameWorldUiView::StealButtonY,
		GameWorldUiView::StealButtonWidth,
		GameWorldUiView::StealButtonHeight,
		GameWorldUiController::onStealButtonSelected);
	this->statusButton = Button<Game&>(
		GameWorldUiView::StatusButtonX,
		GameWorldUiView::StatusButtonY,
		GameWorldUiView::StatusButtonWidth,
		GameWorldUiView::StatusButtonHeight,
		GameWorldUiController::onStatusButtonSelected);
	this->magicButton = Button<>(
		GameWorldUiView::MagicButtonX,
		GameWorldUiView::MagicButtonY,
		GameWorldUiView::MagicButtonWidth,
		GameWorldUiView::MagicButtonHeight,
		GameWorldUiController::onMagicButtonSelected);
	this->logbookButton = Button<Game&>(
		GameWorldUiView::LogbookButtonX,
		GameWorldUiView::LogbookButtonY,
		GameWorldUiView::LogbookButtonWidth,
		GameWorldUiView::LogbookButtonHeight,
		GameWorldUiController::onLogbookButtonSelected);
	this->useItemButton = Button<>(
		GameWorldUiView::UseItemButtonX,
		GameWorldUiView::UseItemButtonY,
		GameWorldUiView::UseItemButtonWidth,
		GameWorldUiView::UseItemButtonHeight,
		GameWorldUiController::onUseItemButtonSelected);
	this->campButton = Button<>(
		GameWorldUiView::CampButtonX,
		GameWorldUiView::CampButtonY,
		GameWorldUiView::CampButtonWidth,
		GameWorldUiView::CampButtonHeight,
		GameWorldUiController::onCampButtonSelected);
	this->scrollUpButton = Button<GameWorldPanel&>(
		GameWorldUiView::ScrollUpButtonX,
		GameWorldUiView::ScrollUpButtonY,
		GameWorldUiView::ScrollUpButtonWidth,
		GameWorldUiView::ScrollUpButtonHeight,
		GameWorldUiController::onScrollUpButtonSelected);
	this->scrollDownButton = Button<GameWorldPanel&>(
		GameWorldUiView::ScrollDownButtonX,
		GameWorldUiView::ScrollDownButtonY,
		GameWorldUiView::ScrollDownButtonWidth,
		GameWorldUiView::ScrollDownButtonHeight,
		GameWorldUiController::onScrollDownButtonSelected);
	this->pauseButton = Button<Game&>(GameWorldUiController::onPauseButtonSelected);
	this->mapButton = Button<Game&, bool>(
		GameWorldUiView::MapButtonX,
		GameWorldUiView::MapButtonY,
		GameWorldUiView::MapButtonWidth,
		GameWorldUiView::MapButtonHeight,
		GameWorldUiController::onMapButtonSelected);

	// Set all of the cursor regions relative to the current window.
	const Int2 screenDims = game.getRenderer().getWindowDimensions();
	GameWorldUiModel::updateNativeCursorRegions(
		BufferView<Rect>(this->nativeCursorRegions.data(), static_cast<int>(this->nativeCursorRegions.size())),
		screenDims.x, screenDims.y);

	// If in modern mode, lock mouse to center of screen for free-look.
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, true);
	}

	return true;
}

GameWorldPanel::~GameWorldPanel()
{
	// If in modern mode, disable free-look.
	auto &game = this->getGame();
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, false);
	}
}

std::optional<CursorData> GameWorldPanel::getCurrentCursor() const
{
	// The cursor texture depends on the current mouse position.
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();
	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	const Int2 mousePosition = game.getInputManager().getMousePosition();

	if (modernInterface)
	{
		// Do not show cursor in modern mode.
		return std::nullopt;
	}
	else
	{
		const std::string &paletteFilename = ArenaPaletteName::Default;
		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
		if (!paletteID.has_value())
		{
			DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
		}

		// See which arrow cursor region the native mouse is in.
		for (int i = 0; i < this->nativeCursorRegions.size(); i++)
		{
			if (this->nativeCursorRegions[i].contains(mousePosition))
			{
				// Get the relevant arrow cursor.
				const std::string &textureFilename = ArenaTextureName::ArrowCursors;
				const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
					textureManager.tryGetTextureBuilderIDs(textureFilename.c_str());
				if (!textureBuilderIDs.has_value())
				{
					DebugCrash("Couldn't get texture builder IDs for \"" + textureFilename + "\".");
				}

				const TextureBuilderID textureBuilderID = textureBuilderIDs->getID(i);

				DebugAssertIndex(GameWorldUiView::ArrowCursorAlignments, i);
				const CursorAlignment cursorAlignment = GameWorldUiView::ArrowCursorAlignments[i];
				return CursorData(textureBuilderID, *paletteID, cursorAlignment);
			}
		}

		// Not in any of the arrow regions.
		return this->getDefaultCursor();
	}
}

void GameWorldPanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	auto &options = game.getOptions();
	auto &player = game.getGameState().getPlayer();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool f4Pressed = inputManager.keyPressed(e, SDLK_F4);

	if (escapePressed)
	{
		this->pauseButton.click(game);
	}
	else if (f4Pressed)
	{
		// Increment or wrap profiler level.
		const int oldProfilerLevel = options.getMisc_ProfilerLevel();
		const int newProfilerLevel = (oldProfilerLevel < Options::MAX_PROFILER_LEVEL) ?
			(oldProfilerLevel + 1) : Options::MIN_PROFILER_LEVEL;
		options.setMisc_ProfilerLevel(newProfilerLevel);
	}

	// Listen for hotkeys.
	const bool drawWeaponHotkeyPressed = inputManager.keyPressed(e, SDLK_f);
	const bool automapHotkeyPressed = inputManager.keyPressed(e, SDLK_n);
	const bool logbookHotkeyPressed = inputManager.keyPressed(e, SDLK_l);
	const bool sheetHotkeyPressed = inputManager.keyPressed(e, SDLK_TAB) || inputManager.keyPressed(e, SDLK_F1);
	const bool statusHotkeyPressed = inputManager.keyPressed(e, SDLK_v);
	const bool worldMapHotkeyPressed = inputManager.keyPressed(e, SDLK_m);
	const bool toggleCompassHotkeyPressed = inputManager.keyPressed(e, SDLK_F8);

	if (drawWeaponHotkeyPressed)
	{
		this->drawWeaponButton.click(player);
	}
	else if (automapHotkeyPressed)
	{
		const bool goToAutomap = true;
		this->mapButton.click(game, goToAutomap);
	}
	else if (logbookHotkeyPressed)
	{
		this->logbookButton.click(game);
	}
	else if (sheetHotkeyPressed)
	{
		this->characterSheetButton.click(game);
	}
	else if (statusHotkeyPressed)
	{
		this->statusButton.click(game);
	}
	else if (worldMapHotkeyPressed)
	{
		const bool goToAutomap = false;
		this->mapButton.click(game, goToAutomap);
	}
	else if (toggleCompassHotkeyPressed)
	{
		// Toggle compass display.
		options.setMisc_ShowCompass(!options.getMisc_ShowCompass());
	}

	// Player's XY coordinate hotkey.
	const bool f2Pressed = inputManager.keyPressed(e, SDLK_F2);

	if (f2Pressed)
	{
		// Refresh player coordinates display (probably intended for debugging in the original game).
		// These coordinates are in Arena's coordinate system.
		const std::string text = GameWorldUiModel::getPlayerPositionText(game);
		this->actionText.setText(text);

		auto &gameState = game.getGameState();
		gameState.setActionTextDuration(text);
	}

	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	// @temp: hold this key down to make clicks cause voxels to fade.
	const bool debugFadeVoxel = inputManager.keyIsDown(SDL_SCANCODE_G);

	const auto &renderer = game.getRenderer();

	// Handle input events based on which player interface mode is active.
	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	if (!modernInterface)
	{
		// Get mouse position relative to letterbox coordinates.
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

		const Rect &centerCursorRegion = this->nativeCursorRegions.at(4);

		if (leftClick)
		{
			// Was an interface button clicked?
			if (this->characterSheetButton.contains(originalPosition))
			{
				this->characterSheetButton.click(game);
			}
			else if (this->drawWeaponButton.contains(originalPosition))
			{
				this->drawWeaponButton.click(player);
			}
			else if (this->mapButton.contains(originalPosition))
			{
				const bool goToAutomap = true;
				this->mapButton.click(game, goToAutomap);
			}
			else if (this->stealButton.contains(originalPosition))
			{
				this->stealButton.click();
			}
			else if (this->statusButton.contains(originalPosition))
			{
				this->statusButton.click(game);
			}
			else if (this->magicButton.contains(originalPosition))
			{
				this->magicButton.click();
			}
			else if (this->logbookButton.contains(originalPosition))
			{
				this->logbookButton.click(game);
			}
			else if (this->useItemButton.contains(originalPosition))
			{
				this->useItemButton.click();
			}
			else if (this->campButton.contains(originalPosition))
			{
				this->campButton.click();
			}
			else
			{
				// Check for left clicks in the game world.
				if (centerCursorRegion.contains(mousePosition))
				{
					const bool primaryClick = true;
					PlayerLogicController::handleClickInWorld(game, mousePosition, primaryClick, debugFadeVoxel, this->actionText);
				}
			}
		}
		else if (rightClick)
		{
			if (this->mapButton.contains(originalPosition))
			{
				this->mapButton.click(game, false);
			}
			else
			{
				// Check for right clicks in the game world.
				if (centerCursorRegion.contains(mousePosition))
				{
					const bool primaryClick = false;
					PlayerLogicController::handleClickInWorld(game, mousePosition, primaryClick, false, this->actionText);
				}
			}
		}
	}
	else
	{
		// Check modern mode input events.
		const bool ePressed = inputManager.keyPressed(e, SDLK_e);

		// Any clicks will be at the center of the window.
		const Int2 windowDims = renderer.getWindowDimensions();
		const Int2 nativeCenter = windowDims / 2;

		if (ePressed)
		{
			// Activate (left click in classic mode).
			const bool primaryClick = true;
			PlayerLogicController::handleClickInWorld(game, nativeCenter, primaryClick, debugFadeVoxel, this->actionText);
		}
		else if (leftClick)
		{
			// Read (right click in classic mode).
			const bool primaryClick = false;
			PlayerLogicController::handleClickInWorld(game, nativeCenter, primaryClick, false, this->actionText);
		}
	}
}

void GameWorldPanel::onPauseChanged(bool paused)
{
	auto &game = this->getGame();

	// If in modern mode, set free-look to the given value.
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, !paused);
	}
}

void GameWorldPanel::resize(int windowWidth, int windowHeight)
{
	// Update the cursor's regions for camera motion.
	GameWorldUiModel::updateNativeCursorRegions(
		BufferView<Rect>(this->nativeCursorRegions.data(), static_cast<int>(this->nativeCursorRegions.size())),
		windowWidth, windowHeight);
}

void GameWorldPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	auto &game = this->getGame();
	const Texture tooltip = TextureUtils::createTooltip(text, game.getFontLibrary(), renderer);	
	const Int2 tooltipPosition = GameWorldUiView::getTooltipPosition(game, tooltip.getHeight());
	renderer.drawOriginal(tooltip, tooltipPosition.x, tooltipPosition.y);
}

void GameWorldPanel::drawCompass(const VoxelDouble2 &direction, TextureManager &textureManager, Renderer &renderer)
{
	auto &game = this->getGame();

	// Visible part of the slider (based on player position)
	const TextureBuilderID compassSliderTextureBuilderID = GameWorldUiView::getCompassSliderTextureBuilderID(game);
	const TextureBuilder &compassSlider = textureManager.getTextureBuilderHandle(compassSliderTextureBuilderID);
	const Rect clipRect = GameWorldUiView::getCompassClipRect(game, direction, compassSlider.getHeight());
	const Int2 sliderPosition = GameWorldUiView::getCompassSliderPosition(clipRect.getWidth(), clipRect.getHeight());

	// @temp: since there are some off-by-one rounding errors with SDL_RenderCopy, draw a black rectangle behind the
	// slider to cover up gaps.
	renderer.fillOriginalRect(
		Color::Black,
		sliderPosition.x - 1,
		sliderPosition.y - 1,
		clipRect.getWidth() + 2,
		clipRect.getHeight() + 2);

	const TextureAssetReference paletteTextureAssetRef = GameWorldUiView::getCompassSliderPaletteTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginalClipped(compassSliderTextureBuilderID, *paletteID, clipRect,
		sliderPosition.x, sliderPosition.y, textureManager);

	// Draw the compass frame over the slider.
	const TextureBuilderID compassFrameTextureBuilderID = GameWorldUiView::getCompassFrameTextureBuilderID(game);
	const TextureBuilder &compassFrame = textureManager.getTextureBuilderHandle(compassFrameTextureBuilderID);
	const Int2 compassFramePosition = GameWorldUiView::getCompassFramePosition(compassFrame.getWidth());
	renderer.drawOriginal(compassFrameTextureBuilderID, *paletteID, compassFramePosition.x, compassFramePosition.y, textureManager);
}

void GameWorldPanel::tick(double dt)
{
	auto &game = this->getGame();
	DebugAssert(game.gameStateIsActive());

	// Get the relative mouse state.
	const auto &inputManager = game.getInputManager();
	const Int2 mouseDelta = inputManager.getMouseDelta();

	// Handle input for player motion.
	BufferViewReadOnly<Rect> nativeCursorRegionsView(
		this->nativeCursorRegions.data(), static_cast<int>(this->nativeCursorRegions.size()));
	PlayerLogicController::handlePlayerTurning(game, dt, mouseDelta, nativeCursorRegionsView);
	PlayerLogicController::handlePlayerMovement(game, dt, nativeCursorRegionsView);

	// Tick the game world clock time.
	auto &gameState = game.getGameState();
	const bool debugFastForwardClock = inputManager.keyIsDown(SDL_SCANCODE_R); // @todo: camp button
	const Clock oldClock = gameState.getClock();
	gameState.tick(debugFastForwardClock ? (dt * 250.0) : dt, game);
	const Clock newClock = gameState.getClock();

	Renderer &renderer = game.getRenderer();

	// See if the clock passed the boundary between night and day, and vice versa.
	const double oldClockTime = oldClock.getPreciseTotalSeconds();
	const double newClockTime = newClock.getPreciseTotalSeconds();
	const double lamppostActivateTime = ArenaClockUtils::LamppostActivate.getPreciseTotalSeconds();
	const double lamppostDeactivateTime = ArenaClockUtils::LamppostDeactivate.getPreciseTotalSeconds();
	const bool activateNightLights =
		(oldClockTime < lamppostActivateTime) &&
		(newClockTime >= lamppostActivateTime);
	const bool deactivateNightLights =
		(oldClockTime < lamppostDeactivateTime) &&
		(newClockTime >= lamppostDeactivateTime);

	if (activateNightLights)
	{
		MapLogicController::handleNightLightChange(game, true);
	}
	else if (deactivateNightLights)
	{
		MapLogicController::handleNightLightChange(game, false);
	}

	const MapDefinition &mapDef = gameState.getActiveMapDef();
	const MapType mapType = mapDef.getMapType();

	// Check for changes in exterior music depending on the time.
	if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
	{
		const double dayMusicStartTime = ArenaClockUtils::MusicSwitchToDay.getPreciseTotalSeconds();
		const double nightMusicStartTime = ArenaClockUtils::MusicSwitchToNight.getPreciseTotalSeconds();
		const bool changeToDayMusic = (oldClockTime < dayMusicStartTime) && (newClockTime >= dayMusicStartTime);
		const bool changeToNightMusic = (oldClockTime < nightMusicStartTime) && (newClockTime >= nightMusicStartTime);

		AudioManager &audioManager = game.getAudioManager();
		const MusicLibrary &musicLibrary = game.getMusicLibrary();

		if (changeToDayMusic)
		{
			const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicDefinition::Type::Weather, game.getRandom(), [&weatherDef](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Weather);
				const auto &weatherMusicDef = def.getWeatherMusicDefinition();
				return weatherMusicDef.weatherDef == weatherDef;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing weather music.");
			}

			audioManager.setMusic(musicDef);
		}
		else if (changeToNightMusic)
		{
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinition(
				MusicDefinition::Type::Night, game.getRandom());

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing night music.");
			}

			audioManager.setMusic(musicDef);
		}
	}

	// Tick the player.
	auto &player = gameState.getPlayer();
	const CoordDouble3 oldPlayerCoord = player.getPosition();
	player.tick(game, dt);
	const CoordDouble3 newPlayerCoord = player.getPosition();

	// Handle input for the player's attack.
	PlayerLogicController::handlePlayerAttack(game, mouseDelta);

	MapInstance &mapInst = gameState.getActiveMapInst();
	const double latitude = [&gameState]()
	{
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		return locationDef.getLatitude();
	}();

	const EntityDefinitionLibrary &entityDefLibrary = game.getEntityDefinitionLibrary();
	TextureManager &textureManager = game.getTextureManager();

	EntityGeneration::EntityGenInfo entityGenInfo;
	entityGenInfo.init(gameState.nightLightsAreActive());

	// Tick active map (entities, animated distant land, etc.).
	const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo = [&game, &gameState, mapType,
		&entityDefLibrary, &textureManager]() -> std::optional<CitizenUtils::CitizenGenInfo>
	{
		if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
		{
			const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
			const LocationDefinition &locationDef = gameState.getLocationDefinition();
			const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();
			return CitizenUtils::makeCitizenGenInfo(provinceDef.getRaceID(), cityDef.climateType,
				entityDefLibrary, textureManager);
		}
		else
		{
			return std::nullopt;
		}
	}();

	mapInst.update(dt, game, newPlayerCoord, mapDef, latitude, gameState.getDaytimePercent(),
		game.getOptions().getMisc_ChunkDistance(), entityGenInfo, citizenGenInfo, entityDefLibrary,
		game.getBinaryAssetLibrary(), textureManager, game.getAudioManager());

	// See if the player changed voxels in the XZ plane. If so, trigger text and sound events,
	// and handle any level transition.
	const LevelInstance &levelInst = mapInst.getActiveLevel();
	const double ceilingScale = levelInst.getCeilingScale();
	const CoordInt3 oldPlayerVoxelCoord(
		oldPlayerCoord.chunk, VoxelUtils::pointToVoxel(oldPlayerCoord.point, ceilingScale));
	const CoordInt3 newPlayerVoxelCoord(
		newPlayerCoord.chunk, VoxelUtils::pointToVoxel(newPlayerCoord.point, ceilingScale));
	if (newPlayerVoxelCoord != oldPlayerVoxelCoord)
	{
		MapLogicController::handleTriggers(game, newPlayerVoxelCoord, this->triggerText);

		if (mapType == MapType::Interior)
		{
			MapLogicController::handleLevelTransition(game, oldPlayerVoxelCoord, newPlayerVoxelCoord);
		}
	}
}

void GameWorldPanel::render(Renderer &renderer)
{
	DebugAssert(this->getGame().gameStateIsActive());

	// Clear full screen.
	renderer.clear();

	// Draw game world onto the native frame buffer. The game world buffer
	// might not completely fill up the native buffer (bottom corners), so
	// clearing the native buffer beforehand is still necessary.
	auto &game = this->getGame();
	auto &gameState = game.getGameState();
	auto &player = gameState.getPlayer();
	const MapDefinition &activeMapDef = gameState.getActiveMapDef();
	const MapInstance &activeMapInst = gameState.getActiveMapInst();
	const LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();
	const SkyInstance &activeSkyInst = activeMapInst.getActiveSky();
	const WeatherInstance &activeWeatherInst = gameState.getWeatherInstance();
	const auto &options = game.getOptions();
	const double ambientPercent = gameState.getAmbientPercent();

	const double latitude = [&gameState]()
	{
		const LocationDefinition &locationDef = gameState.getLocationDefinition();
		return locationDef.getLatitude();
	}();

	const bool isExterior = activeMapDef.getMapType() != MapType::Interior;

	auto &textureManager = game.getTextureManager();
	const std::string &defaultPaletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> defaultPaletteID = textureManager.tryGetPaletteID(defaultPaletteFilename.c_str());
	if (!defaultPaletteID.has_value())
	{
		DebugLogError("Couldn't get default palette ID from \"" + defaultPaletteFilename + "\".");
		return;
	}

	const Palette &defaultPalette = textureManager.getPaletteHandle(*defaultPaletteID);

	renderer.renderWorld(player.getPosition(), player.getDirection(), options.getGraphics_VerticalFOV(),
		ambientPercent, gameState.getDaytimePercent(), gameState.getChasmAnimPercent(), latitude,
		gameState.nightLightsAreActive(), isExterior, options.getMisc_PlayerHasLight(),
		options.getMisc_ChunkDistance(), activeLevelInst.getCeilingScale(), activeLevelInst, activeSkyInst,
		activeWeatherInst, game.getRandom(), game.getEntityDefinitionLibrary(), defaultPalette);

	const TextureBuilderID gameWorldInterfaceTextureBuilderID = GameWorldUiView::getGameWorldInterfaceTextureBuilderID(textureManager);

	const TextureBuilderID statusGradientTextureBuilderID = [this, &game]()
	{
		constexpr int gradientID = 0; // Default for now.
		return GameWorldUiView::getStatusGradientTextureBuilderID(game, gradientID);
	}();
	
	const TextureBuilderID playerPortraitTextureBuilderID = [this, &game, &player]()
	{
		const std::string &headsFilename = PortraitFile::getHeads(player.isMale(), player.getRaceID(), true);
		return GameWorldUiView::getPlayerPortraitTextureBuilderID(game, headsFilename, player.getPortraitID());
	}();

	const TextureBuilderID noSpellTextureBuilderID = GameWorldUiView::getNoSpellTextureBuilderID(game);

	const auto &inputManager = game.getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const bool modernInterface = options.getGraphics_ModernInterface();

	// Continue drawing more interface objects if in classic mode.
	// - @todo: clamp game world interface to screen edges, not letterbox edges.
	if (!modernInterface)
	{
		// Draw game world interface.
		const TextureBuilderRef gameWorldInterfaceTextureBuilderRef =
			textureManager.getTextureBuilderRef(gameWorldInterfaceTextureBuilderID);
		const Int2 gameWorldInterfacePosition =
			GameWorldUiView::getGameWorldInterfacePosition(gameWorldInterfaceTextureBuilderRef.getHeight());
		renderer.drawOriginal(gameWorldInterfaceTextureBuilderID, *defaultPaletteID,
			gameWorldInterfacePosition.x, gameWorldInterfacePosition.y, textureManager);

		// Draw player portrait.
		renderer.drawOriginal(statusGradientTextureBuilderID, *defaultPaletteID,
			GameWorldUiView::PlayerPortraitX, GameWorldUiView::PlayerPortraitY, textureManager);
		renderer.drawOriginal(playerPortraitTextureBuilderID, *defaultPaletteID,
			GameWorldUiView::PlayerPortraitX, GameWorldUiView::PlayerPortraitY, textureManager);

		// If the player's class can't use magic, show the darkened spell icon.
		const auto &charClassLibrary = game.getCharacterClassLibrary();
		const auto &charClassDef = charClassLibrary.getDefinition(player.getCharacterClassDefID());
		if (!charClassDef.canCastMagic())
		{
			const Int2 noMagicTexturePosition = GameWorldUiView::getNoMagicTexturePosition();
			renderer.drawOriginal(noSpellTextureBuilderID, *defaultPaletteID,
				noMagicTexturePosition.x, noMagicTexturePosition.y, textureManager);
		}

		// Draw text: player name.
		const Rect &playerNameTextBoxRect = this->playerNameTextBox.getRect();
		renderer.drawOriginal(this->playerNameTextBox.getTexture(),
			playerNameTextBoxRect.getLeft(), playerNameTextBoxRect.getTop());
	}
}

void GameWorldPanel::renderSecondary(Renderer &renderer)
{
	DebugAssert(this->getGame().gameStateIsActive());
	
	auto &game = this->getGame();
	auto &gameState = game.getGameState();
	auto &player = gameState.getPlayer();
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	// Several interface objects are in this method because they are hidden by the status
	// pop-up and the spells list.
	auto &textureManager = game.getTextureManager();
	const TextureAssetReference defaultPaletteTextureAssetRef = GameWorldUiView::getDefaultPaletteTextureAssetRef();
	const std::optional<PaletteID> defaultPaletteID = textureManager.tryGetPaletteID(defaultPaletteTextureAssetRef);
	if (!defaultPaletteID.has_value())
	{
		DebugLogError("Couldn't get default palette ID from \"" + defaultPaletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureBuilderID gameWorldInterfaceTextureBuilderID =
		GameWorldUiView::getGameWorldInterfaceTextureBuilderID(textureManager);

	// Display player's weapon if unsheathed.
	const std::optional<TextureBuilderID> weaponTextureBuilderID = GameWorldUiView::getActiveWeaponAnimationTextureBuilderID(game);
	if (weaponTextureBuilderID.has_value())
	{
		const TextureBuilderRef gameWorldInterfaceTextureBuilderRef =
			textureManager.getTextureBuilderRef(gameWorldInterfaceTextureBuilderID);
		const TextureBuilderRef weaponTextureBuilderRef = textureManager.getTextureBuilderRef(*weaponTextureBuilderID);
		const Int2 weaponOffset = GameWorldUiView::getCurrentWeaponAnimationOffset(game);

		// Draw the current weapon image depending on interface mode.
		if (modernInterface)
		{
			// @todo: this would probably be a lot easier to do if it could just specify it's in the native vector space?
			// - clean this up for draw calls/UiTextureID stuff

			// Draw stretched to fit the window.
			const int letterboxStretchMode = Options::MAX_LETTERBOX_MODE;
			renderer.setLetterboxMode(letterboxStretchMode);

			// Percent of the horizontal weapon offset across the original screen.
			const double weaponOffsetXPercent = static_cast<double>(weaponOffset.x) /
				static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);

			// Native left and right screen edges converted to original space.
			const int newLeft = renderer.nativeToOriginal(Int2(0, 0)).x;
			const int newRight = renderer.nativeToOriginal(Int2(renderer.getWindowDimensions().x, 0)).x;
			const double newDiff = static_cast<double>(newRight - newLeft);

			// Values to scale original weapon dimensions by.
			const double weaponScaleX = newDiff / static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
			const double weaponScaleY = static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT) /
				static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilderRef.getHeight());

			const int weaponX = newLeft + static_cast<int>(std::round(newDiff * weaponOffsetXPercent));
			const int weaponY = static_cast<int>(std::round(static_cast<double>(weaponOffset.y) * weaponScaleY));
			const int weaponWidth = static_cast<int>(std::round(static_cast<double>(weaponTextureBuilderRef.getWidth()) * weaponScaleX));
			const int weaponHeight = static_cast<int>(std::round(static_cast<double>(
				std::min(weaponTextureBuilderRef.getHeight() + 1, std::max(ArenaRenderUtils::SCREEN_HEIGHT - weaponY, 0))) * weaponScaleY));

			renderer.drawOriginal(*weaponTextureBuilderID, *defaultPaletteID, weaponX, weaponY,
				weaponWidth, weaponHeight, textureManager);

			// Reset letterbox mode back to what it was.
			renderer.setLetterboxMode(options.getGraphics_LetterboxMode());
		}
		else
		{
			// Clamp the max weapon height non-negative since some weapon animations like the
			// morning star can cause it to become -1.
			const int maxWeaponHeight = std::max(
				(ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilderRef.getHeight()) - weaponOffset.y, 0);

			// Add 1 to the height because Arena's renderer has an off-by-one bug, and a 1 pixel
			// gap appears unless a small change is added.
			const int weaponHeight = std::clamp(weaponTextureBuilderRef.getHeight() + 1, 0, maxWeaponHeight);
			renderer.drawOriginal(*weaponTextureBuilderID, *defaultPaletteID,
				weaponOffset.x, weaponOffset.y, weaponTextureBuilderRef.getWidth(), weaponHeight, textureManager);
		}
	}

	// Draw the visible portion of the compass slider, and the frame over it.
	if (options.getMisc_ShowCompass())
	{
		this->drawCompass(player.getGroundDirection(), textureManager, renderer);
	}

	// Draw each pop-up text if its duration is positive.
	// - @todo: maybe give delta time to render()? Or store in tick()? I want to avoid
	//   subtracting the time in tick() because it would always be one frame shorter then.
	if (gameState.triggerTextIsVisible())
	{
		const Texture &triggerTextTexture = this->triggerText.getTexture();
		const TextureBuilderRef gameWorldInterfaceTextureBuilderRef =
			textureManager.getTextureBuilderRef(gameWorldInterfaceTextureBuilderID);
		const Int2 textPosition = GameWorldUiView::getTriggerTextPosition(
			game, triggerTextTexture.getWidth(), triggerTextTexture.getHeight(),
			gameWorldInterfaceTextureBuilderRef.getHeight());

		renderer.drawOriginal(triggerTextTexture, textPosition.x, textPosition.y);
	}

	if (gameState.actionTextIsVisible())
	{
		const Texture &actionTextTexture = this->actionText.getTexture();
		const Int2 textPosition = GameWorldUiView::getActionTextPosition(actionTextTexture.getWidth());
		renderer.drawOriginal(actionTextTexture, textPosition.x, textPosition.y);
	}

	if (gameState.effectTextIsVisible())
	{
		// @todo: draw "effect text".
		//GameWorldUiView::getEffectTextPosition()
	}

	// Check if the mouse is over one of the buttons for tooltips in classic mode.
	if (!modernInterface)
	{
		auto &game = this->getGame();
		const auto &inputManager = game.getInputManager();
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

		// Get the hovered tooltip string, or the empty string if none are hovered over.
		const std::string tooltip = [this, &game, &player, &originalPosition]() -> std::string
		{
			const auto &charClassLibrary = game.getCharacterClassLibrary();
			const auto &charClassDef = charClassLibrary.getDefinition(player.getCharacterClassDefID());

			if (this->characterSheetButton.contains(originalPosition))
			{
				return GameWorldUiModel::getCharacterSheetTooltipText();
			}
			else if (this->drawWeaponButton.contains(originalPosition))
			{
				return GameWorldUiModel::getWeaponTooltipText();
			}
			else if (this->mapButton.contains(originalPosition))
			{
				return GameWorldUiModel::getMapTooltipText();
			}
			else if (this->stealButton.contains(originalPosition))
			{
				return GameWorldUiModel::getStealTooltipText();
			}
			else if (this->statusButton.contains(originalPosition))
			{
				return GameWorldUiModel::getStatusTooltipText();
			}
			else if (this->magicButton.contains(originalPosition) && charClassDef.canCastMagic())
			{
				return GameWorldUiModel::getMagicTooltipText();
			}
			else if (this->logbookButton.contains(originalPosition))
			{
				return GameWorldUiModel::getLogbookTooltipText();
			}
			else if (this->useItemButton.contains(originalPosition))
			{
				return GameWorldUiModel::getUseItemTooltipText();
			}
			else if (this->campButton.contains(originalPosition))
			{
				return GameWorldUiModel::getCampTooltipText();
			}
			else
			{
				// None are hovered. Return empty string.
				return std::string();
			}
		}();

		if (tooltip.size() > 0)
		{
			this->drawTooltip(tooltip, renderer);
		}
	}

	// Optionally draw profiler text.
	GameWorldUiView::DEBUG_DrawProfiler(game, renderer);
}
