#include <algorithm>
#include <cmath>

#include "SDL.h"

#include "AutomapPanel.h"
#include "CharacterPanel.h"
#include "GameWorldPanel.h"
#include "GameWorldUiController.h"
#include "GameWorldUiModel.h"
#include "GameWorldUiView.h"
#include "LogbookPanel.h"
#include "PauseMenuPanel.h"
#include "TextSubPanel.h"
#include "WorldMapPanel.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Audio/AudioManager.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/CharacterClassDefinition.h"
#include "../Entities/CharacterClassLibrary.h"
#include "../Entities/Entity.h"
#include "../Entities/EntityAnimationInstance.h"
#include "../Entities/EntityAnimationUtils.h"
#include "../Entities/EntityType.h"
#include "../Entities/Player.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/ArenaDateUtils.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/GameState.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Game/PlayerInterface.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/PortraitFile.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/FontLibrary.h"
#include "../UI/FontName.h"
#include "../UI/FontUtils.h"
#include "../UI/RichTextString.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/Texture.h"
#include "../World/ArenaInteriorUtils.h"
#include "../World/ArenaLevelUtils.h"
#include "../World/ArenaVoxelUtils.h"
#include "../World/ArenaWeatherUtils.h"
#include "../World/ArenaWildUtils.h"
#include "../World/ChunkUtils.h"
#include "../World/MapType.h"
#include "../World/SkyUtils.h"
#include "../World/VoxelFacing3D.h"
#include "../World/WeatherUtils.h"
#include "../WorldMap/LocationType.h"
#include "../WorldMap/LocationUtils.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

GameWorldPanel::GameWorldPanel(Game &game)
	: Panel(game)
{
	DebugAssert(game.gameStateIsActive());

	this->playerNameTextBox = [&game]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			GameWorldUiModel::getPlayerNameText(game),
			GameWorldUiView::PlayerNameFontName,
			GameWorldUiView::PlayerNameTextColor,
			GameWorldUiView::PlayerNameTextAlignment,
			fontLibrary);

		return std::make_unique<TextBox>(
			GameWorldUiView::PlayerNameTextBoxX,
			GameWorldUiView::PlayerNameTextBoxY,
			richText,
			fontLibrary,
			game.getRenderer());
	}();

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
	this->updateCursorRegions(screenDims.x, screenDims.y);

	// If in modern mode, lock mouse to center of screen for free-look.
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (modernInterface)
	{
		GameWorldUiModel::setFreeLookActive(game, true);
	}
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

std::optional<Panel::CursorData> GameWorldPanel::getCurrentCursor() const
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

void GameWorldPanel::updateCursorRegions(int width, int height)
{
	// @todo: maybe the classic rects should be converted to vector space then scaled by the ratio of aspect ratios?
	const double xScale = static_cast<double>(width) / static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
	const double yScale = static_cast<double>(height) / static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT);

	for (int i = 0; i < static_cast<int>(this->nativeCursorRegions.size()); i++)
	{
		this->nativeCursorRegions[i] = GameWorldUiView::scaleClassicCursorRectToNative(i, xScale, yScale);
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
		auto &gameState = game.getGameState();
		gameState.setActionText(text, game.getFontLibrary(), game.getRenderer());
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
					this->handleClickInWorld(mousePosition, primaryClick, debugFadeVoxel);
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
					this->handleClickInWorld(mousePosition, primaryClick, false);
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
			this->handleClickInWorld(nativeCenter, primaryClick, debugFadeVoxel);
		}
		else if (leftClick)
		{
			// Read (right click in classic mode).
			const bool primaryClick = false;
			this->handleClickInWorld(nativeCenter, primaryClick, false);
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
	this->updateCursorRegions(windowWidth, windowHeight);
}

