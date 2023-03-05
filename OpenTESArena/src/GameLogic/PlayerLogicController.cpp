#include "MapLogicController.h"
#include "PlayerLogicController.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Collision/ArenaSelectionUtils.h"
#include "../Collision/Physics.h"
#include "../Collision/SelectionUtils.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Interface/GameWorldUiModel.h"
#include "../Interface/GameWorldUiView.h"
#include "../UI/TextBox.h"
#include "../Voxels/ArenaVoxelUtils.h"

#include "components/utilities/String.h"

namespace PlayerLogicController
{
	void handlePlayerMovementClassic(Player &player, double dt, double walkSpeed, bool isOnGround, bool isGhostModeEnabled,
		const InputManager &inputManager, const BufferView<const Rect> &nativeCursorRegions)
	{
		// Classic interface mode.
		// Arena uses arrow keys, but let's use the left hand side of the keyboard
		// because we like being comfortable.

		// A and D turn the player, and if Ctrl is held, the player slides instead.
		// Let's keep the turning part in the other method because turning doesn't
		// affect velocity.

		// Listen for mouse, WASD, and Ctrl.
		const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);

		const bool forward = inputManager.keyIsDown(SDL_SCANCODE_W);
		const bool backward = inputManager.keyIsDown(SDL_SCANCODE_S);
		const bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		const bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		const bool space = inputManager.keyIsDown(SDL_SCANCODE_SPACE);
		const bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		// Get some relevant player direction data (getDirection() isn't necessary here
		// because the Y component is intentionally truncated).
		const Double2 groundDirection = player.getGroundDirection();
		const Double3 groundDirection3D = Double3(groundDirection.x, 0.0, groundDirection.y).normalized();
		const Double3 &rightDirection = player.getRight();

		// Mouse movement takes priority over key movement.
		if (leftClick && isOnGround)
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
				double accelMagnitude = percent * walkSpeed;

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
					player.accelerate(accelDirection, accelMagnitude, dt);
				}
			}
		}
		else if ((forward || backward || ((left || right) && lCtrl) || space) && isOnGround)
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
			double accelMagnitude = walkSpeed;

			// Check for jumping first (so the player can't slide jump on the first frame).
			if (space)
			{
				// Jump.
				player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
			}
			// Change the player's velocity if valid.
			else if (std::isfinite(accelDirection.length()))
			{
				player.accelerate(accelDirection, accelMagnitude, dt);
			}
		}
		else if (isOnGround)
		{
			player.setVelocityToZero();
		}
	}

	void handlePlayerMovementModern(Player &player, double dt, double walkSpeed, bool isOnGround, bool isGhostModeEnabled,
		const InputManager &inputManager)
	{
		// Modern interface. Listen for WASD.
		const bool forward = inputManager.keyIsDown(SDL_SCANCODE_W);
		const bool backward = inputManager.keyIsDown(SDL_SCANCODE_S);
		const bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		const bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		const bool jump = inputManager.keyIsDown(SDL_SCANCODE_SPACE);
		const bool down = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		// Get some relevant player direction data (getDirection() isn't necessary here
		// because the Y component is intentionally truncated).
		const Double3 direction = player.getDirection();
		const Double2 groundDirection = player.getGroundDirection();
		const Double3 groundDirection3D = Double3(groundDirection.x, 0.0, groundDirection.y).normalized();
		const Double3 &rightDirection = player.getRight();
		const Double3 upDirection = rightDirection.cross(direction).normalized();

		if (!isGhostModeEnabled)
		{
			if ((forward || backward || left || right || jump) && isOnGround)
			{
				// Check for jumping first so the player can't slide jump on the first frame.
				if (jump)
				{
					player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
				}
				else
				{
					Double3 accelDirection = Double3::Zero;
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

					if (accelDirection.lengthSquared() > 0.0)
					{
						accelDirection = accelDirection.normalized();
						player.accelerate(accelDirection, walkSpeed, dt);
					}
				}
			}
			else if (isOnGround)
			{
				player.setVelocityToZero();
			}
		}
		else
		{
			Double3 accelDirection = Double3::Zero;
			if (forward)
			{
				accelDirection = accelDirection + direction;
			}

			if (backward)
			{
				accelDirection = accelDirection - direction;
			}

			if (right)
			{
				accelDirection = accelDirection + rightDirection;
			}

			if (left)
			{
				accelDirection = accelDirection - rightDirection;
			}

			if (jump)
			{
				accelDirection = accelDirection + upDirection;
			}

			if (down)
			{
				accelDirection = accelDirection - upDirection;
			}

			if (accelDirection.lengthSquared() > 0.0)
			{
				accelDirection = accelDirection.normalized();

				const CoordDouble3 &playerCoord = player.getPosition();

				constexpr double ghostSpeed = 10.0;
				const VoxelDouble3 deltaPoint = accelDirection * (ghostSpeed * dt);
				const CoordDouble3 newPlayerCoord = ChunkUtils::recalculateCoord(playerCoord.chunk, playerCoord.point + deltaPoint);
				player.teleport(newPlayerCoord);
			}
		}		
	}
}

