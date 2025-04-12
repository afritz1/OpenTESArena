#include "PlayerLogicController.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Collision/ArenaSelectionUtils.h"
#include "../Collision/Physics.h"
#include "../Collision/RayCastTypes.h"
#include "../Collision/SelectionUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Interface/GameWorldUiModel.h"
#include "../Interface/GameWorldUiView.h"
#include "../Items/ArenaItemUtils.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../UI/TextBox.h"
#include "../Voxels/ArenaVoxelUtils.h"
#include "../World/CardinalDirection.h"
#include "../World/CardinalDirectionName.h"
#include "../World/MapLogicController.h"

#include "components/utilities/String.h"

namespace PlayerLogicController
{
	void handlePlayerMovementClassic(Player &player, double dt, double walkSpeed, bool isOnGround, bool canJump, bool isGhostModeEnabled,
		const InputManager &inputManager, BufferView<const Rect> nativeCursorRegions)
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

		// Get some relevant player direction data (getDirection() isn't necessary here because the Y component is intentionally truncated).
		const Double3 groundDirection = player.getGroundDirection();
		const Double3 rightDirection = player.right;

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
			Double3 accelDirection = Double3::Zero;
			if (topLeft.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection;
				percent = 1.0 - (static_cast<double>(mouseY) / topLeft.getHeight());
			}
			else if (top.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection;
				percent = 1.0 - (static_cast<double>(mouseY) / top.getHeight());
			}
			else if (topRight.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection;
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
				accelDirection = accelDirection - groundDirection;
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
					if (canJump)
					{
						player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
					}
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
			Double3 accelDirection = Double3::Zero;

			if (forward)
			{
				accelDirection = accelDirection + groundDirection;
			}

			if (backward)
			{
				accelDirection = accelDirection - groundDirection;
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
				if (canJump)
				{
					player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
				}
			}
			// Change the player's velocity if valid.
			else if (std::isfinite(accelDirection.length()))
			{
				player.accelerate(accelDirection, accelMagnitude, dt);
			}
		}
		else if (isOnGround)
		{
			player.setPhysicsVelocity(Double3::Zero);
		}
	}

	void handlePlayerMovementModern(Player &player, double dt, double walkSpeed, bool isOnGround, bool canJump, bool isGhostModeEnabled,
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
		const Double3 direction = player.forward;
		const Double3 groundDirection = player.getGroundDirection();
		const Double3 rightDirection = player.right;
		const Double3 upDirection = rightDirection.cross(direction).normalized();

		if (!isGhostModeEnabled)
		{
			if (isOnGround)
			{
				if (forward || backward || left || right || jump)
				{
					// Check for jumping first so the player can't slide jump on the first frame.
					if (jump)
					{
						if (canJump)
						{
							player.accelerateInstant(Double3::UnitY, player.getJumpMagnitude());
						}
					}
					else
					{
						Double3 accelDirection = Double3::Zero;
						if (forward)
						{
							accelDirection = accelDirection + groundDirection;
						}

						if (backward)
						{
							accelDirection = accelDirection - groundDirection;
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
				else
				{
					player.setPhysicsVelocity(Double3::Zero);
				}
			}
		}
		else
		{
			// Ghost movement.
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

				const WorldDouble3 playerFeetPosition = player.getFeetPosition();
				const WorldDouble3 deltaPosition = accelDirection * (PlayerConstants::GHOST_MODE_SPEED * dt);
				const WorldDouble3 newPlayerFeetPosition = playerFeetPosition + deltaPosition;
				player.setPhysicsPositionRelativeToFeet(newPlayerFeetPosition);
			}
		}		
	}

	int getMeleeAnimDirectionStateIndex(const WeaponAnimationDefinition &animDef, CardinalDirectionName direction)
	{
		int stateIndex = -1;
		if (direction == CardinalDirectionName::North)
		{
			animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_FORWARD.c_str(), &stateIndex);
		}
		else if (direction == CardinalDirectionName::NorthEast)
		{
			animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_RIGHT.c_str(), &stateIndex);
		}
		else if (direction == CardinalDirectionName::East)
		{
			animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_RIGHT.c_str(), &stateIndex);
		}
		else if (direction == CardinalDirectionName::SouthEast)
		{
			animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_DOWN_RIGHT.c_str(), &stateIndex);
		}
		else if (direction == CardinalDirectionName::South)
		{
			animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_DOWN.c_str(), &stateIndex);
		}
		else if (direction == CardinalDirectionName::SouthWest)
		{
			animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_DOWN_LEFT.c_str(), &stateIndex);
		}
		else if (direction == CardinalDirectionName::West)
		{
			animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_LEFT.c_str(), &stateIndex);
		}
		else if (direction == CardinalDirectionName::NorthWest)
		{
			animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_LEFT.c_str(), &stateIndex);
		}

		return stateIndex;
	}
}