void GameWorldPanel::handlePlayerTurning(double dt, const Int2 &mouseDelta)
{
	// @todo: move this to some game/player logic controller namespace

	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicTurning()
	// 2) handleModernTurning()

	// Don't handle weapon swinging here. That can go in another method.
	// If right click is held, weapon is out, and mouse motion is significant, then
	// get the swing direction and swing.
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();

	if (!modernInterface)
	{
		// Classic interface mode.
		auto &player = game.getGameState().getPlayer();
		const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);
		const bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		const bool right = inputManager.keyIsDown(SDL_SCANCODE_D);

		// Don't turn if LCtrl is held.
		const bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		// Mouse turning takes priority over key turning.
		if (leftClick)
		{
			const Int2 mousePosition = inputManager.getMousePosition();

			// Strength of turning is determined by proximity of the mouse cursor to
			// the left or right screen edge.
			const double dx = [this, &mousePosition]()
			{
				// Measure the magnitude of rotation. -1.0 is left, 1.0 is right.
				const double percent = [this, &mousePosition]()
				{
					const int mouseX = mousePosition.x;

					// Native cursor regions for turning (scaled to the current window).
					const Rect &topLeft = this->nativeCursorRegions[GameWorldUiView::CursorTopLeftIndex];
					const Rect &topRight = this->nativeCursorRegions[GameWorldUiView::CursorTopRightIndex];
					const Rect &middleLeft = this->nativeCursorRegions[GameWorldUiView::CursorMiddleLeftIndex];
					const Rect &middleRight = this->nativeCursorRegions[GameWorldUiView::CursorMiddleRightIndex];

					if (topLeft.contains(mousePosition))
					{
						return -1.0 + (static_cast<double>(mouseX) / topLeft.getWidth());
					}
					else if (topRight.contains(mousePosition))
					{
						return static_cast<double>(mouseX - topRight.getLeft()) / topRight.getWidth();
					}
					else if (middleLeft.contains(mousePosition))
					{
						return -1.0 + (static_cast<double>(mouseX) / middleLeft.getWidth());
					}
					else if (middleRight.contains(mousePosition))
					{
						return static_cast<double>(mouseX - middleRight.getLeft()) / middleRight.getWidth();
					}
					else
					{
						return 0.0;
					}
				}();

				// No NaNs or infinities allowed.
				return std::isfinite(percent) ? percent : 0.0;
			}();

			// Yaw the camera left or right. No vertical movement in classic camera mode.
			// Multiply turning speed by delta time so it behaves correctly with different
			// frame rates.
			player.rotate(dx * dt, 0.0, options.getInput_HorizontalSensitivity(),
				options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
		}
		else if (!lCtrl)
		{
			// If left control is not held, then turning is permitted.
			if (left)
			{
				// Turn left at a fixed angular velocity.
				player.rotate(-dt, 0.0, options.getInput_HorizontalSensitivity(),
					options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
			}
			else if (right)
			{
				// Turn right at a fixed angular velocity.
				player.rotate(dt, 0.0, options.getInput_HorizontalSensitivity(),
					options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
			}
		}
	}
	else
	{
		// Modern interface. Make the camera look around if the player's weapon is not in use.
		const int dx = mouseDelta.x;
		const int dy = mouseDelta.y;
		const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);

		auto &player = game.getGameState().getPlayer();
		const auto &weaponAnim = player.getWeaponAnimation();
		const bool turning = ((dx != 0) || (dy != 0)) && (weaponAnim.isSheathed() || !rightClick);

		if (turning)
		{
			const Int2 dimensions = game.getRenderer().getWindowDimensions();

			// Get the smaller of the two dimensions, so the look sensitivity is relative
			// to a square instead of a rectangle. This keeps the camera look independent
			// of the aspect ratio.
			const int minDimension = std::min(dimensions.x, dimensions.y);
			const double dxx = static_cast<double>(dx) / static_cast<double>(minDimension);
			const double dyy = static_cast<double>(dy) / static_cast<double>(minDimension);

			// Pitch and/or yaw the camera.
			player.rotate(dxx, -dyy, options.getInput_HorizontalSensitivity(),
				options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
		}
	}
}

void GameWorldPanel::handlePlayerMovement(double dt)
{
	// @todo: this should be in some game/player logic controller namespace

	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicMovement()
	// 2) handleModernMovement()

	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();

	// Arbitrary movement speeds.
	constexpr double walkSpeed = 15.0;
	constexpr double runSpeed = 30.0;

	const GameState &gameState = game.getGameState();
	const MapInstance &mapInst = gameState.getActiveMapInst();
	const LevelInstance &levelInst = mapInst.getActiveLevel();

	const bool modernInterface = game.getOptions().getGraphics_ModernInterface();
	if (!modernInterface)
	{
		// Classic interface mode.
		// Arena uses arrow keys, but let's use the left hand side of the keyboard
		// because we like being comfortable.

		// A and D turn the player, and if Ctrl is held, the player slides instead.
		// Let's keep the turning part in the other method because turning doesn't
		// affect velocity.

		// Listen for mouse, WASD, and Ctrl.
		const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);

		bool forward = inputManager.keyIsDown(SDL_SCANCODE_W);
		bool backward = inputManager.keyIsDown(SDL_SCANCODE_S);
		bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		bool space = inputManager.keyIsDown(SDL_SCANCODE_SPACE);
		bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		// The original game didn't have sprinting, but it seems like something
		// relevant to do anyway (at least for development).
		bool isRunning = inputManager.keyIsDown(SDL_SCANCODE_LSHIFT);

		auto &player = game.getGameState().getPlayer();

		// Get some relevant player direction data (getDirection() isn't necessary here
		// because the Y component is intentionally truncated).
		const Double2 groundDirection = player.getGroundDirection();
		const Double3 groundDirection3D = Double3(groundDirection.x, 0.0,
			groundDirection.y).normalized();
		const Double3 &rightDirection = player.getRight();

		// Mouse movement takes priority over key movement.
		if (leftClick && player.onGround(levelInst))
		{
			const Int2 mousePosition = inputManager.getMousePosition();
			const int mouseX = mousePosition.x;
			const int mouseY = mousePosition.y;

			// Native cursor regions for motion (scaled to the current window).
			const Rect &topLeft = this->nativeCursorRegions[GameWorldUiView::CursorTopLeftIndex];
			const Rect &top = this->nativeCursorRegions[GameWorldUiView::CursorTopMiddleIndex];
			const Rect &topRight = this->nativeCursorRegions[GameWorldUiView::CursorTopRightIndex];
			const Rect &bottomLeft = this->nativeCursorRegions[GameWorldUiView::CursorBottomLeftIndex];
			const Rect &bottom = this->nativeCursorRegions[GameWorldUiView::CursorBottomMiddleIndex];
			const Rect &bottomRight = this->nativeCursorRegions[GameWorldUiView::CursorBottomRightIndex];

			// Strength of movement is determined by the mouse's position in each region.
			// Motion magnitude (percent) is between 0.0 and 1.0.
			double percent = 0.0;
			Double3 accelDirection(0.0, 0.0, 0.0);
			if (topLeft.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection3D;
				percent = 1.0 - (static_cast<double>(mouseY) / topLeft.getHeight());
			}
			else if (top.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection3D;
				percent = 1.0 - (static_cast<double>(mouseY) / top.getHeight());
			}
			else if (topRight.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection3D;
				percent = 1.0 - (static_cast<double>(mouseY) / topRight.getHeight());
			}
			else if (bottomLeft.contains(mousePosition))
			{
				// Left.
				accelDirection = accelDirection - rightDirection;
				percent = 1.0 - (static_cast<double>(mouseX) / bottomLeft.getWidth());
			}
			else if (bottom.contains(mousePosition))
			{
				// Backwards.
				accelDirection = accelDirection - groundDirection3D;
				percent = static_cast<double>(mouseY - bottom.getTop()) / bottom.getHeight();
			}
			else if (bottomRight.contains(mousePosition))
			{
				// Right.
				accelDirection = accelDirection + rightDirection;
				percent = static_cast<double>(mouseX - bottomRight.getLeft()) / bottomRight.getWidth();
			}

			// Only attempt to accelerate if a direction was chosen.
			if (accelDirection.lengthSquared() > 0.0)
			{
				// Use a normalized direction.
				accelDirection = accelDirection.normalized();

				// Set the magnitude of the acceleration to some arbitrary number. These values
				// are independent of max speed.
				double accelMagnitude = percent * (isRunning ? runSpeed : walkSpeed);

				// Check for jumping first (so the player can't slide jump on the first frame).
				const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);
				if (rightClick)
				{
					// Jump.
					player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
				}
				// Change the player's velocity if valid.
				else if (std::isfinite(accelDirection.length()) && std::isfinite(accelMagnitude))
				{
					player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
				}
			}
		}
		else if ((forward || backward || ((left || right) && lCtrl) || space) && player.onGround(levelInst))
		{
			// Calculate the acceleration direction based on input.
			Double3 accelDirection(0.0, 0.0, 0.0);

			if (forward)
			{
				accelDirection = accelDirection + groundDirection3D;
			}

			if (backward)
			{
				accelDirection = accelDirection - groundDirection3D;
			}

			if (right)
			{
				accelDirection = accelDirection + rightDirection;
			}

			if (left)
			{
				accelDirection = accelDirection - rightDirection;
			}

			// Use a normalized direction.
			accelDirection = accelDirection.normalized();

			// Set the magnitude of the acceleration to some arbitrary number. These values
			// are independent of max speed.
			double accelMagnitude = isRunning ? runSpeed : walkSpeed;

			// Check for jumping first (so the player can't slide jump on the first frame).
			if (space)
			{
				// Jump.
				player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
			}
			// Change the player's velocity if valid.
			else if (std::isfinite(accelDirection.length()))
			{
				player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
			}
		}
	}
	else
	{
		// Modern interface. Listen for WASD.
		bool forward = inputManager.keyIsDown(SDL_SCANCODE_W);
		bool backward = inputManager.keyIsDown(SDL_SCANCODE_S);
		bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		bool space = inputManager.keyIsDown(SDL_SCANCODE_SPACE);

		// The original game didn't have sprinting, but it seems like something
		// relevant to do anyway (at least for development).
		bool isRunning = inputManager.keyIsDown(SDL_SCANCODE_LSHIFT);

		auto &player = game.getGameState().getPlayer();

		// Get some relevant player direction data (getDirection() isn't necessary here
		// because the Y component is intentionally truncated).
		const Double2 groundDirection = player.getGroundDirection();
		const Double3 groundDirection3D = Double3(groundDirection.x, 0.0,
			groundDirection.y).normalized();
		const Double3 &rightDirection = player.getRight();

		if ((forward || backward || left || right || space) && player.onGround(levelInst))
		{
			// Calculate the acceleration direction based on input.
			Double3 accelDirection(0.0, 0.0, 0.0);

			if (forward)
			{
				accelDirection = accelDirection + groundDirection3D;
			}

			if (backward)
			{
				accelDirection = accelDirection - groundDirection3D;
			}

			if (right)
			{
				accelDirection = accelDirection + rightDirection;
			}

			if (left)
			{
				accelDirection = accelDirection - rightDirection;
			}

			// Use a normalized direction.
			accelDirection = accelDirection.normalized();

			// Set the magnitude of the acceleration to some arbitrary number. These values
			// are independent of max speed.
			double accelMagnitude = isRunning ? runSpeed : walkSpeed;

			// Check for jumping first (so the player can't slide jump on the first frame).
			if (space)
			{
				// Jump.
				player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
			}
			// Change the player's horizontal velocity if valid.
			else if (std::isfinite(accelDirection.length()))
			{
				player.accelerate(accelDirection, accelMagnitude, isRunning, dt);
			}
		}
	}
}