Double2 PlayerLogicController::makeTurningAngularValues(Game &game, double dt,
	const BufferView<const Rect> &nativeCursorRegions)
{
	const auto &inputManager = game.getInputManager();

	const auto &options = game.getOptions();
	const bool modernInterface = options.getGraphics_ModernInterface();
	if (!modernInterface)
	{
		// Classic interface mode.
		auto &player = game.getPlayer();
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
			return Double2(dx * dt, 0.0);
		}
		else if (!lCtrl)
		{
			// If left control is not held, then turning is permitted.
			if (left)
			{
				// Turn left at a fixed angular velocity.
				return Double2(-dt, 0.0);
			}
			else if (right)
			{
				// Turn right at a fixed angular velocity.
				return Double2(dt, 0.0);
			}
		}
	}
	else
	{
		// Modern interface. Make the camera look around if the player's weapon is not in use.
		const Int2 mouseDelta = inputManager.getMouseDelta();
		const int dx = mouseDelta.x;
		const int dy = mouseDelta.y;
		const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);

		auto &player = game.getPlayer();
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
			return Double2(dxx, -dyy);
		}
	}

	// No turning.
	return Double2::Zero;
}

void PlayerLogicController::turnPlayer(Game &game, double dx, double dy)
{
	const auto &options = game.getOptions();
	auto &player = game.getPlayer();
	player.rotate(dx, dy, options.getInput_HorizontalSensitivity(),
		options.getInput_VerticalSensitivity(), options.getInput_CameraPitchLimit());
}

