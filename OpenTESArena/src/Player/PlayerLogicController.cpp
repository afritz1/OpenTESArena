#include "PlayerLogicController.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Collision/ArenaSelectionUtils.h"
#include "../Collision/Physics.h"
#include "../Collision/RayCastTypes.h"
#include "../Collision/SelectionUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Interface/GameWorldUiController.h"
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
		if (!isOnGround)
		{
			return;
		}

		const Double3 groundDirection = player.getGroundDirection();
		const Double3 rightDirection = player.right;

		const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);
		const bool forward = inputManager.keyIsDown(SDL_SCANCODE_W);
		const bool backward = inputManager.keyIsDown(SDL_SCANCODE_S);
		const bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		const bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		const bool space = inputManager.keyIsDown(SDL_SCANCODE_SPACE);
		const bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		const bool anyMouseMovementInput = leftClick;
		const bool anyKeyboardMovementInput = forward || backward || ((left || right) && lCtrl) || space;

		// Mouse movement takes priority.
		if (anyMouseMovementInput)
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
			if (accelDirection.lengthSquared() == 0.0)
			{
				player.setPhysicsVelocity(Double3::Zero);
				return;
			}

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
		else if (anyKeyboardMovementInput)
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
		else
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

	void handleRayCastHitVoxel(Game &game, const RayCastHit &hit, bool isPrimaryInteraction, bool debugDestroyVoxel, double ceilingScale, VoxelChunk &voxelChunk, TextBox &actionTextBox)
	{
		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const ExeData &exeData = binaryAssetLibrary.getExeData();

		const RayCastVoxelHit &voxelHit = hit.voxelHit;
		const VoxelInt3 &voxel = voxelHit.voxel;
		const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.getTraitsDefID(voxel.x, voxel.y, voxel.z);
		const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.getTraitsDef(voxelTraitsDefID);
		const ArenaTypes::VoxelType voxelType = voxelTraitsDef.type;

		GameState &gameState = game.gameState;
		const Player &player = game.player;

		if (isPrimaryInteraction)
		{
			// Arbitrary max distance for selection.
			// @todo: move to some ArenaPlayerUtils maybe
			if (hit.t <= SelectionUtils::MAX_PRIMARY_INTERACTION_DISTANCE)
			{
				if (ArenaSelectionUtils::isVoxelSelectableAsPrimary(voxelType))
				{
					if (!debugDestroyVoxel)
					{
						const bool isWall = voxelType == ArenaTypes::VoxelType::Wall;

						// The only edge voxels with a transition should be should be palace entrances (with collision).
						const bool isEdge = (voxelType == ArenaTypes::VoxelType::Edge) && voxelTraitsDef.edge.collider;

						if (isWall || isEdge)
						{
							VoxelTransitionDefID transitionDefID;
							if (voxelChunk.tryGetTransitionDefID(voxel.x, voxel.y, voxel.z, &transitionDefID))
							{
								const TransitionDefinition &transitionDef = voxelChunk.getTransitionDef(transitionDefID);
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
						if (!voxelChunk.tryGetFadeAnimInstIndex(voxel.x, voxel.y, voxel.z, &fadeAnimInstIndex))
						{
							VoxelFadeAnimationInstance fadeAnimInst;
							fadeAnimInst.init(voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::FADING_VOXEL_SECONDS);
							voxelChunk.addFadeAnimInst(std::move(fadeAnimInst));
						}
					}
				}
				else if (voxelType == ArenaTypes::VoxelType::Door)
				{
					// If the door is closed, try to open it.
					int doorAnimInstIndex;
					const bool isDoorClosed = !voxelChunk.tryGetDoorAnimInstIndex(voxel.x, voxel.y, voxel.z, &doorAnimInstIndex);
					if (isDoorClosed)
					{
						bool canDoorBeOpened = true;
						bool isUsingDoorKey = false;
						int requiredDoorKeyID = -1;

						VoxelLockDefID lockDefID;
						if (voxelChunk.tryGetLockDefID(voxel.x, voxel.y, voxel.z, &lockDefID))
						{
							const LockDefinition &lockDef = voxelChunk.getLockDef(lockDefID);
							requiredDoorKeyID = lockDef.keyID;

							if (requiredDoorKeyID >= 0)
							{
								if (player.isIdInKeyInventory(requiredDoorKeyID))
								{
									int triggerInstIndex;
									const bool isDoorKeyAlreadyUsed = voxelChunk.tryGetTriggerInstIndex(voxel.x, voxel.y, voxel.z, &triggerInstIndex);
									if (!isDoorKeyAlreadyUsed)
									{
										isUsingDoorKey = true;
									}
								}
								else
								{
									canDoorBeOpened = false;
								}
							}
						}

						if (canDoorBeOpened)
						{
							VoxelDoorAnimationInstance newDoorAnimInst;
							newDoorAnimInst.initOpening(voxel.x, voxel.y, voxel.z, ArenaVoxelUtils::DOOR_ANIM_SPEED);
							voxelChunk.addDoorAnimInst(std::move(newDoorAnimInst));

							VoxelDoorDefID doorDefID;
							if (!voxelChunk.tryGetDoorDefID(voxel.x, voxel.y, voxel.z, &doorDefID))
							{
								DebugCrash("Expected door def ID to exist.");
							}

							const VoxelDoorDefinition &doorDef = voxelChunk.getDoorDef(doorDefID);
							const VoxelDoorOpenSoundDefinition &openSoundDef = doorDef.openSoundDef;
							const std::string &soundFilename = openSoundDef.soundFilename;
							const CoordDouble3 soundCoord(voxelChunk.getPosition(), VoxelUtils::getVoxelCenter(voxel, ceilingScale));
							const WorldDouble3 soundPosition = VoxelUtils::coordToWorldPoint(soundCoord);

							if (isUsingDoorKey)
							{
								GameWorldUiController::onDoorUnlockedWithKey(game, requiredDoorKeyID, soundFilename, soundPosition, exeData);

								VoxelTriggerInstance newTriggerInst;
								newTriggerInst.init(voxel.x, voxel.y, voxel.z);
								voxelChunk.addTriggerInst(std::move(newTriggerInst));
							}
							else
							{
								AudioManager &audioManager = game.audioManager;
								audioManager.playSound(soundFilename.c_str(), soundPosition);
							}
						}
						else
						{
							const int lockDifficultyIndex = 0; // @todo determine from thieving skill value
							const std::string requiredDoorKeyMsg = GameWorldUiModel::getLockDifficultyMessage(lockDifficultyIndex, exeData);
							actionTextBox.setText(requiredDoorKeyMsg);
							gameState.setActionTextDuration(requiredDoorKeyMsg);
						}
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
				if (voxelChunk.tryGetBuildingNameID(voxel.x, voxel.y, voxel.z, &buildingNameID))
				{
					const std::string &buildingName = voxelChunk.getBuildingName(buildingNameID);
					actionTextBox.setText(buildingName);
					gameState.setActionTextDuration(buildingName);
				}
			}
		}
	}

	void handleRayCastHitEntity(Game &game, const RayCastHit &hit, bool isPrimaryInteraction, double ceilingScale, VoxelChunk &voxelChunk, TextBox &actionTextBox)
	{
		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const ExeData &exeData = binaryAssetLibrary.getExeData();

		const ChunkInt2 chunkPos = hit.coord.chunk;
		const RayCastEntityHit &entityHit = hit.entityHit;
		const VoxelInt3 voxel = VoxelUtils::pointToVoxel(hit.coord.point, ceilingScale);

		GameState &gameState = game.gameState;
		Player &player = game.player;
		EntityChunkManager &entityChunkManager = game.sceneManager.entityChunkManager;

		if (isPrimaryInteraction)
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
			const EntityInstanceID entityInstID = entityHit.id;
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const EntityDefinitionType entityType = entityDef.type;
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
				switch (entityType)
				{
				case EntityDefinitionType::Item:
				{
					const ItemEntityDefinition &itemDef = entityDef.item;
					const ItemEntityDefinitionType itemDefType = itemDef.type;

					// Attempt to pick up item.
					if (itemDefType == ItemEntityDefinitionType::Key)
					{
						VoxelTriggerDefID triggerDefID;
						if (voxelChunk.tryGetTriggerDefID(voxel.x, voxel.y, voxel.z, &triggerDefID))
						{
							const VoxelTriggerDefinition &triggerDef = voxelChunk.getTriggerDef(triggerDefID);
							if (triggerDef.hasKeyDef())
							{
								const VoxelTriggerKeyDefinition &triggerKeyDef = triggerDef.key;
								const int keyID = triggerKeyDef.keyID;
								GameWorldUiController::onKeyPickedUp(game, keyID, exeData);
								player.addToKeyInventory(keyID);
								entityChunkManager.queueEntityDestroy(entityInstID, &chunkPos);
							}
						}
					}
					else if (itemDefType == ItemEntityDefinitionType::QuestItem)
					{
						AudioManager &audioManager = game.audioManager;
						audioManager.playSound(ArenaSoundName::Fanfare2);
						DebugLog("Picked up quest item.");
						entityChunkManager.queueEntityDestroy(entityInstID, &chunkPos);
					}

					break;
				}
				default:
					// Placeholder text for testing.
					text = "Entity " + std::to_string(entityInstID) + " (" + EntityUtils::defTypeToString(entityDef) + ")";
					break;
				}
			}

			actionTextBox.setText(text);
			gameState.setActionTextDuration(text);
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

Double2 PlayerLogicController::makeTurningAngularValues(Game &game, double dt, const Int2 &mouseDelta, BufferView<const Rect> nativeCursorRegions)
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
		// Get smaller screen dimension so mouse delta is relative to a square.
		const Int2 dimensions = game.renderer.getWindowDimensions();
		const int minDimension = std::min(dimensions.x, dimensions.y);

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

void PlayerLogicController::handleScreenToWorldInteraction(Game &game, const Int2 &nativePoint, bool isPrimaryInteraction,
	bool debugFadeVoxel, TextBox &actionTextBox)
{
	SceneManager &sceneManager = game.sceneManager;
	VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	const EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
	const CollisionChunkManager &collisionChunkManager = sceneManager.collisionChunkManager;
	const GameState &gameState = game.gameState;
	const double ceilingScale = gameState.getActiveCeilingScale();

	const Player &player = game.player;
	const Double3 &cameraDirection = player.forward;
	const CoordDouble3 rayStart = player.getEyeCoord();
	const VoxelDouble3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, nativePoint);
	constexpr bool includeEntities = true;

	RayCastHit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
		includeEntities, voxelChunkManager, entityChunkManager, collisionChunkManager,
		EntityDefinitionLibrary::getInstance(), hit);

	if (success)
	{
		VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(hit.coord.chunk);

		if (hit.type == RayCastHitType::Voxel)
		{
			handleRayCastHitVoxel(game, hit, isPrimaryInteraction, debugFadeVoxel, ceilingScale, voxelChunk, actionTextBox);
		}
		else if (hit.type == RayCastHitType::Entity)
		{
			handleRayCastHitEntity(game, hit, isPrimaryInteraction, ceilingScale, voxelChunk, actionTextBox);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(hit.type)));
		}
	}
}