void GameWorldPanel::handlePlayerAttack(const Int2 &mouseDelta)
{
	// @todo: run this method at fixed time-steps instead of every frame, because if,
	// for example, the game is running at 200 fps, then the player has to move their
	// cursor much faster for it to count as a swing. The GameWorldPanel would probably
	// need to save its own "swing" mouse delta independently of the input manager, or
	// maybe the game loop could call a "Panel::fixedTick()" method.

	// Only handle attacking if the player's weapon is currently idle.
	auto &weaponAnimation = this->getGame().getGameState().getPlayer().getWeaponAnimation();
	if (weaponAnimation.isIdle())
	{
		const auto &inputManager = this->getGame().getInputManager();
		auto &audioManager = this->getGame().getAudioManager();

		if (!weaponAnimation.isRanged())
		{
			// Handle melee attack.
			const Int2 dimensions = this->getGame().getRenderer().getWindowDimensions();

			// Get the smaller of the two dimensions, so the percentage change in mouse position
			// is relative to a square instead of a rectangle.
			const int minDimension = std::min(dimensions.x, dimensions.y);

			// Percentages that the mouse moved across the screen.
			const double dxx = static_cast<double>(mouseDelta.x) /
				static_cast<double>(minDimension);
			const double dyy = static_cast<double>(mouseDelta.y) /
				static_cast<double>(minDimension);

			const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);

			// If the mouse moves fast enough, it's considered an attack. The distances
			// are in percentages of screen dimensions.
			const double requiredDistance = 0.060;
			const double mouseDistance = std::sqrt((dxx * dxx) + (dyy * dyy));
			const bool isAttack = rightClick && (mouseDistance >= requiredDistance);

			if (isAttack)
			{
				// Convert the change in mouse coordinates to a vector. Reverse the change in
				// y so that positive values are up.
				const Double2 mouseDirection = Double2(dxx, -dyy).normalized();

				// Calculate the direction the mouse moved in (let's use cardinal directions
				// for convenience. This is actually a little weird now because +X is south
				// and +Y is west).
				CardinalDirectionName cardinalDirection = CardinalDirection::getDirectionName(
					Double2(-mouseDirection.y, -mouseDirection.x));

				// Set the weapon animation state.
				if (cardinalDirection == CardinalDirectionName::North)
				{
					weaponAnimation.setState(WeaponAnimation::State::Forward);
				}
				else if (cardinalDirection == CardinalDirectionName::NorthEast)
				{
					weaponAnimation.setState(WeaponAnimation::State::Right);
				}
				else if (cardinalDirection == CardinalDirectionName::East)
				{
					weaponAnimation.setState(WeaponAnimation::State::Right);
				}
				else if (cardinalDirection == CardinalDirectionName::SouthEast)
				{
					weaponAnimation.setState(WeaponAnimation::State::DownRight);
				}
				else if (cardinalDirection == CardinalDirectionName::South)
				{
					weaponAnimation.setState(WeaponAnimation::State::Down);
				}
				else if (cardinalDirection == CardinalDirectionName::SouthWest)
				{
					weaponAnimation.setState(WeaponAnimation::State::DownLeft);
				}
				else if (cardinalDirection == CardinalDirectionName::West)
				{
					weaponAnimation.setState(WeaponAnimation::State::Left);
				}
				else if (cardinalDirection == CardinalDirectionName::NorthWest)
				{
					weaponAnimation.setState(WeaponAnimation::State::Left);
				}

				// Play the swing sound.
				audioManager.playSound(ArenaSoundName::Swish);
			}
		}
		else
		{
			// Handle ranged attack.
			const bool isAttack = [this, &inputManager]()
			{
				auto &game = this->getGame();
				const auto &options = game.getOptions();
				const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);

				if (!options.getGraphics_ModernInterface())
				{
					// The cursor must be above the game world interface in order to fire. In
					// the original game, the cursor has to be in the center "X" region, but
					// that seems pretty inconvenient, given that the border between cursor
					// regions is hard to see at a glance, and that might be the difference
					// between shooting an arrow and not shooting an arrow, so I'm relaxing
					// the requirements here.
					auto &textureManager = game.getTextureManager();
					auto &renderer = game.getRenderer();
					const TextureBuilderID gameWorldInterfaceTextureID =
						GameWorldUiView::getGameWorldInterfaceTextureBuilderID(textureManager);
					const TextureBuilder &gameWorldInterfaceTextureBuilder =
						textureManager.getTextureBuilderHandle(gameWorldInterfaceTextureID);
					const int originalCursorY = renderer.nativeToOriginal(inputManager.getMousePosition()).y;
					return rightClick && (originalCursorY <
						(ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceTextureBuilder.getHeight()));
				}
				else
				{
					// Right clicking anywhere in modern mode fires an arrow.
					return rightClick;
				}
			}();

			if (isAttack)
			{
				// Set firing state for animation.
				weaponAnimation.setState(WeaponAnimation::State::Firing);

				// Play the firing sound.
				audioManager.playSound(ArenaSoundName::ArrowFire);
			}
		}
	}
}

