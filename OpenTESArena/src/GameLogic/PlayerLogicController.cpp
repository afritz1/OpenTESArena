#include "MapLogicController.h"
#include "PlayerLogicController.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Game/Physics.h"
#include "../Interface/GameWorldUiModel.h"
#include "../Interface/GameWorldUiView.h"
#include "../UI/TextBox.h"
#include "../World/ArenaVoxelUtils.h"

#include "components/utilities/String.h"

void PlayerLogicController::handlePlayerTurning(Game &game, double dt, const Int2 &mouseDelta,
	const BufferView<const Rect> &nativeCursorRegions)
{
	// @todo: move this to some game/player logic controller namespace

	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicTurning()
	// 2) handleModernTurning()

	// Don't handle weapon swinging here. That can go in another method.
	// If right click is held, weapon is out, and mouse motion is significant, then
	// get the swing direction and swing.
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
			const double dx = [&mousePosition, &nativeCursorRegions]()
			{
				// Measure the magnitude of rotation. -1.0 is left, 1.0 is right.
				const double percent = [&mousePosition, &nativeCursorRegions]()
				{
					const int mouseX = mousePosition.x;

					// Native cursor regions for turning (scaled to the current window).
					const Rect &topLeft = nativeCursorRegions.get(GameWorldUiView::CursorTopLeftIndex);
					const Rect &topRight = nativeCursorRegions.get(GameWorldUiView::CursorTopRightIndex);
					const Rect &middleLeft = nativeCursorRegions.get(GameWorldUiView::CursorMiddleLeftIndex);
					const Rect &middleRight = nativeCursorRegions.get(GameWorldUiView::CursorMiddleRightIndex);

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

void PlayerLogicController::handlePlayerMovement(Game &game, double dt,
	const BufferView<const Rect> &nativeCursorRegions)
{
	// @todo: this should be in some game/player logic controller namespace

	// In the future, maybe this could be separated into two methods:
	// 1) handleClassicMovement()
	// 2) handleModernMovement()

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
			const Rect &topLeft = nativeCursorRegions.get(GameWorldUiView::CursorTopLeftIndex);
			const Rect &top = nativeCursorRegions.get(GameWorldUiView::CursorTopMiddleIndex);
			const Rect &topRight = nativeCursorRegions.get(GameWorldUiView::CursorTopRightIndex);
			const Rect &bottomLeft = nativeCursorRegions.get(GameWorldUiView::CursorBottomLeftIndex);
			const Rect &bottom = nativeCursorRegions.get(GameWorldUiView::CursorBottomMiddleIndex);
			const Rect &bottomRight = nativeCursorRegions.get(GameWorldUiView::CursorBottomRightIndex);

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

void PlayerLogicController::handlePlayerAttack(Game &game, const Int2 &mouseDelta)
{
	// @todo: run this method at fixed time-steps instead of every frame, because if,
	// for example, the game is running at 200 fps, then the player has to move their
	// cursor much faster for it to count as a swing. The GameWorldPanel would probably
	// need to save its own "swing" mouse delta independently of the input manager, or
	// maybe the game loop could call a "Panel::fixedTick()" method.

	// Only handle attacking if the player's weapon is currently idle.
	auto &weaponAnimation = game.getGameState().getPlayer().getWeaponAnimation();
	if (weaponAnimation.isIdle())
	{
		const auto &inputManager = game.getInputManager();
		auto &audioManager = game.getAudioManager();

		if (!weaponAnimation.isRanged())
		{
			// Handle melee attack.
			const Int2 dimensions = game.getRenderer().getWindowDimensions();

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
			const bool isAttack = [&game, &inputManager]()
			{
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

void PlayerLogicController::handleScreenToWorldInteraction(Game &game, const Int2 &nativePoint,
	bool primaryInteraction, bool debugFadeVoxel, TextBox &actionTextBox)
{
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

			// Primary interaction handles selection in the game world. Secondary interaction handles
			// reading names of things.
			if (primaryInteraction)
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
									MapLogicController::handleMapTransition(game, hit, *transitionDef);
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
						actionTextBox.setText(*buildingName);

						auto &gameState = game.getGameState();
						gameState.setActionTextDuration(*buildingName);
					}
				}
			}
		}
		else if (hit.getType() == Physics::Hit::Type::Entity)
		{
			const Physics::Hit::EntityHit &entityHit = hit.getEntityHit();
			const auto &exeData = game.getBinaryAssetLibrary().getExeData();

			if (primaryInteraction)
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

				actionTextBox.setText(text);

				auto &gameState = game.getGameState();
				gameState.setActionTextDuration(text);
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(hit.getType())));
		}
	}
}