void PlayerLogicController::handlePlayerMovement(Game &game, double dt, const BufferView<const Rect> &nativeCursorRegions)
{
	const InputManager &inputManager = game.getInputManager();

	// Arbitrary movement speed.
	constexpr double walkSpeed = 15.0;

	const GameState &gameState = game.getGameState();
	const MapInstance &mapInst = gameState.getActiveMapInst();
	const LevelInstance &levelInst = mapInst.getActiveLevel();
	Player &player = game.getPlayer();
	const bool isOnGround = player.onGround(levelInst);

	const Options &options = game.getOptions();
	const bool isGhostModeEnabled = options.getMisc_GhostMode();
	const bool modernInterface = options.getGraphics_ModernInterface();
	if (!modernInterface)
	{
		PlayerLogicController::handlePlayerMovementClassic(player, dt, walkSpeed, isOnGround, isGhostModeEnabled,
			inputManager, nativeCursorRegions);
	}
	else
	{
		PlayerLogicController::handlePlayerMovementModern(player, dt, walkSpeed, isOnGround, isGhostModeEnabled, inputManager);
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
	auto &weaponAnimation = game.getPlayer().getWeaponAnimation();
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
					// The cursor must be above the game world interface in order to fire. In the original game,
					// the cursor has to be in the center "X" region, but that seems pretty inconvenient, given
					// that the border between cursor regions is hard to see at a glance, and that might be the
					// difference between shooting an arrow and not shooting an arrow, so I'm relaxing the
					// requirements here.
					auto &textureManager = game.getTextureManager();
					auto &renderer = game.getRenderer();
					const TextureAsset gameWorldInterfaceTextureAsset = GameWorldUiView::getGameWorldInterfaceTextureAsset();
					const std::optional<TextureFileMetadataID> metadataID =
						textureManager.tryGetMetadataID(gameWorldInterfaceTextureAsset.filename.c_str());
					if (!metadataID.has_value())
					{
						DebugCrash("Couldn't get game world interface metadata ID for \"" + gameWorldInterfaceTextureAsset.filename + "\".");
					}

					const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);
					const int gameWorldInterfaceHeight = metadata.getHeight(0);
					const int originalCursorY = renderer.nativeToOriginal(inputManager.getMousePosition()).y;
					return rightClick && (originalCursorY < (ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceHeight));
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
	const auto &options = game.getOptions();
	auto &gameState = game.getGameState();
	const MapDefinition &mapDef = gameState.getActiveMapDef();
	MapInstance &mapInst = gameState.getActiveMapInst();
	LevelInstance &levelInst = mapInst.getActiveLevel();
	VoxelChunkManager &voxelChunkManager = levelInst.getVoxelChunkManager();
	const EntityChunkManager &entityChunkManager = levelInst.getEntityChunkManager();
	const double ceilingScale = levelInst.getCeilingScale();

	auto &player = game.getPlayer();
	const Double3 &cameraDirection = player.getDirection();
	const CoordDouble3 rayStart = player.getPosition();
	const VoxelDouble3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, nativePoint);
	constexpr bool includeEntities = true;

	Physics::Hit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
		includeEntities, levelInst, game.getEntityDefinitionLibrary(), game.getRenderer(), hit);

	// See if the ray hit anything.
	if (success)
	{
		if (hit.getType() == Physics::HitType::Voxel)
		{
			const ChunkInt2 chunkPos = hit.getCoord().chunk;
			VoxelChunk &chunk = voxelChunkManager.getChunkAtPosition(chunkPos);
			const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();
			const VoxelInt3 &voxel = voxelHit.voxel;
			const VoxelChunk::VoxelTraitsDefID voxelTraitsDefID = chunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
			const VoxelTraitsDefinition &voxelTraitsDef = chunk.getTraitsDef(voxelTraitsDefID);
			const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

			// Primary interaction handles selection in the game world. Secondary interaction handles
			// reading names of things.
			if (primaryInteraction)
			{
				// Arbitrary max distance for selection.
				// @todo: move to some ArenaPlayerUtils maybe
				if (hit.getT() <= SelectionUtils::MAX_DISTANCE)
				{
					if (ArenaSelectionUtils::isVoxelSelectableAsPrimary(voxelType))
					{
						if (!debugFadeVoxel)
						{
							const bool isWall = voxelType == ArenaTypes::VoxelType::Wall;

							// The only edge voxels with a transition should be should be palace entrances (with collision).
							const bool isEdge = (voxelType == ArenaTypes::VoxelType::Edge) && voxelTraitsDef.edge.collider;

							if (isWall || isEdge)
							{
								VoxelChunk::TransitionDefID transitionDefID;
								if (chunk.tryGetTransitionDefID(voxel.x, voxel.y, voxel.z, &transitionDefID))
								{
									const TransitionDefinition &transitionDef = chunk.getTransitionDef(transitionDefID);
									if (transitionDef.getType() != TransitionType::LevelChange)
									{
										MapLogicController::handleMapTransition(game, hit, transitionDef);
									}
								}
							}
						}
						else
						{
							// @temp: add to fading voxels if it doesn't already exist.
							int fadeAnimInstIndex;
							if (!chunk.tryGetFadeAnimInstIndex(voxel.x, voxel.y, voxel.z, &fadeAnimInstIndex))
							{
								VoxelFadeAnimationInstance fadeAnimInst;
								fadeAnimInst.init(voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::FADING_VOXEL_SECONDS);
								chunk.addFadeAnimInst(std::move(fadeAnimInst));
							}
						}
					}
					else if (voxelType == ArenaTypes::VoxelType::Door)
					{
						// If the door is closed, then open it.
						int doorAnimInstIndex;
						const bool isClosed = !chunk.tryGetDoorAnimInstIndex(voxel.x, voxel.y, voxel.z, &doorAnimInstIndex);
						if (isClosed)
						{
							// Add the door to the open doors list.
							VoxelDoorAnimationInstance newDoorAnimInst;
							newDoorAnimInst.initOpening(voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::DOOR_ANIM_SPEED);
							chunk.addDoorAnimInst(std::move(newDoorAnimInst));

							// Get the door's opening sound and play it at the center of the voxel.
							VoxelChunk::DoorDefID doorDefID;
							if (!chunk.tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID))
							{
								DebugCrash("Expected door def ID to exist.");
							}

							const DoorDefinition &doorDef = chunk.getDoorDef(doorDefID);
							const DoorDefinition::OpenSoundDef &openSoundDef = doorDef.getOpenSound();

							auto &audioManager = game.getAudioManager();
							const std::string &soundFilename = openSoundDef.soundFilename;

							const CoordDouble3 soundCoord(chunk.getPosition(), VoxelUtils::getVoxelCenter(voxel, ceilingScale));
							const WorldDouble3 soundPosition = VoxelUtils::coordToWorldPoint(soundCoord);
							audioManager.playSound(soundFilename, soundPosition);
						}
					}
				}
			}
			else
			{
				// Handle secondary click (i.e. right click).
				if (ArenaSelectionUtils::isVoxelSelectableAsSecondary(voxelType))
				{
					VoxelChunk::BuildingNameID buildingNameID;
					if (chunk.tryGetBuildingNameID(voxel.x, voxel.y, voxel.z, &buildingNameID))
					{
						const std::string &buildingName = chunk.getBuildingName(buildingNameID);
						actionTextBox.setText(buildingName);

						auto &gameState = game.getGameState();
						gameState.setActionTextDuration(buildingName);
					}
				}
			}
		}
		else if (hit.getType() == Physics::HitType::Entity)
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

				// Try inspecting the entity (can be from any distance). If they have a display name, then show it.
				const EntityInstance &entityInst = entityChunkManager.getEntity(entityHit.id);
				const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID, game.getEntityDefinitionLibrary());
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
					text = "Entity " + std::to_string(entityHit.id) + " (" + EntityUtils::defTypeToString(entityDef) + ")";
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