void GameWorldPanel::handleClickInWorld(const Int2 &nativePoint, bool primaryClick,
	bool debugFadeVoxel)
{
	auto &game = this->getGame();
	auto &gameState = game.getGameState();
	const auto &options = game.getOptions();
	auto &player = gameState.getPlayer();
	const Double3 &cameraDirection = player.getDirection();
	const MapDefinition &mapDef = gameState.getActiveMapDef();
	MapInstance &mapInst = gameState.getActiveMapInst();
	LevelInstance &levelInst = mapInst.getActiveLevel();
	ChunkManager &chunkManager = levelInst.getChunkManager();
	const EntityManager &entityManager = levelInst.getEntityManager();
	const double ceilingScale = levelInst.getCeilingScale();

	const CoordDouble3 rayStart = player.getPosition();
	const VoxelDouble3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, nativePoint);

	// Pixel-perfect selection determines whether an entity's texture is used in the
	// selection calculation.
	const bool pixelPerfectSelection = options.getInput_PixelPerfectSelection();

	const std::string &paletteFilename = ArenaPaletteName::Default;
	auto &textureManager = game.getTextureManager();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	constexpr bool includeEntities = true;

	Physics::Hit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
		pixelPerfectSelection, palette, includeEntities, levelInst, game.getEntityDefinitionLibrary(),
		game.getRenderer(), hit);

	// See if the ray hit anything.
	if (success)
	{
		if (hit.getType() == Physics::Hit::Type::Voxel)
		{
			const ChunkInt2 chunk = hit.getCoord().chunk;
			Chunk *chunkPtr = chunkManager.tryGetChunk(chunk);
			DebugAssert(chunkPtr != nullptr);

			const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();
			const VoxelInt3 &voxel = voxelHit.voxel;
			const Chunk::VoxelID voxelID = chunkPtr->getVoxel(voxel.x, voxel.y, voxel.z);
			const VoxelDefinition &voxelDef = chunkPtr->getVoxelDef(voxelID);

			// Primary click handles selection in the game world. Secondary click handles
			// reading names of things.
			if (primaryClick)
			{
				// Arbitrary max distance for selection.
				// @todo: move to some ArenaPlayerUtils maybe
				constexpr double maxSelectionDist = 1.75;

				if (hit.getT() <= maxSelectionDist)
				{
					if (voxelDef.type == ArenaTypes::VoxelType::Wall ||
						voxelDef.type == ArenaTypes::VoxelType::Floor ||
						voxelDef.type == ArenaTypes::VoxelType::Raised ||
						voxelDef.type == ArenaTypes::VoxelType::Diagonal ||
						voxelDef.type == ArenaTypes::VoxelType::TransparentWall ||
						voxelDef.type == ArenaTypes::VoxelType::Edge)
					{
						if (!debugFadeVoxel)
						{
							const bool isWall = voxelDef.type == ArenaTypes::VoxelType::Wall;

							// The only edge voxels with a transition should be should be palace entrances (with collision).
							const bool isEdge = (voxelDef.type == ArenaTypes::VoxelType::Edge) && voxelDef.edge.collider;

							if (isWall || isEdge)
							{
								const TransitionDefinition *transitionDef = chunkPtr->tryGetTransition(voxel);
								if ((transitionDef != nullptr) &&
									(transitionDef->getType() != TransitionType::LevelChange))
								{
									this->handleMapTransition(hit, *transitionDef);
								}
							}
						}
						else
						{
							// @temp: add to fading voxels if it doesn't already exist.
							const VoxelInstance *existingFadingVoxelInst =
								chunkPtr->tryGetVoxelInst(voxel, VoxelInstance::Type::Fading);
							const bool isFading = existingFadingVoxelInst != nullptr;
							if (!isFading)
							{
								VoxelInstance newFadingVoxelInst = VoxelInstance::makeFading(
									voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::FADING_VOXEL_SECONDS);
								chunkPtr->addVoxelInst(std::move(newFadingVoxelInst));
							}
						}
					}
					else if (voxelDef.type == ArenaTypes::VoxelType::Door)
					{
						const VoxelDefinition::DoorData &doorData = voxelDef.door;

						// If the door is closed, then open it.
						const VoxelInstance *existingOpenDoorInst =
							chunkPtr->tryGetVoxelInst(voxel, VoxelInstance::Type::OpenDoor);
						const bool isClosed = existingOpenDoorInst == nullptr;

						if (isClosed)
						{
							// Add the door to the open doors list.
							VoxelInstance newOpenDoorInst = VoxelInstance::makeDoor(
								voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::DOOR_ANIM_SPEED);
							chunkPtr->addVoxelInst(std::move(newOpenDoorInst));

							// Get the door's opening sound and play it at the center of the voxel.
							const DoorDefinition *doorDefPtr = chunkPtr->tryGetDoor(voxel);
							DebugAssert(doorDefPtr != nullptr);
							const DoorDefinition::OpenSoundDef &openSoundDef = doorDefPtr->getOpenSound();

							auto &audioManager = game.getAudioManager();
							const std::string &soundFilename = openSoundDef.soundFilename;
							const Double3 soundPosition(
								static_cast<SNDouble>(voxel.x) + 0.50,
								(static_cast<double>(voxel.y) * ceilingScale) + (ceilingScale * 0.50),
								static_cast<WEDouble>(voxel.z) + 0.50);

							audioManager.playSound(soundFilename, soundPosition);
						}
					}
				}
			}
			else
			{
				// Handle secondary click (i.e., right click).
				if (voxelDef.type == ArenaTypes::VoxelType::Wall)
				{
					const std::string *buildingName = chunkPtr->tryGetBuildingName(voxel);
					if (buildingName != nullptr)
					{
						auto &gameState = game.getGameState();
						gameState.setActionText(*buildingName, game.getFontLibrary(), game.getRenderer());
					}
				}
			}
		}
		else if (hit.getType() == Physics::Hit::Type::Entity)
		{
			const Physics::Hit::EntityHit &entityHit = hit.getEntityHit();
			const auto &exeData = game.getBinaryAssetLibrary().getExeData();

			if (primaryClick)
			{
				// @todo: max selection distance matters when talking to NPCs and selecting corpses.
				// - need to research a bit since I think it switches between select and inspect
				//   depending on distance and entity state.
				// - Also need the "too far away..." text?
				/*const double maxSelectionDist = 1.50;
				if (hit.t <= maxSelectionDist)
				{

				}*/

				// Try inspecting the entity (can be from any distance). If they have a display name,
				// then show it.
				ConstEntityRef entityRef = entityManager.getEntityRef(entityHit.id, entityHit.type);
				DebugAssert(entityRef.getID() != EntityManager::NO_ID);

				const EntityDefinition &entityDef = entityManager.getEntityDef(
					entityRef.get()->getDefinitionID(), game.getEntityDefinitionLibrary());
				const auto &charClassLibrary = game.getCharacterClassLibrary();

				std::string entityName;
				std::string text;
				if (EntityUtils::tryGetDisplayName(entityDef, charClassLibrary, &entityName))
				{
					text = exeData.ui.inspectedEntityName;

					// Replace format specifier with entity name.
					text = String::replace(text, "%s", entityName);
				}
				else
				{
					// Placeholder text for testing.
					text = "Entity " + std::to_string(entityHit.id) + " (" +
						EntityUtils::defTypeToString(entityDef) + ")";
				}

				auto &gameState = game.getGameState();
				gameState.setActionText(text, game.getFontLibrary(), game.getRenderer());
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(hit.getType())));
		}
	}
}