Double2 PlayerLogicController::makeTurningAngularValues(Game &game, double dt, BufferView<const Rect> nativeCursorRegions)
{
	const auto &inputManager = game.inputManager;

	const auto &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();
	if (!modernInterface)
	{
		// Classic interface mode.
		auto &player = game.player;
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

		const Player &player = game.player;
		const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
		const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
		const WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
		const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
		const bool turning = ((dx != 0) || (dy != 0)) && (WeaponAnimationUtils::isSheathed(weaponAnimDefState) || !rightClick);

		if (turning)
		{
			const Int2 dimensions = game.renderer.getWindowDimensions();

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

void PlayerLogicController::handlePlayerMovement(Game &game, double dt, BufferView<const Rect> nativeCursorRegions)
{
	const InputManager &inputManager = game.inputManager;
	const JPH::PhysicsSystem &physicsSystem = game.physicsSystem;

	Player &player = game.player;
	const double maxWalkSpeed = PlayerConstants::MOVE_SPEED;
	const bool isOnGround = player.onGround(physicsSystem);
	const bool canJump = player.canJump(physicsSystem);

	const Options &options = game.options;
	const bool isGhostModeEnabled = options.getMisc_GhostMode();
	const bool modernInterface = options.getGraphics_ModernInterface();
	if (!modernInterface)
	{
		PlayerLogicController::handlePlayerMovementClassic(player, dt, maxWalkSpeed, isOnGround, canJump, isGhostModeEnabled, inputManager, nativeCursorRegions);
	}
	else
	{
		PlayerLogicController::handlePlayerMovementModern(player, dt, maxWalkSpeed, isOnGround, canJump, isGhostModeEnabled, inputManager);
	}
}

void PlayerLogicController::handlePlayerAttack(Game &game, const Int2 &mouseDelta)
{
	Player &player = game.player;
	WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(player.weaponAnimDefID);
	const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
	if (!WeaponAnimationUtils::isIdle(weaponAnimDefState))
	{
		return;
	}

	const InputManager &inputManager = game.inputManager;
	const Options &options = game.options;

	const bool isAttackMouseButtonDown = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);
	const int weaponAnimIdleStateIndex = weaponAnimInst.currentStateIndex;
	int newStateIndex = weaponAnimIdleStateIndex;
	int nextStateIndex = -1;
	const char *sfxFilename = nullptr;

	if (!ArenaItemUtils::isRangedWeapon(player.weaponAnimDefID))
	{
		const Int2 dimensions = game.renderer.getWindowDimensions();

		// Get smaller screen dimension so mouse delta is relative to a square.
		const int minDimension = std::min(dimensions.x, dimensions.y);

		// @todo: this isn't frame-rate-independent, maybe need to track last 1/30 seconds of mouse positions?
		const double mouseDeltaXPercent = static_cast<double>(mouseDelta.x) / static_cast<double>(minDimension);
		const double mouseDeltaYPercent = static_cast<double>(mouseDelta.y) / static_cast<double>(minDimension);
		const double mouseDistancePercent = std::sqrt((mouseDeltaXPercent * mouseDeltaXPercent) + (mouseDeltaYPercent * mouseDeltaYPercent));
		constexpr double requiredDistancePercent = 0.060;
		const bool isMouseDeltaFastEnough = mouseDistancePercent >= requiredDistancePercent;
		if (isAttackMouseButtonDown && isMouseDeltaFastEnough)
		{
			const Double2 mouseDirection = Double2(mouseDeltaXPercent, -mouseDeltaYPercent).normalized();
			CardinalDirectionName cardinalDirection = CardinalDirection::getDirectionName(Double2(-mouseDirection.y, -mouseDirection.x));

			newStateIndex = PlayerLogicController::getMeleeAnimDirectionStateIndex(weaponAnimDef, cardinalDirection);
			nextStateIndex = weaponAnimIdleStateIndex;
			sfxFilename = ArenaSoundName::Swish;
		}
	}
	else
	{
		bool isAttack = false;
		if (options.getGraphics_ModernInterface())
		{
			isAttack = isAttackMouseButtonDown;
		}
		else
		{
			// Cursor must be above game world interface. In the original game, it has to be an "X", but relaxing that here.
			auto &textureManager = game.textureManager;
			auto &renderer = game.renderer;
			const TextureAsset gameWorldInterfaceTextureAsset = GameWorldUiView::getGameWorldInterfaceTextureAsset();
			const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(gameWorldInterfaceTextureAsset.filename.c_str());
			if (!metadataID.has_value())
			{
				DebugCrash("Couldn't get game world interface metadata ID for \"" + gameWorldInterfaceTextureAsset.filename + "\".");
			}

			const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);
			const int gameWorldInterfaceHeight = metadata.getHeight(0);
			const int originalCursorY = renderer.nativeToOriginal(inputManager.getMousePosition()).y;
			isAttack = isAttackMouseButtonDown && (originalCursorY < (ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceHeight));
		}

		if (isAttack)
		{
			weaponAnimDef.tryGetStateIndex(WeaponAnimationUtils::STATE_FIRING.c_str(), &newStateIndex);
			nextStateIndex = weaponAnimIdleStateIndex;
			sfxFilename = ArenaSoundName::ArrowFire;
		}
	}

	if (newStateIndex != weaponAnimIdleStateIndex)
	{
		weaponAnimInst.setStateIndex(newStateIndex);
		weaponAnimInst.setNextStateIndex(nextStateIndex);

		if (sfxFilename != nullptr)
		{
			AudioManager &audioManager = game.audioManager;
			audioManager.playSound(sfxFilename);
		}
	}
}

void PlayerLogicController::handleScreenToWorldInteraction(Game &game, const Int2 &nativePoint,
	bool primaryInteraction, bool debugFadeVoxel, TextBox &actionTextBox)
{
	const auto &options = game.options;
	auto &gameState = game.gameState;
	const MapDefinition &mapDef = gameState.getActiveMapDef();
	SceneManager &sceneManager = game.sceneManager;
	VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	const double ceilingScale = gameState.getActiveCeilingScale();

	auto &player = game.player;
	const Double3 &cameraDirection = player.forward;
	const CoordDouble3 rayStart = player.getEyeCoord();
	const VoxelDouble3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, nativePoint);
	constexpr bool includeEntities = true;

	const CollisionChunkManager &collisionChunkManager = sceneManager.collisionChunkManager;

	RayCastHit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
		includeEntities, voxelChunkManager, entityChunkManager, collisionChunkManager,
		EntityDefinitionLibrary::getInstance(), hit);

	// See if the ray hit anything.
	if (success)
	{
		if (hit.type == RayCastHitType::Voxel)
		{
			const ChunkInt2 chunkPos = hit.coord.chunk;
			VoxelChunk &chunk = voxelChunkManager.getChunkAtPosition(chunkPos);
			const RayCastVoxelHit &voxelHit = hit.voxelHit;
			const VoxelInt3 &voxel = voxelHit.voxel;
			const VoxelTraitsDefID voxelTraitsDefID = chunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
			const VoxelTraitsDefinition &voxelTraitsDef = chunk.getTraitsDef(voxelTraitsDefID);
			const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

			// Primary interaction handles selection in the game world. Secondary interaction handles
			// reading names of things.
			if (primaryInteraction)
			{
				// Arbitrary max distance for selection.
				// @todo: move to some ArenaPlayerUtils maybe
				if (hit.t <= SelectionUtils::MAX_PRIMARY_INTERACTION_DISTANCE)
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
								VoxelTransitionDefID transitionDefID;
								if (chunk.tryGetTransitionDefID(voxel.x, voxel.y, voxel.z, &transitionDefID))
								{
									const TransitionDefinition &transitionDef = chunk.getTransitionDef(transitionDefID);
									if (transitionDef.type != TransitionType::InteriorLevelChange)
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
							VoxelDoorDefID doorDefID;
							if (!chunk.tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID))
							{
								DebugCrash("Expected door def ID to exist.");
							}

							const VoxelDoorDefinition &doorDef = chunk.getDoorDef(doorDefID);
							const VoxelDoorOpenSoundDefinition &openSoundDef = doorDef.openSoundDef;

							auto &audioManager = game.audioManager;
							const std::string &soundFilename = openSoundDef.soundFilename;

							const CoordDouble3 soundCoord(chunk.getPosition(), VoxelUtils::getVoxelCenter(voxel, ceilingScale));
							const WorldDouble3 soundPosition = VoxelUtils::coordToWorldPoint(soundCoord);
							audioManager.playSound(soundFilename.c_str(), soundPosition);
						}
					}
				}
			}
			else
			{
				// Handle secondary click (i.e. right click).
				if (ArenaSelectionUtils::isVoxelSelectableAsSecondary(voxelType))
				{
					VoxelBuildingNameID buildingNameID;
					if (chunk.tryGetBuildingNameID(voxel.x, voxel.y, voxel.z, &buildingNameID))
					{
						const std::string &buildingName = chunk.getBuildingName(buildingNameID);
						actionTextBox.setText(buildingName);

						auto &gameState = game.gameState;
						gameState.setActionTextDuration(buildingName);
					}
				}
			}
		}
		else if (hit.type == RayCastHitType::Entity)
		{
			const RayCastEntityHit &entityHit = hit.entityHit;
			const auto &exeData = BinaryAssetLibrary::getInstance().getExeData();

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
				const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
				const auto &charClassLibrary = CharacterClassLibrary::getInstance();

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

				auto &gameState = game.gameState;
				gameState.setActionTextDuration(text);
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(hit.type)));
		}
	}
}
