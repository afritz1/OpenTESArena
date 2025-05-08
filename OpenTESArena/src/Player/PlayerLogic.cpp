#include "PlayerLogic.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Collision/ArenaSelectionUtils.h"
#include "../Collision/Physics.h"
#include "../Collision/RayCastTypes.h"
#include "../Collision/SelectionUtils.h"
#include "../Combat/CombatLogic.h"
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
#include "../World/MapLogic.h"

#include "components/utilities/String.h"

namespace PlayerLogic
{
	PlayerInputAcceleration getInputAccelerationClassic(const Player &player, double moveSpeed, bool isOnGround, bool canJump, bool isClimbing,
		double ceilingScale, bool isGhostModeEnabled, const InputManager &inputManager, BufferView<const Rect> nativeCursorRegions)
	{
		PlayerInputAcceleration inputAcceleration;
		if (!isOnGround && !isClimbing)
		{
			return inputAcceleration;
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
				percent = 1.0 - (static_cast<double>(mouseY) / topLeft.height);
			}
			else if (top.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection;
				percent = 1.0 - (static_cast<double>(mouseY) / top.height);
			}
			else if (topRight.contains(mousePosition))
			{
				// Forward.
				accelDirection = accelDirection + groundDirection;
				percent = 1.0 - (static_cast<double>(mouseY) / topRight.height);
			}
			else if (bottomLeft.contains(mousePosition))
			{
				// Left.
				accelDirection = accelDirection - rightDirection;
				percent = 1.0 - (static_cast<double>(mouseX) / bottomLeft.width);
			}
			else if (bottom.contains(mousePosition))
			{
				// Backwards.
				accelDirection = accelDirection - groundDirection;
				percent = static_cast<double>(mouseY - bottom.getTop()) / bottom.height;
			}
			else if (bottomRight.contains(mousePosition))
			{
				// Right.
				accelDirection = accelDirection + rightDirection;
				percent = static_cast<double>(mouseX - bottomRight.getLeft()) / bottomRight.width;
			}

			// Only attempt to accelerate if a direction was chosen.
			if (accelDirection.lengthSquared() > 0.0)
			{
				accelDirection = accelDirection.normalized();

				// Set the magnitude of the acceleration to some arbitrary number. These values
				// are independent of max speed.
				double accelMagnitude = percent * moveSpeed;

				// Check for jumping first (so the player can't slide jump on the first frame).
				const bool rightClick = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);
				if (rightClick)
				{
					if (canJump)
					{
						inputAcceleration.direction = Double3::UnitY;
						inputAcceleration.magnitude = player.getJumpMagnitude();
						inputAcceleration.isInstantJump = true;
					}
				}
				else if (std::isfinite(accelDirection.length()) && std::isfinite(accelMagnitude))
				{
					inputAcceleration.direction = accelDirection;
					inputAcceleration.magnitude = accelMagnitude;
				}
			}
			else if (!isClimbing)
			{
				inputAcceleration.shouldResetVelocity = true;
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
			double accelMagnitude = moveSpeed;

			// Check for jumping first (so the player can't slide jump on the first frame).
			if (space)
			{
				if (canJump)
				{
					inputAcceleration.direction = Double3::UnitY;
					inputAcceleration.magnitude = player.getJumpMagnitude();
					inputAcceleration.isInstantJump = true;
				}
			}
			else if (std::isfinite(accelDirection.length()))
			{
				inputAcceleration.direction = accelDirection;
				inputAcceleration.magnitude = accelMagnitude;
			}
		}
		else if (!isClimbing)
		{
			inputAcceleration.shouldResetVelocity = true;
		}