void GameWorldPanel::handleNightLightChange(bool active)
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	GameState &gameState = game.getGameState();
	MapInstance &mapInst = gameState.getActiveMapInst();
	LevelInstance &levelInst = mapInst.getActiveLevel();
	auto &entityManager = levelInst.getEntityManager();
	const auto &entityDefLibrary = game.getEntityDefinitionLibrary();

	// Turn streetlights on or off.
	Buffer<Entity*> entityBuffer(entityManager.getCountOfType(EntityType::Static));
	const int entityCount = entityManager.getEntitiesOfType(
		EntityType::Static, entityBuffer.get(), entityBuffer.getCount());

	for (int i = 0; i < entityCount; i++)
	{
		Entity *entity = entityBuffer.get(i);
		const EntityDefID defID = entity->getDefinitionID();
		const EntityDefinition &entityDef = entityManager.getEntityDef(defID, entityDefLibrary);

		if (EntityUtils::isStreetlight(entityDef))
		{
			const std::string &newStateName = active ?
				EntityAnimationUtils::STATE_ACTIVATED : EntityAnimationUtils::STATE_IDLE;

			const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
			const std::optional<int> newStateIndex = animDef.tryGetStateIndex(newStateName.c_str());
			if (!newStateIndex.has_value())
			{
				DebugLogWarning("Missing entity animation state \"" + newStateName + "\".");
				continue;
			}

			EntityAnimationInstance &animInst = entity->getAnimInstance();
			animInst.setStateIndex(*newStateIndex);
		}
	}

	TextureManager &textureManager = game.getTextureManager();
	const std::string paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	renderer.setNightLightsActive(active, palette);
}

void GameWorldPanel::handleTriggers(const CoordInt3 &coord)
{
	auto &game = this->getGame();
	GameState &gameState = game.getGameState();
	MapInstance &mapInst = gameState.getActiveMapInst();
	LevelInstance &levelInst = mapInst.getActiveLevel();
	ChunkManager &chunkManager = levelInst.getChunkManager();
	Chunk *chunkPtr = chunkManager.tryGetChunk(coord.chunk);
	DebugAssert(chunkPtr != nullptr);

	const TriggerDefinition *triggerDef = chunkPtr->tryGetTrigger(coord.voxel);
	if (triggerDef != nullptr)
	{
		if (triggerDef->hasSoundDef())
		{
			const TriggerDefinition::SoundDef &soundDef = triggerDef->getSoundDef();
			const std::string &soundFilename = soundDef.getFilename();

			// Play the sound.
			auto &audioManager = game.getAudioManager();
			audioManager.playSound(soundFilename);
		}

		if (triggerDef->hasTextDef())
		{
			const TriggerDefinition::TextDef &textDef = triggerDef->getTextDef();
			const VoxelInt3 &voxel = coord.voxel;
			const VoxelInstance *triggerInst = chunkPtr->tryGetVoxelInst(voxel, VoxelInstance::Type::Trigger);
			const bool hasBeenDisplayed = (triggerInst != nullptr) && triggerInst->getTriggerState().isTriggered();
			const bool canDisplay = !textDef.isDisplayedOnce() || !hasBeenDisplayed;

			if (canDisplay)
			{
				// Ignore the newline at the end.
				const std::string &textDefText = textDef.getText();
				const std::string text = textDefText.substr(0, textDefText.size() - 1);

				gameState.setTriggerText(text, game.getFontLibrary(), game.getRenderer());

				// Set the text trigger as activated (regardless of whether or not it's single-shot, just
				// for consistency).
				if (triggerInst == nullptr)
				{
					constexpr bool triggered = true;
					VoxelInstance newTriggerInst = VoxelInstance::makeTrigger(voxel.x, voxel.y, voxel.z, triggered);
					chunkPtr->addVoxelInst(std::move(newTriggerInst));
				}
			}
		}
	}
}

