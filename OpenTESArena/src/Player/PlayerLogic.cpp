#include "PlayerLogic.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Collision/ArenaSelectionUtils.h"
#include "../Collision/Physics.h"
#include "../Collision/RayCastTypes.h"
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
		double ceilingScale, bool isGhostModeEnabled, const InputManager &inputManager, Span<const Rect> nativeCursorRegions)
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
			const Rect &topLeft = nativeCursorRegions[GameWorldUiView::CursorTopLeftIndex];
			const Rect &top = nativeCursorRegions[GameWorldUiView::CursorTopMiddleIndex];
			const Rect &topRight = nativeCursorRegions[GameWorldUiView::CursorTopRightIndex];
			const Rect &bottomLeft = nativeCursorRegions[GameWorldUiView::CursorBottomLeftIndex];
			const Rect &bottom = nativeCursorRegions[GameWorldUiView::CursorBottomMiddleIndex];
			const Rect &bottomRight = nativeCursorRegions[GameWorldUiView::CursorBottomRightIndex];

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

	void handleRayCastHitVoxel(Game &game, const RayCastHit &hit, bool isPrimaryInteraction, bool debugDestroyVoxel, double ceilingScale,
		VoxelChunkManager &voxelChunkManager, TextBox &actionTextBox)
	{
		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const ExeData &exeData = binaryAssetLibrary.getExeData();

		const RayCastVoxelHit &voxelHit = hit.voxelHit;
		const ChunkInt2 chunkPos = voxelHit.voxelCoord.chunk;
		const VoxelInt3 voxel = voxelHit.voxelCoord.voxel;

		VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(chunkPos);
		const VoxelTraitsDefID voxelTraitsDefID = voxelChunk.traitsDefIDs.get(voxel.x, voxel.y, voxel.z);
		const VoxelTraitsDefinition &voxelTraitsDef = voxelChunk.traitsDefs[voxelTraitsDefID];
		const ArenaVoxelType voxelType = voxelTraitsDef.type;

		GameState &gameState = game.gameState;
		const Player &player = game.player;

		if (isPrimaryInteraction)
		{
			const bool passesVoxelDistanceTest = hit.t <= ArenaSelectionUtils::VOXEL_MAX_DISTANCE;

			if (ArenaSelectionUtils::isVoxelSelectableAsPrimary(voxelType))
			{
				if (!debugDestroyVoxel)
				{
					if (passesVoxelDistanceTest)
					{
						const bool isWall = voxelType == ArenaVoxelType::Wall;

						// The only edge voxels with a transition should be should be palace entrances (with collision).
						const bool isEdge = (voxelType == ArenaVoxelType::Edge) && voxelTraitsDef.edge.collider;

						if (isWall || isEdge)
						{
							VoxelTransitionDefID transitionDefID;
							if (voxelChunk.tryGetTransitionDefID(voxel.x, voxel.y, voxel.z, &transitionDefID))
							{
								const TransitionDefinition &transitionDef = voxelChunk.transitionDefs[transitionDefID];
								if (transitionDef.type != TransitionType::InteriorLevelChange)
								{
									MapLogic::handleMapTransition(game, hit, transitionDef);
								}
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
						voxelChunk.fadeAnimInsts.emplace_back(std::move(fadeAnimInst));
					}
				}
			}
			else if (voxelType == ArenaVoxelType::Door)
			{
				if (passesVoxelDistanceTest)
				{
					// If the door is closed, try to open it.
					int doorAnimInstIndex;
					const bool isDoorClosed = !voxelChunk.tryGetDoorAnimInstIndex(voxel.x, voxel.y, voxel.z, &doorAnimInstIndex);
					if (isDoorClosed)
					{
						bool canDoorBeOpened = true;
						bool isApplyingDoorKeyToLock = false;
						int requiredDoorKeyID = -1;
						int lockLevel = 0;

						int triggerInstIndex;
						const bool hasDoorBeenUnlocked = voxelChunk.tryGetTriggerInstIndex(voxel.x, voxel.y, voxel.z, &triggerInstIndex);
						if (!hasDoorBeenUnlocked)
						{
							VoxelLockDefID lockDefID;
							if (voxelChunk.tryGetLockDefID(voxel.x, voxel.y, voxel.z, &lockDefID))
							{
								const LockDefinition &lockDef = voxelChunk.lockDefs[lockDefID];
								requiredDoorKeyID = lockDef.keyID;
								lockLevel = lockDef.lockLevel;

								if (requiredDoorKeyID >= 0)
								{
									if (player.isIdInKeyInventory(requiredDoorKeyID))
									{
										isApplyingDoorKeyToLock = true;
									}
									else
									{
										canDoorBeOpened = false || debugDestroyVoxel; // Can't open unless using debug input
									}
								}
							}
						}

						if (canDoorBeOpened)
						{
							constexpr bool isWeaponBashing = false;
							MapLogic::handleDoorOpen(game, voxelChunk, voxel, ceilingScale, isApplyingDoorKeyToLock, requiredDoorKeyID, isWeaponBashing);
						}
						else
						{
							const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
							const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(player.charClassDefID);

							const int lockDifficultyIndex = ArenaPlayerUtils::getLockDifficultyMessageIndex(lockLevel, charClassDef.thievingDivisor, player.level, player.primaryAttributes, exeData);
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
					const std::string &buildingName = voxelChunk.buildingNames[buildingNameID];
					actionTextBox.setText(buildingName);
					gameState.setActionTextDuration(buildingName);
				}
			}
		}
	}

	void handleRayCastHitEntity(Game &game, const RayCastHit &hit, bool isPrimaryInteraction, double ceilingScale, const VoxelChunkManager &voxelChunkManager,
		EntityChunkManager &entityChunkManager, TextBox &actionTextBox)
	{
		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const ExeData &exeData = binaryAssetLibrary.getExeData();

		const RayCastEntityHit &entityHit = hit.entityHit;

		GameState &gameState = game.gameState;
		Player &player = game.player;

		if (isPrimaryInteraction)
		{
			const EntityInstanceID entityInstID = entityHit.id;
			const EntityInstance &entityInst = entityChunkManager.getEntity(entityInstID);
			const WorldDouble3 entityPosition = entityChunkManager.getEntityPosition(entityInstID);
			const CoordDouble3 entityCoord = VoxelUtils::worldPointToCoord(entityPosition);
			const ChunkInt2 entityChunkPos = entityCoord.chunk;
			const VoxelInt3 entityVoxel = VoxelUtils::pointToVoxel(entityCoord.point, ceilingScale);

			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const EntityDefinitionType entityType = entityDef.type;

			switch (entityType)
			{
			case EntityDefinitionType::Enemy:
			{
				const EnemyEntityDefinition &enemyDef = entityDef.enemy;
				EntityCombatState &combatState = entityChunkManager.getEntityCombatState(entityInst.combatStateID);
				if (!combatState.isDying)
				{
					GameWorldUiController::onEnemyAliveInspected(game, entityInstID, entityDef, actionTextBox);
				}

				if (combatState.isDead)
				{
					if (hit.t <= ArenaSelectionUtils::LOOT_MAX_DISTANCE)
					{
						if (!combatState.hasBeenLootedBefore)
						{
							combatState.hasBeenLootedBefore = true;
							GameWorldUiController::onEnemyCorpseInteractedFirstTime(game, entityInstID, entityDef);
						}
						else
						{
							GameWorldUiController::onEnemyCorpseInteracted(game, entityInstID, entityDef);
						}
					}
				}

				break;
			}
			case EntityDefinitionType::Citizen:
				if (hit.t <= ArenaSelectionUtils::CITIZEN_MAX_DISTANCE)
				{
					GameWorldUiController::onCitizenInteracted(game, entityInst);
				}
				break;
			case EntityDefinitionType::StaticNPC:
			{
				const StaticNpcEntityDefinition &staticNpcDef = entityDef.staticNpc;
				GameWorldUiController::onStaticNpcInteracted(game, staticNpcDef.personalityType);
				break;
			}
			case EntityDefinitionType::Item:
			{
				if (hit.t <= ArenaSelectionUtils::LOOT_MAX_DISTANCE)
				{
					const ItemEntityDefinition &itemDef = entityDef.item;
					const ItemEntityDefinitionType itemDefType = itemDef.type;

					if (itemDefType == ItemEntityDefinitionType::Key)
					{
						const VoxelChunk &voxelChunk = voxelChunkManager.getChunkAtPosition(entityChunkPos);

						VoxelTriggerDefID triggerDefID;
						if (voxelChunk.tryGetTriggerDefID(entityVoxel.x, entityVoxel.y, entityVoxel.z, &triggerDefID))
						{
							const VoxelTriggerDefinition &triggerDef = voxelChunk.triggerDefs[triggerDefID];
							if (triggerDef.hasKeyDef())
							{
								const VoxelTriggerKeyDefinition &triggerKeyDef = triggerDef.key;
								const int keyID = triggerKeyDef.keyID;
								player.addToKeyInventory(keyID);

								// Destroy entity after popup to avoid using freed transform buffer ID in RenderEntityManager draw calls due to skipping scene simulation.
								const auto callback = [&entityChunkManager, entityChunkPos, entityInstID]()
								{
									entityChunkManager.queueEntityDestroy(entityInstID, &entityChunkPos);
								};

								GameWorldUiController::onKeyPickedUp(game, keyID, exeData, callback);
							}
						}
					}
					else if (itemDefType == ItemEntityDefinitionType::QuestItem)
					{
						AudioManager &audioManager = game.audioManager;
						audioManager.playSound(ArenaSoundName::Fanfare2);
						DebugLogFormat("Picked up quest item (entity %d).", entityInstID);
						entityChunkManager.queueEntityDestroy(entityInstID, &entityChunkPos);
					}
				}

				break;
			}
			case EntityDefinitionType::Container:
			{
				if (hit.t <= ArenaSelectionUtils::LOOT_MAX_DISTANCE)
				{
					const ContainerEntityDefinition &containerDef = entityDef.container;
					const ContainerEntityDefinitionType containerDefType = containerDef.type;

					bool isContainerInventoryAccessible = true;
					if (entityInst.canBeLocked())
					{
						const EntityLockState &lockState = entityChunkManager.getEntityLockState(entityInst.lockStateID);
						isContainerInventoryAccessible = !lockState.isLocked;
					}

					if (isContainerInventoryAccessible)
					{
						ItemInventory &containerItemInventory = entityChunkManager.getEntityItemInventory(entityInst.itemInventoryInstID);
						constexpr bool destroyEntityIfEmpty = true; // Always for piles/chests.
						GameWorldUiController::onContainerInventoryOpened(game, entityInstID, containerItemInventory, destroyEntityIfEmpty);
					}
				}

				break;
			}
			default:
				break;
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

PlayerInputAcceleration::PlayerInputAcceleration()
{
	this->magnitude = 0.0;
	this->isInstantJump = false;
	this->isGhostMode = false;
	this->shouldResetVelocity = false;
}

Double2 PlayerLogic::makeTurningAngularValues(Game &game, double dt, const Int2 &mouseDelta, Span<const Rect> nativeCursorRegions)
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
					const Rect &topLeft = nativeCursorRegions[GameWorldUiView::CursorTopLeftIndex];
					const Rect &topRight = nativeCursorRegions[GameWorldUiView::CursorTopRightIndex];
					const Rect &middleLeft = nativeCursorRegions[GameWorldUiView::CursorMiddleLeftIndex];
					const Rect &middleRight = nativeCursorRegions[GameWorldUiView::CursorMiddleRightIndex];

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

			return Double2((-dx * turningScale) * dt, 0.0);
		}
		else if (!lCtrl)
		{
			// Turn with keyboard.
			if (left)
			{
				return Double2(turningScale * dt, 0.0);
			}
			else if (right)
			{
				return Double2(-turningScale * dt, 0.0);
			}
		}
	}
	else
	{
		const int dx = mouseDelta.x;
		const int dy = mouseDelta.y;
		const bool isTurning = (dx != 0) || (dy != 0);

		if (isTurning)
		{
			const Int2 dimensions = game.window.getPixelDimensions();

			// Get the smaller of the two dimensions, so the look sensitivity is relative
			// to a square instead of a rectangle. This keeps the camera look independent
			// of the aspect ratio.
			const int minDimension = std::min(dimensions.x, dimensions.y);
			const double dxPercent = static_cast<double>(dx) / static_cast<double>(minDimension);
			const double dyPercent = static_cast<double>(dy) / static_cast<double>(minDimension);

			return Double2(-dxPercent, -dyPercent);
		}
	}

	return Double2::Zero;
}

PlayerInputAcceleration PlayerLogic::getInputAcceleration(Game &game, Span<const Rect> nativeCursorRegions)
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

CardinalDirectionName PlayerLogic::getRandomMeleeSwingDirection(Random &random)
{
	constexpr int directionCount = static_cast<int>(std::size(CardinalDirection::DisplayNames));
	const int randomValue = random.next(directionCount);
	return static_cast<CardinalDirectionName>(randomValue);
}

bool PlayerLogic::tryGetMeleeSwingDirectionFromMouseDelta(const Int2 &mouseDelta, const Int2 &windowDims, CardinalDirectionName *outDirectionName)
{
	// Get smaller screen dimension so mouse delta is relative to a square.
	const int minDimension = std::min(windowDims.x, windowDims.y);
	constexpr double requiredDistancePercent = 0.060; // Arbitrary

	const double mouseDeltaXPercent = static_cast<double>(mouseDelta.x) / static_cast<double>(minDimension);
	const double mouseDeltaYPercent = static_cast<double>(mouseDelta.y) / static_cast<double>(minDimension);
	const double mouseDistancePercent = std::sqrt((mouseDeltaXPercent * mouseDeltaXPercent) + (mouseDeltaYPercent * mouseDeltaYPercent));
	const bool isMouseDeltaFastEnough = mouseDistancePercent >= requiredDistancePercent;
	if (!isMouseDeltaFastEnough)
	{
		return false;
	}

	const Double2 mouseDirection = Double2(mouseDeltaXPercent, -mouseDeltaYPercent).normalized();
	*outDirectionName = CardinalDirection::getDirectionName(Double2(-mouseDirection.y, -mouseDirection.x));
	return true;
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
	const bool isModernInterface = options.getGraphics_ModernInterface();
	const InputManager &inputManager = game.inputManager;
	const GameState &gameState = game.gameState;
	const double ceilingScale = gameState.getActiveCeilingScale();
	AudioManager &audioManager = game.audioManager;
	const Window &window = game.window;
	Renderer &renderer = game.renderer;
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
		const Int2 windowDims = window.getPixelDimensions();

		CardinalDirectionName meleeSwingDirection = static_cast<CardinalDirectionName>(-1);
		bool hasSelectedMeleeSwingDirection = false;
		if (isModernInterface)
		{
			if (player.queuedMeleeSwingDirection >= 0)
			{
				meleeSwingDirection = static_cast<CardinalDirectionName>(player.queuedMeleeSwingDirection);
				hasSelectedMeleeSwingDirection = true;
			}
		}
		else
		{
			hasSelectedMeleeSwingDirection = PlayerLogic::tryGetMeleeSwingDirectionFromMouseDelta(mouseDelta, windowDims, &meleeSwingDirection);
		}

		if (isAttackMouseButtonDown && hasSelectedMeleeSwingDirection)
		{
			newStateIndex = PlayerLogic::getMeleeAnimDirectionStateIndex(weaponAnimDef, meleeSwingDirection);
			nextStateIndex = weaponAnimIdleStateIndex;
			sfxFilename = ArenaSoundName::Swish;

			constexpr double playerMeleeSwingRange = PlayerConstants::MELEE_HIT_RANGE;
			constexpr double playerHitSearchRadius = PlayerConstants::MELEE_HIT_SEARCH_RADIUS;
			constexpr double playerHalfHeight = PlayerConstants::TOP_OF_HEAD_HEIGHT / 2.0;
			const WorldDouble3 playerFeetPosition = player.getFeetPosition();
			const WorldDouble3 hitSearchCenterPoint = playerFeetPosition + WorldDouble3(0.0, playerHalfHeight, 0.0) + (player.getGroundDirection() * playerMeleeSwingRange);
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
						const LockDefinition &lockDef = hitVoxelChunk.lockDefs[lockDefID];
						isDoorBashable = lockDef.lockLevel >= 0; // @todo don't allow key-only doors to be bashable

						if (isDoorBashable)
						{
							const WorldDouble3 hitWorldVoxelCenter = VoxelUtils::getVoxelCenter(hitWorldVoxel, ceilingScale);
							audioManager.playSound(ArenaSoundName::Bash, hitWorldVoxelCenter);

							if (ArenaItemUtils::isFistsWeapon(player.weaponAnimDefID))
								player.currentHealth -= (ArenaPlayerUtils::getSelfDamageFromBashWithFists(random));

							int damage = 6;	// @todo: Calculate damage

							if (ArenaPlayerUtils::doesBashSucceed(damage, lockDef.lockLevel, player.primaryAttributes, random))
							{
								constexpr bool isApplyingDoorKeyToLock = false;
								constexpr int doorKeyID = -1;
								constexpr bool isWeaponBashing = true;
								MapLogic::handleDoorOpen(game, hitVoxelChunk, hitVoxel, ceilingScale, isApplyingDoorKeyToLock, doorKeyID, isWeaponBashing);
							}
						}
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

				const EntityCombatState *hitEntityCombatState = nullptr;
				bool canHitEntityBeKilled = false;
				if (hitEntityInst.canBeKilledInCombat())
				{
					hitEntityCombatState = &entityChunkManager.getEntityCombatState(hitEntityInst.combatStateID);
					canHitEntityBeKilled = !hitEntityCombatState->isInDeathState();
				}

				EntityLockState *hitEntityLockState = nullptr;
				bool canHitEntityLockBeBroken = false;
				if (hitEntityInst.canBeLocked())
				{
					hitEntityLockState = &entityChunkManager.getEntityLockState(hitEntityInst.lockStateID);
					canHitEntityLockBeBroken = hitEntityLockState->isLocked;
				}

				if (canHitEntityBeKilled)
				{
					// Simulate weapon swing against them.
					const bool canHitEntityResistDamage = hitEntityDef.type == EntityDefinitionType::Enemy; // @todo give citizens only 1 hp
					const bool isHitEntityHpAtZero = !canHitEntityResistDamage || random.nextBool(); // @todo actual hp dmg calculation

					if (isHitEntityHpAtZero)
					{
						const std::optional<int> hitEntityDeathAnimStateIndex = EntityUtils::tryGetDeathAnimStateIndex(hitEntityAnimDef);
						const bool hitEntityHasDeathAnim = hitEntityDeathAnimStateIndex.has_value();

						if (hitEntityHasDeathAnim)
						{
							hitEntityAnimInst.setStateIndex(*hitEntityDeathAnimStateIndex);
						}
						else
						{
							entityChunkManager.queueEntityDestroy(hitEntityInstID, true);
						}

						if (hitEntityInst.isCitizen())
						{
							GameWorldUiController::onCitizenKilled(game);
						}

						// Arbitrary height where the swing is hitting.
						const double hitVfxHeightBias = std::min(PlayerConstants::TOP_OF_HEAD_HEIGHT * 0.60, hitEntityBBox.halfHeight);

						// Avoid z-fighting with entity.
						const Double2 hitVfxPositionBias = -player.getGroundDirectionXZ() * Constants::Epsilon;

						const WorldDouble3 hitVfxPosition(
							hitEntityPosition.x + hitVfxPositionBias.x,
							hitEntityPosition.y + hitVfxHeightBias,
							hitEntityPosition.z + hitVfxPositionBias.y);

						CombatLogic::spawnHitVfx(hitEntityDef, hitVfxPosition, entityChunkManager, random, game.physicsSystem, renderer);

						audioManager.playSound(ArenaSoundName::EnemyHit, hitEntityMiddlePosition);
					}
					else
					{
						audioManager.playSound(ArenaSoundName::Clank, hitEntityMiddlePosition);
					}
				}
				else if (canHitEntityLockBeBroken)
				{
					const bool isLockBashSuccessful = random.nextBool(); // @todo actual lock bash calculation

					if (isLockBashSuccessful)
					{
						hitEntityLockState->isLocked = false;

						const std::optional<int> unlockedAnimDefStateIndex = hitEntityAnimDef.findStateIndex(EntityAnimationUtils::STATE_UNLOCKED.c_str());
						DebugAssert(unlockedAnimDefStateIndex.has_value());
						hitEntityAnimInst.setStateIndex(*unlockedAnimDefStateIndex);
					}

					audioManager.playSound(ArenaSoundName::Bash, hitEntityMiddlePosition);
				}
			}
		}
	}
	else
	{
		bool isAttack = false;
		if (isModernInterface)
		{
			isAttack = isAttackMouseButtonDown;
		}
		else
		{
			// Cursor must be above game world interface. In the original game, it has to be an "X", but relaxing that here.
			TextureManager &textureManager = game.textureManager;
			const TextureAsset gameWorldInterfaceTextureAsset = GameWorldUiView::getGameWorldInterfaceTextureAsset();
			const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(gameWorldInterfaceTextureAsset.filename.c_str());
			if (!metadataID.has_value())
			{
				DebugCrash("Couldn't get game world interface metadata ID for \"" + gameWorldInterfaceTextureAsset.filename + "\".");
			}

			const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);
			const int gameWorldInterfaceHeight = metadata.getHeight(0);
			const int originalCursorY = window.nativeToOriginal(inputManager.getMousePosition()).y;
			const bool isCursorInSceneView = originalCursorY < (ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceHeight);
			isAttack = isAttackMouseButtonDown && isCursorInSceneView;
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
	EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;
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
		if (hit.type == RayCastHitType::Voxel)
		{
			handleRayCastHitVoxel(game, hit, isPrimaryInteraction, debugFadeVoxel, ceilingScale, voxelChunkManager, actionTextBox);
		}
		else if (hit.type == RayCastHitType::Entity)
		{
			handleRayCastHitEntity(game, hit, isPrimaryInteraction, ceilingScale, voxelChunkManager, entityChunkManager, actionTextBox);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(hit.type)));
		}
	}
}