		return inputAcceleration;
	}

	PlayerInputAcceleration getInputAccelerationModern(Player &player, double moveSpeed, bool isOnGround, bool canJump, bool isClimbing,
		double ceilingScale, bool isGhostModeEnabled, const InputManager &inputManager)
	{
		PlayerInputAcceleration inputAcceleration;

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
			if (isOnGround || isClimbing)
			{
				if (forward || backward || left || right || jump)
				{
					// Check for jumping first so the player can't slide jump on the first frame.
					if (jump)
					{
						if (canJump)
						{
							inputAcceleration.direction = Double3::UnitY;
							inputAcceleration.magnitude = player.getJumpMagnitude();
							inputAcceleration.isInstantJump = true;
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
							inputAcceleration.direction = accelDirection;
							inputAcceleration.magnitude = moveSpeed;
						}
					}
				}
				else if (!isClimbing)
				{
					inputAcceleration.shouldResetVelocity = true;
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
				inputAcceleration.direction = accelDirection;
				inputAcceleration.magnitude = PlayerConstants::GHOST_MODE_SPEED;
				inputAcceleration.isGhostMode = true;
				inputAcceleration.shouldResetVelocity = true;
			}
		}

		return inputAcceleration;
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
									MapLogic::handleMapTransition(game, hit, transitionDef);
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
									canDoorBeOpened = false || debugDestroyVoxel; // Can't open unless using debug input
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
			if (entityInst.isCitizen())
			{
				GameWorldUiController::onCitizenInteracted(game, entityInst);
			}
			else if (EntityUtils::tryGetDisplayName(entityDef, charClassLibrary, &entityName))
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
								player.addToKeyInventory(keyID);

								// Destroy entity after popup to avoid using freed transform buffer ID in RenderEntityChunkManager draw calls due to skipping scene simulation.
								const auto callback = [&entityChunkManager, chunkPos, entityInstID]()
								{
									entityChunkManager.queueEntityDestroy(entityInstID, &chunkPos);
								};

								GameWorldUiController::onKeyPickedUp(game, keyID, exeData, callback);
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

PlayerInputAcceleration::PlayerInputAcceleration()
{
	this->magnitude = 0.0;
	this->isInstantJump = false;
	this->isGhostMode = false;
	this->shouldResetVelocity = false;
}

Double2 PlayerLogic::makeTurningAngularValues(Game &game, double dt, const Int2 &mouseDelta, BufferView<const Rect> nativeCursorRegions)
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
		const bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		const double turningScale = !player.groundState.isSwimming ? 1.0 : (2.0 / 3.0);

		// Mouse takes priority over keyboard.
		if (leftClick)
		{
			const Int2 mousePosition = inputManager.getMousePosition();

			// Turning strength is determined by closeness of the mouse cursor to left/right screen edge.
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
						return -1.0 + (static_cast<double>(mouseX) / topLeft.width);
					}
					else if (topRight.contains(mousePosition))
					{
						return static_cast<double>(mouseX - topRight.getLeft()) / topRight.width;
					}
					else if (middleLeft.contains(mousePosition))
					{
						return -1.0 + (static_cast<double>(mouseX) / middleLeft.width);
					}
					else if (middleRight.contains(mousePosition))
					{
						return static_cast<double>(mouseX - middleRight.getLeft()) / middleRight.width;
					}
					else
					{
						return 0.0;
					}
				}();

				// No NaNs or infinities allowed.
				return std::isfinite(percent) ? percent : 0.0;
			}();

			return Double2((dx * turningScale) * dt, 0.0);
		}
		else if (!lCtrl)
		{
			// Turn with keyboard.
			if (left)
			{
				return Double2(-turningScale * dt, 0.0);
			}
			else if (right)
			{
				return Double2(turningScale * dt, 0.0);
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

	return Double2::Zero;
}

PlayerInputAcceleration PlayerLogic::getInputAcceleration(Game &game, BufferView<const Rect> nativeCursorRegions)
{
	const InputManager &inputManager = game.inputManager;
	const JPH::PhysicsSystem &physicsSystem = game.physicsSystem;
	const double ceilingScale = game.gameState.getActiveCeilingScale();

	Player &player = game.player;
	const PlayerGroundState &groundState = player.groundState;
	const bool isOnGround = groundState.onGround;
	const bool canJump = groundState.canJump;
	const bool isClimbing = player.movementType == PlayerMovementType::Climbing;
	const double maxMoveSpeed = player.getMaxMoveSpeed();

	const Options &options = game.options;
	const bool isGhostModeEnabled = options.getMisc_GhostMode();
	const bool modernInterface = options.getGraphics_ModernInterface();

	PlayerInputAcceleration inputAcceleration;
	if (!modernInterface)
	{
		inputAcceleration = PlayerLogic::getInputAccelerationClassic(player, maxMoveSpeed, isOnGround, canJump, isClimbing, ceilingScale, isGhostModeEnabled, inputManager, nativeCursorRegions);
	}
	else
	{
		inputAcceleration = PlayerLogic::getInputAccelerationModern(player, maxMoveSpeed, isOnGround, canJump, isClimbing, ceilingScale, isGhostModeEnabled, inputManager);
	}

	return inputAcceleration;
}

void PlayerLogic::handleAttack(Game &game, const Int2 &mouseDelta)
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

	const Options &options = game.options;
	const InputManager &inputManager = game.inputManager;
	const GameState &gameState = game.gameState;
	const double ceilingScale = gameState.getActiveCeilingScale();
	AudioManager &audioManager = game.audioManager;
	Random &random = game.random;
	SceneManager &sceneManager = game.sceneManager;
	VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;

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

			newStateIndex = PlayerLogic::getMeleeAnimDirectionStateIndex(weaponAnimDef, cardinalDirection);
			nextStateIndex = weaponAnimIdleStateIndex;
			sfxFilename = ArenaSoundName::Swish;

			constexpr double playerMeleeSwingRange = PlayerConstants::MELEE_HIT_RANGE;
			constexpr double playerHitSearchRadius = PlayerConstants::MELEE_HIT_SEARCH_RADIUS;
			constexpr double playerHalfHeight = PlayerConstants::TOP_OF_HEAD_HEIGHT / 2.0;
			const WorldDouble3 hitSearchCenterPoint = player.getFeetPosition() + WorldDouble3(0.0, playerHalfHeight, 0.0) + (player.getGroundDirection() * playerMeleeSwingRange);
			CombatHitSearchResult hitSearchResult;
			CombatLogic::getHitSearchResult(hitSearchCenterPoint, playerHitSearchRadius, ceilingScale, voxelChunkManager, entityChunkManager, &hitSearchResult);

			for (const WorldInt3 hitWorldVoxel : hitSearchResult.getVoxels())
			{
				const CoordInt3 hitVoxelCoord = VoxelUtils::worldVoxelToCoord(hitWorldVoxel);
				const VoxelInt3 hitVoxel = hitVoxelCoord.voxel;
				VoxelChunk &hitVoxelChunk = voxelChunkManager.getChunkAtPosition(hitVoxelCoord.chunk);

				VoxelDoorDefID doorDefID;
				if (!hitVoxelChunk.tryGetDoorDefID(hitVoxel.x, hitVoxel.y, hitVoxel.z, &doorDefID))
				{
					continue;
				}

				// Can't hit if already open.
				int doorAnimInstIndex;
				if (hitVoxelChunk.tryGetDoorAnimInstIndex(hitVoxel.x, hitVoxel.y, hitVoxel.z, &doorAnimInstIndex))
				{
					continue;
				}

				// Can only hit if not previously unlocked.
				bool isDoorBashable = false;
				int triggerInstIndex;
				if (!hitVoxelChunk.tryGetTriggerInstIndex(hitVoxel.x, hitVoxel.y, hitVoxel.z, &triggerInstIndex))
				{
					VoxelLockDefID lockDefID;
					if (hitVoxelChunk.tryGetLockDefID(hitVoxel.x, hitVoxel.y, hitVoxel.z, &lockDefID))
					{
						const LockDefinition &lockDef = hitVoxelChunk.getLockDef(lockDefID);
						isDoorBashable = lockDef.lockLevel >= 0; // @todo don't allow key-only doors to be bashable
					}
				}

				if (isDoorBashable)
				{
					const WorldDouble3 hitWorldVoxelCenter = VoxelUtils::getVoxelCenter(hitWorldVoxel, ceilingScale);
					audioManager.playSound(ArenaSoundName::Bash, hitWorldVoxelCenter);

					DebugLog("Door bashing not implemented.");
					if (random.nextBool())
					{
						/*VoxelTriggerInstance newTriggerInst;
						newTriggerInst.init(hitVoxel.x, hitVoxel.y, hitVoxel.z);
						hitVoxelChunk.addTriggerInst(std::move(newTriggerInst));*/

						// @todo open door, add door anim, play open sound. maybe could be commonized in a MapLogic
					}
				}
			}

			for (const EntityInstanceID hitEntityInstID : hitSearchResult.getEntities())
			{
				const EntityInstance &hitEntityInst = entityChunkManager.getEntity(hitEntityInstID);
				const WorldDouble3 hitEntityPosition = entityChunkManager.getEntityPosition(hitEntityInst.positionID);
				const BoundingBox3D &hitEntityBBox = entityChunkManager.getEntityBoundingBox(hitEntityInst.bboxID);
				const WorldDouble3 hitEntityMiddlePosition(hitEntityPosition.x, hitEntityPosition.y + hitEntityBBox.halfHeight, hitEntityPosition.z);

				const EntityDefinition &hitEntityDef = entityChunkManager.getEntityDef(hitEntityInst.defID);
				const EntityAnimationDefinition &hitEntityAnimDef = hitEntityDef.animDef;
				EntityAnimationInstance &hitEntityAnimInst = entityChunkManager.getEntityAnimationInstance(hitEntityInst.animInstID);
				const int hitEntityAnimInstCurrentStateIndex = hitEntityAnimInst.currentStateIndex;				
				const std::optional<int> hitEntityCorpseAnimStateIndex = hitEntityAnimDef.tryGetStateIndex(EntityAnimationUtils::STATE_DEATH.c_str());
				const bool hitEntityHasDeathAnim = hitEntityCorpseAnimStateIndex.has_value();
				
				// @todo use hitEntityDef.enemy.creature.hasNoCorpse since some enemies disintegrate (queueEntityDestroy) at end of death state
				// - human enemies always leave corpse
				const bool canHitEntityLeaveCorpse = hitEntityHasDeathAnim;

				// @todo as far as "can i hit them" is concerned, we care if 1) is not dying and 2) is not dead (i.e. death state progress == 1.0)
				const bool isHitEntityDead = hitEntityHasDeathAnim && hitEntityAnimInstCurrentStateIndex == *hitEntityCorpseAnimStateIndex;

				const EntityDefinitionType hitEntityDefType = hitEntityDef.type;
				const bool canHitEntityDie = !isHitEntityDead && hitEntityHasDeathAnim || (hitEntityDefType == EntityDefinitionType::Citizen);

				if (canHitEntityDie)
				{
					const bool isHitEntityDying = random.nextBool(); // @todo actual hp dmg calculation

					if (isHitEntityDying)
					{
						if (hitEntityHasDeathAnim)
						{
							hitEntityAnimInst.setStateIndex(*hitEntityCorpseAnimStateIndex);
						}
						else
						{
							entityChunkManager.queueEntityDestroy(hitEntityInstID, true);
						}
						
						audioManager.playSound(ArenaSoundName::EnemyHit, hitEntityMiddlePosition);

						// @todo spawn blood vfx at hit position
					}
					else
					{
						audioManager.playSound(ArenaSoundName::Clank, hitEntityMiddlePosition);
					}
				}				
			}
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
			audioManager.playSound(sfxFilename);
		}
	}
}

void PlayerLogic::handleScreenToWorldInteraction(Game &game, const Int2 &nativePoint, bool isPrimaryInteraction,
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