void GameWorldPanel::handleMapTransition(const Physics::Hit &hit, const TransitionDefinition &transitionDef)
{
	const TransitionType transitionType = transitionDef.getType();
	DebugAssert(transitionType != TransitionType::LevelChange);

	DebugAssert(hit.getType() == Physics::Hit::Type::Voxel);
	const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();
	const CoordInt3 hitCoord(hit.getCoord().chunk, voxelHit.voxel);

	auto &game = this->getGame();
	auto &gameState = game.getGameState();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const MapDefinition &activeMapDef = gameState.getActiveMapDef();
	const MapType activeMapType = activeMapDef.getMapType();
	MapInstance &activeMapInst = gameState.getActiveMapInst();
	LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();

	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	DebugAssert(locationDef.getType() == LocationDefinition::Type::City);
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

	// Decide based on the active world type.
	if (activeMapType == MapType::Interior)
	{
		DebugAssert(transitionType == TransitionType::ExitInterior);

		// @temp: temporary condition while test interiors are allowed on the main menu.
		if (!gameState.isActiveMapNested())
		{
			DebugLogWarning("Test interiors have no exterior.");
			return;
		}

		// Leave the interior and go to the saved exterior.
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		if (!gameState.tryPopMap(game.getEntityDefinitionLibrary(), game.getBinaryAssetLibrary(),
			textureManager, renderer))
		{
			DebugCrash("Couldn't leave interior.");
		}

		// Change to exterior music.
		const auto &clock = gameState.getClock();
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = [&game, &gameState, &musicLibrary]()
		{
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
				return musicLibrary.getRandomMusicDefinition(MusicDefinition::Type::Night, game.getRandom());
			}
		}();

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing exterior music.");
		}

		// Only play jingle if the exterior is inside the city.
		const MusicDefinition *jingleMusicDef = nullptr;
		if (gameState.getActiveMapDef().getMapType() == MapType::City)
		{
			jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
				game.getRandom(), [&cityDef](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
				const auto &jingleMusicDef = def.getJingleMusicDefinition();
				return (jingleMusicDef.cityType == cityDef.type) &&
					(jingleMusicDef.climateType == cityDef.climateType);
			});

			if (jingleMusicDef == nullptr)
			{
				DebugLogWarning("Missing jingle music.");
			}
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef, jingleMusicDef);
	}
	else
	{
		// Either city or wilderness. If the transition is for an interior, enter it. If it's the city gates,
		// toggle between city and wilderness.
		if (transitionType == TransitionType::EnterInterior)
		{
			const CoordInt3 returnCoord = [&voxelHit, &hitCoord]()
			{
				const VoxelInt3 delta = [&voxelHit, &hitCoord]()
				{
					// Assuming this is a wall voxel.
					DebugAssert(voxelHit.facing.has_value());
					const VoxelFacing3D facing = *voxelHit.facing;

					if (facing == VoxelFacing3D::PositiveX)
					{
						return VoxelInt3(1, 0, 0);
					}
					else if (facing == VoxelFacing3D::NegativeX)
					{
						return VoxelInt3(-1, 0, 0);
					}
					else if (facing == VoxelFacing3D::PositiveZ)
					{
						return VoxelInt3(0, 0, 1);
					}
					else if (facing == VoxelFacing3D::NegativeZ)
					{
						return VoxelInt3(0, 0, -1);
					}
					else
					{
						DebugUnhandledReturnMsg(VoxelInt3, std::to_string(static_cast<int>(facing)));
					}
				}();

				return hitCoord + delta;
			}();

			const TransitionDefinition::InteriorEntranceDef &interiorEntranceDef = transitionDef.getInteriorEntrance();
			const MapGeneration::InteriorGenInfo &interiorGenInfo = interiorEntranceDef.interiorGenInfo;

			const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
			if (!gameState.tryPushInterior(interiorGenInfo, returnCoord, game.getCharacterClassLibrary(),
				game.getEntityDefinitionLibrary(), binaryAssetLibrary, textureManager, renderer))
			{
				DebugLogError("Couldn't push new interior.");
				return;
			}

			// Change to interior music.
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition::InteriorMusicDefinition::Type interiorMusicType =
				MusicUtils::getInteriorMusicType(interiorGenInfo.getInteriorType());
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicDefinition::Type::Interior, game.getRandom(), [interiorMusicType](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Interior);
				const auto &interiorMusicDef = def.getInteriorMusicDefinition();
				return interiorMusicDef.type == interiorMusicType;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing interior music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);
		}
		else if (transitionType == TransitionType::CityGate)
		{
			// City gate transition.
			const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
			const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
			const LocationDefinition &locationDef = gameState.getLocationDefinition();
			const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
			const int currentDay = gameState.getDate().getDay();
			const int starCount = SkyUtils::getStarCountFromDensity(game.getOptions().getMisc_StarDensity());

			if (activeMapType == MapType::City)
			{
				// From city to wilderness. Use the gate position to determine where to put the player.

				// The voxel face that was hit determines where to put the player relative to the gate.
				const VoxelInt2 transitionDir = [&voxelHit]()
				{
					// Assuming this is a wall voxel.
					DebugAssert(voxelHit.facing.has_value());
					const VoxelFacing3D facing = *voxelHit.facing;

					if (facing == VoxelFacing3D::PositiveX)
					{
						return VoxelUtils::North;
					}
					else if (facing == VoxelFacing3D::NegativeX)
					{
						return VoxelUtils::South;
					}
					else if (facing == VoxelFacing3D::PositiveZ)
					{
						return VoxelUtils::East;
					}
					else if (facing == VoxelFacing3D::NegativeZ)
					{
						return VoxelUtils::West;
					}
					else
					{
						DebugUnhandledReturnMsg(VoxelInt2, std::to_string(static_cast<int>(facing)));
					}
				}();

				const auto &exeData = binaryAssetLibrary.getExeData();
				Buffer2D<ArenaWildUtils::WildBlockID> wildBlockIDs =
					ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, exeData.wild);

				MapGeneration::WildGenInfo wildGenInfo;
				wildGenInfo.init(std::move(wildBlockIDs), cityDef, cityDef.citySeed);

				SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
				skyGenInfo.init(cityDef.climateType, weatherDef, currentDay, starCount, cityDef.citySeed,
					cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

				// Use current weather.
				const WeatherDefinition &overrideWeather = weatherDef;

				// Calculate wilderness position based on the gate's voxel in the city.
				const CoordInt3 startCoord = [&hitCoord, &transitionDir]()
				{
					// Origin of the city in the wilderness.
					const ChunkInt2 wildCityChunk(ArenaWildUtils::CITY_ORIGIN_CHUNK_X, ArenaWildUtils::CITY_ORIGIN_CHUNK_Z);

					// Player position bias based on selected gate face.
					const VoxelInt3 offset(transitionDir.x, 0, transitionDir.y);

					return CoordInt3(wildCityChunk + hitCoord.chunk, hitCoord.voxel + offset);
				}();

				// No need to change world map location here.
				const std::optional<GameState::WorldMapLocationIDs> worldMapLocationIDs;

				if (!gameState.trySetWilderness(wildGenInfo, skyGenInfo, overrideWeather, startCoord,
					worldMapLocationIDs, game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
					binaryAssetLibrary, textureManager, renderer))
				{
					DebugLogError("Couldn't switch from city to wilderness for \"" + locationDef.getName() + "\".");
					return;
				}
			}
			else if (activeMapType == MapType::Wilderness)
			{
				// From wilderness to city.
				Buffer<uint8_t> reservedBlocks = [&cityDef]()
				{
					DebugAssert(cityDef.reservedBlocks != nullptr);
					Buffer<uint8_t> buffer(static_cast<int>(cityDef.reservedBlocks->size()));
					std::copy(cityDef.reservedBlocks->begin(), cityDef.reservedBlocks->end(), buffer.get());
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
				cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
					cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
					cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
					mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY,
					cityDef.cityBlocksPerSide);

				SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
				skyGenInfo.init(cityDef.climateType, weatherDef, currentDay, starCount, cityDef.citySeed,
					cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

				// Use current weather.
				const WeatherDefinition &overrideWeather = weatherDef;

				// No need to change world map location here.
				const std::optional<GameState::WorldMapLocationIDs> worldMapLocationIDs;

				if (!gameState.trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
					game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), binaryAssetLibrary,
					game.getTextAssetLibrary(), textureManager, renderer))
				{
					DebugLogError("Couldn't switch from wilderness to city for \"" + locationDef.getName() + "\".");
					return;
				}
			}
			else
			{
				DebugLogError("Map type \"" + std::to_string(static_cast<int>(activeMapType)) +
					"\" does not support city gate transitions.");
				return;
			}

			// Reset the current music (even if it's the same one).
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = [&game, &gameState, &musicLibrary]()
			{
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

			// Only play jingle when going wilderness to city.
			const MusicDefinition *jingleMusicDef = nullptr;
			if (activeMapType == MapType::Wilderness)
			{
				jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
					game.getRandom(), [&cityDef](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
					const auto &jingleMusicDef = def.getJingleMusicDefinition();
					return (jingleMusicDef.cityType == cityDef.type) &&
						(jingleMusicDef.climateType == cityDef.climateType);
				});

				if (jingleMusicDef == nullptr)
				{
					DebugLogWarning("Missing jingle music.");
				}
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef, jingleMusicDef);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(transitionType)));
		}
	}
}

void GameWorldPanel::handleLevelTransition(const CoordInt3 &playerCoord, const CoordInt3 &transitionCoord)
{
	auto &game = this->getGame();
	auto &gameState = game.getGameState();

	// Level transitions are always between interiors.
	const MapDefinition &interiorMapDef = gameState.getActiveMapDef();
	DebugAssert(interiorMapDef.getMapType() == MapType::Interior);

	MapInstance &interiorMapInst = gameState.getActiveMapInst();
	const LevelInstance &level = interiorMapInst.getActiveLevel();
	const ChunkManager &chunkManager = level.getChunkManager();
	const Chunk *chunkPtr = chunkManager.tryGetChunk(transitionCoord.chunk);
	DebugAssert(chunkPtr != nullptr);

	const VoxelInt3 &transitionVoxel = transitionCoord.voxel;
	if (!chunkPtr->isValidVoxel(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z))
	{
		// Not in the chunk.
		return;
	}

	// Get the voxel definition associated with the voxel.
	const VoxelDefinition &voxelDef = [chunkPtr, &transitionVoxel]()
	{
		const Chunk::VoxelID voxelID = chunkPtr->getVoxel(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z);
		return chunkPtr->getVoxelDef(voxelID);
	}();

	// If the associated voxel data is a wall, then it might be a transition voxel.
	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		const TransitionDefinition *transitionDef = chunkPtr->tryGetTransition(transitionCoord.voxel);
		if (transitionDef != nullptr)
		{
			// The direction from a level up/down voxel to where the player should end up after
			// going through. In other words, it points to the destination voxel adjacent to the
			// level up/down voxel.
			const VoxelDouble3 dirToNewVoxel = [&playerCoord, &transitionCoord]()
			{
				const VoxelInt3 diff = transitionCoord - playerCoord;

				// @todo: this probably isn't robust enough. Maybe also check the player's angle
				// of velocity with angles to the voxel's corners to get the "arrival vector"
				// and thus the "near face" that is intersected, because this method doesn't
				// handle the player coming in at a diagonal.

				// Check which way the player is going and get the reverse of it.
				if (diff.x > 0)
				{
					// From south to north.
					return -Double3::UnitX;
				}
				else if (diff.x < 0)
				{
					// From north to south.
					return Double3::UnitX;
				}
				else if (diff.z > 0)
				{
					// From west to east.
					return -Double3::UnitZ;
				}
				else if (diff.z < 0)
				{
					// From east to west.
					return Double3::UnitZ;
				}
				else
				{
					throw DebugException("Bad player transition voxel.");
				}
			}();

			// Player destination after going through a level up/down voxel.
			auto &player = gameState.getPlayer();
			const VoxelDouble3 transitionVoxelCenter = VoxelUtils::getVoxelCenter(transitionCoord.voxel);
			const CoordDouble3 destinationCoord = ChunkUtils::recalculateCoord(
				transitionCoord.chunk, transitionVoxelCenter + dirToNewVoxel);

			// Lambda for transitioning the player to the given level.
			auto switchToLevel = [&game, &gameState, &interiorMapDef, &interiorMapInst, &player, &destinationCoord,
				&dirToNewVoxel](int levelIndex)
			{
				// Clear all open doors and fading voxels in the level the player is switching away from.
				// @todo: why wouldn't it just clear them when it gets switched to in setActive()?
				auto &oldActiveLevel = interiorMapInst.getActiveLevel();
				
				// @todo: find a modern equivalent for this w/ either the LevelInstance or ChunkManager.
				//oldActiveLevel.clearTemporaryVoxelInstances();

				// Select the new level.
				interiorMapInst.setActiveLevelIndex(levelIndex, interiorMapDef);

				// Set the new level active in the renderer.
				auto &newActiveLevel = interiorMapInst.getActiveLevel();
				auto &newActiveSky = interiorMapInst.getActiveSky();

				WeatherDefinition weatherDef;
				weatherDef.initClear();

				const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo; // Not used with interiors.

				// @todo: should this be called differently so it doesn't badly influence data for the rest of
				// this frame? Level changing should be done earlier I think.
				auto &textureManager = game.getTextureManager();
				auto &renderer = game.getRenderer();
				if (!newActiveLevel.trySetActive(weatherDef, gameState.nightLightsAreActive(), levelIndex,
					interiorMapDef, citizenGenInfo, textureManager, renderer))
				{
					DebugCrash("Couldn't set new level active in renderer.");
				}

				if (!newActiveSky.trySetActive(levelIndex, interiorMapDef, textureManager, renderer))
				{
					DebugCrash("Couldn't set new sky active in renderer.");
				}

				// Move the player to where they should be in the new level.
				const VoxelDouble3 playerDestinationPoint(
					destinationCoord.point.x,
					newActiveLevel.getCeilingScale() + Player::HEIGHT,
					destinationCoord.point.z);
				const CoordDouble3 playerDestinationCoord(destinationCoord.chunk, playerDestinationPoint);
				player.teleport(playerDestinationCoord);
				player.lookAt(player.getPosition() + dirToNewVoxel);
				player.setVelocityToZero();

				EntityGeneration::EntityGenInfo entityGenInfo;
				entityGenInfo.init(gameState.nightLightsAreActive());

				// Tick the level's chunk manager once during initialization so the renderer is passed valid
				// chunks this frame.
				constexpr double dummyDeltaTime = 0.0;
				const int chunkDistance = game.getOptions().getMisc_ChunkDistance();
				newActiveLevel.update(dummyDeltaTime, game, player.getPosition(), levelIndex, interiorMapDef,
					entityGenInfo, citizenGenInfo, chunkDistance, game.getEntityDefinitionLibrary(),
					game.getBinaryAssetLibrary(), game.getTextureManager(), game.getAudioManager());
			};

			// Lambda for opening the world map when the player enters a transition voxel
			// that will "lead to the surface of the dungeon".
			auto switchToWorldMap = [&playerCoord, &game, &player]()
			{
				// Move player to center of previous voxel in case they change their mind
				// about fast traveling. Don't change their direction.
				const VoxelInt2 playerVoxelXZ(playerCoord.voxel.x, playerCoord.voxel.z);
				const VoxelDouble2 playerVoxelCenterXZ = VoxelUtils::getVoxelCenter(playerVoxelXZ);
				const VoxelDouble3 playerDestinationPoint(
					playerVoxelCenterXZ.x,
					player.getPosition().point.y,
					playerVoxelCenterXZ.y);
				const CoordDouble3 playerDestinationCoord(playerCoord.chunk, playerDestinationPoint);
				player.teleport(playerDestinationCoord);
				player.setVelocityToZero();

				game.setPanel<WorldMapPanel>(game, nullptr);
			};

			// See if it's a level up or level down transition. Ignore other transition types.
			if (transitionDef->getType() == TransitionType::LevelChange)
			{
				const TransitionDefinition::LevelChangeDef &levelChangeDef = transitionDef->getLevelChange();
				if (levelChangeDef.isLevelUp)
				{
					// Level up transition. If the custom function has a target, call it and reset it (necessary
					// for main quest start dungeon).
					auto &onLevelUpVoxelEnter = gameState.getOnLevelUpVoxelEnter();

					if (onLevelUpVoxelEnter)
					{
						onLevelUpVoxelEnter(game);
						onLevelUpVoxelEnter = std::function<void(Game&)>();
					}
					else if (interiorMapInst.getActiveLevelIndex() > 0)
					{
						// Decrement the world's level index and activate the new level.
						switchToLevel(interiorMapInst.getActiveLevelIndex() - 1);
					}
					else
					{
						switchToWorldMap();
					}
				}
				else
				{
					// Level down transition.
					if (interiorMapInst.getActiveLevelIndex() < (interiorMapInst.getLevelCount() - 1))
					{
						// Increment the world's level index and activate the new level.
						switchToLevel(interiorMapInst.getActiveLevelIndex() + 1);
					}
					else
					{
						switchToWorldMap();
					}
				}
			}
		}
	}
}

void GameWorldPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	auto &game = this->getGame();
	const Texture tooltip = Panel::createTooltip(
		text,
		GameWorldUiView::TooltipFontName,
		game.getFontLibrary(),
		renderer);
	
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
	this->handlePlayerTurning(dt, mouseDelta);
	this->handlePlayerMovement(dt);

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
		this->handleNightLightChange(true);
	}
	else if (deactivateNightLights)
	{
		this->handleNightLightChange(false);
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
	this->handlePlayerAttack(mouseDelta);

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
		this->handleTriggers(newPlayerVoxelCoord);

		if (mapType == MapType::Interior)
		{
			this->handleLevelTransition(oldPlayerVoxelCoord, newPlayerVoxelCoord);
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
		renderer.drawOriginal(this->playerNameTextBox->getTexture(),
			this->playerNameTextBox->getX(), this->playerNameTextBox->getY());
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
		const Texture *triggerTextTexture;
		gameState.getTriggerTextRenderInfo(&triggerTextTexture);

		const TextureBuilderRef gameWorldInterfaceTextureBuilderRef =
			textureManager.getTextureBuilderRef(gameWorldInterfaceTextureBuilderID);
		const Int2 textPosition = GameWorldUiView::getTriggerTextPosition(
			game, triggerTextTexture->getWidth(), triggerTextTexture->getHeight(),
			gameWorldInterfaceTextureBuilderRef.getHeight());

		renderer.drawOriginal(*triggerTextTexture, textPosition.x, textPosition.y);
	}

	if (gameState.actionTextIsVisible())
	{
		const Texture *actionTextTexture;
		gameState.getActionTextRenderInfo(&actionTextTexture);

		const Int2 textPosition = GameWorldUiView::getActionTextPosition(actionTextTexture->getWidth());
		renderer.drawOriginal(*actionTextTexture, textPosition.x, textPosition.y);
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
