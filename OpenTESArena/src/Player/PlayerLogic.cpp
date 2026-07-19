#include "PlayerLogic.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaSoundName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextAssetLibrary.h"
#include "../Collision/ArenaSelectionUtils.h"
#include "../Collision/Physics.h"
#include "../Collision/RayCastTypes.h"
#include "../Combat/CombatLogic.h"
#include "../Entities/ArenaEntityUtils.h"
#include "../Entities/EntityDefinitionLibrary.h"
#include "../Game/Game.h"
#include "../Interface/GameWorldUiMVC.h"
#include "../Interface/GameWorldUiState.h"
#include "../Items/ArenaItemUtils.h"
#include "../Items/ItemLibrary.h"
#include "../Player/WeaponAnimationLibrary.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Time/ArenaClockUtils.h"
#include "../Voxels/ArenaVoxelUtils.h"
#include "../World/CardinalDirection.h"
#include "../World/CardinalDirectionName.h"
#include "../World/MapLogic.h"
#include "../World/MapType.h"

#include "components/utilities/String.h"

namespace PlayerLogic
{
	PlayerInputAcceleration getInputAccelerationClassic(const Player &player, double moveSpeed, bool isOnGround, bool canJump, bool isClimbing,
		bool canPlayerMoveAndTurn, double ceilingScale, bool isGhostModeEnabled, const InputManager &inputManager, Span<const Rect> nativeCursorRegions)
	{
		PlayerInputAcceleration inputAcceleration;
		if ((!isOnGround && !isClimbing) || !canPlayerMoveAndTurn)
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
		bool canPlayerMoveAndTurn, double ceilingScale, bool isGhostModeEnabled, const InputManager &inputManager)
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
			if (!canPlayerMoveAndTurn)
			{
				return inputAcceleration;
			}

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

	void handleRayCastHitVoxel(Game &game, const RayCastHit &hit, bool isPrimaryInteraction, GameWorldInteractionType interactionType,
		bool debugDestroyVoxel, double ceilingScale, VoxelChunkManager &voxelChunkManager)
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
		const MapType mapType = gameState.getActiveMapType();
		const Clock &clock = gameState.getClock();
		const bool isNight = ArenaClockUtils::nightMusicIsActive(clock);
		const Player &player = game.player;
		Random &random = game.random;
		ArenaRandom &arenaRandom = game.arenaRandom;

		const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
		const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(player.charClassDefID);

		if (isPrimaryInteraction)
		{
			if (!game.canPlayerMoveAndTurn())
			{
				return;
			}

			const bool passesVoxelDistanceTest = hit.t <= ArenaSelectionUtils::VOXEL_MAX_DISTANCE;

			if (ArenaSelectionUtils::isVoxelSelectableAsPrimary(voxelType))
			{
				if (!debugDestroyVoxel)
				{
					if (!passesVoxelDistanceTest)
					{
						return;
					}

					const bool isWall = voxelType == ArenaVoxelType::Wall;
					const bool isEdge = (voxelType == ArenaVoxelType::Edge) && voxelTraitsDef.edge.collider; // Just palaces.

					if (isWall || isEdge)
					{
						VoxelTransitionDefID transitionDefID;
						if (!voxelChunk.tryGetTransitionDefID(voxel.x, voxel.y, voxel.z, &transitionDefID))
						{
							return;
						}

						const TransitionDefinition &transitionDef = voxelChunk.transitionDefs[transitionDefID];
						const bool isTransitionVoxelSelectable = transitionDef.type != TransitionType::InteriorLevelChange;
						if (!isTransitionVoxelSelectable)
						{
							return;
						}

						const InteriorEntranceTransitionDefinition &interiorEntranceTransitionDef = transitionDef.interiorEntrance;
						const ArenaInteriorType interiorType = interiorEntranceTransitionDef.interiorGenInfo.interiorType;
						const bool isPalace = interiorType == ArenaInteriorType::Palace;
						if (isPalace && isNight)
						{
							GameWorldUI::showTextPopUp(exeData.services.palaceClosedAtNight.c_str(), GameWorldUiView::StatusPopUpFontName, GameWorldUiView::StatusPopUpTextAlignment);
							return;
						}

						const bool canEntranceBeLocked =
							(interiorType == ArenaInteriorType::Equipment) ||
							(interiorType == ArenaInteriorType::House) ||
							(interiorType == ArenaInteriorType::MagesGuild) ||
							(interiorType == ArenaInteriorType::Noble) ||
							(interiorType == ArenaInteriorType::Temple);
						const WorldInt3 transitionWorldVoxel = VoxelUtils::coordToWorldVoxel(voxelHit.voxelCoord);

						int lockLevel = 0;
						if (isNight && canEntranceBeLocked)
						{
							const OriginalInt2 transitionOriginalVoxel = VoxelUtils::worldVoxelToOriginalVoxelMapTypeAware(transitionWorldVoxel.getXZ(), mapType);
							lockLevel = ArenaLevelUtils::getDoorVoxelLockLevel(transitionOriginalVoxel.x, transitionOriginalVoxel.y, arenaRandom);
						}

						const bool isLocked = lockLevel > 0;

						bool isQueuingMapTransition = false;
						if (interactionType == GameWorldInteractionType::Default)
						{
							if (isLocked)
							{
								const int lockDifficultyIndex = ArenaPlayerUtils::getLockDifficultyMessageIndex(lockLevel, charClassDef.thievingDivisor, player.level, player.primaryAttributes, exeData, arenaRandom);
								const std::string lockDifficultyMsg = GameWorldUiModel::getLockDifficultyMessage(lockDifficultyIndex, exeData);
								GameWorldUI::setActionText(lockDifficultyMsg.c_str());
							}
							else
							{
								isQueuingMapTransition = true;
							}
						}
						else if (interactionType == GameWorldInteractionType::Thieving)
						{
							GameWorldUI::setInteractionType(GameWorldInteractionType::Default);

							if (isLocked)
							{
								const bool isLockpickingSuccessful = ArenaPlayerUtils::attemptThieving(lockLevel, charClassDef.thievingDivisor, player.level, player.primaryAttributes, arenaRandom, nullptr);
								if (!isLockpickingSuccessful)
								{
									GameWorldUI::setActionText(exeData.thieving.thievingFailure.c_str());
									if (ArenaEntityUtils::doGuardsAppearForTheft(exeData.thieving.thievingEntranceNoGuardsChance, arenaRandom))
									{
										gameState.queueCityGuardEncounter(game);
									}
									return;
								}

								AudioManager &audioManager = game.audioManager;
								audioManager.playSoundOneShot(ArenaSoundName::Lock); // Sounds better if centered on player during scene transition.
							}

							isQueuingMapTransition = true;
						}

						if (isQueuingMapTransition)
						{
							MapLogic::handleMapTransition(game, hit, transitionDef);
						}
					}
				}
				else
				{
					if (interactionType == GameWorldInteractionType::Default)
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
			}
			else if (voxelType == ArenaVoxelType::Door)
			{
				if (!passesVoxelDistanceTest)
				{
					return;
				}

				int doorAnimInstIndex;
				const bool isDoorClosed = !voxelChunk.tryGetDoorAnimInstIndex(voxel.x, voxel.y, voxel.z, &doorAnimInstIndex);
				if (!isDoorClosed)
				{
					return;
				}

				bool canDoorBeOpened = true;
				bool isApplyingDoorKeyToLock = false;
				int requiredDoorKeyID = -1;
				int lockLevel = 0;
				bool isAttemptingLockpick = false;
				bool isLockpickingSuccessful = false;

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

						if (!isApplyingDoorKeyToLock && interactionType == GameWorldInteractionType::Thieving)
						{
							isAttemptingLockpick = true;
							isLockpickingSuccessful = ArenaPlayerUtils::attemptThieving(lockLevel, charClassDef.thievingDivisor, player.level, player.primaryAttributes, arenaRandom, nullptr);
							canDoorBeOpened = isLockpickingSuccessful;
						}
					}
				}

				if (interactionType != GameWorldInteractionType::Default)
				{
					GameWorldUI::setInteractionType(GameWorldInteractionType::Default);
				}

				if (canDoorBeOpened)
				{
					constexpr bool isWeaponBashing = false;
					MapLogic::handleDoorOpen(game, voxelChunk, voxel, ceilingScale, isApplyingDoorKeyToLock, requiredDoorKeyID, isLockpickingSuccessful, isWeaponBashing);
				}
				else if (isAttemptingLockpick)
				{
					DebugAssert(!isLockpickingSuccessful);
					GameWorldUI::setActionText(exeData.thieving.thievingFailure.c_str());
				}
				else
				{
					const int lockDifficultyIndex = ArenaPlayerUtils::getLockDifficultyMessageIndex(lockLevel, charClassDef.thievingDivisor, player.level, player.primaryAttributes, exeData, arenaRandom);
					const std::string lockDifficultyMsg = GameWorldUiModel::getLockDifficultyMessage(lockDifficultyIndex, exeData);
					GameWorldUI::setActionText(lockDifficultyMsg.c_str());
				}
			}
		}
		else
		{
			GameWorldUI::setInteractionType(GameWorldInteractionType::Default);

			// Handle right click.
			if (ArenaSelectionUtils::isVoxelSelectableAsSecondary(voxelType))
			{
				if (interactionType == GameWorldInteractionType::Default)
				{
					VoxelBuildingNameID buildingNameID;
					if (voxelChunk.tryGetBuildingNameID(voxel.x, voxel.y, voxel.z, &buildingNameID))
					{
						const std::string &buildingName = voxelChunk.buildingNames[buildingNameID];
						GameWorldUI::setActionText(buildingName.c_str());
					}
				}
			}
		}
	}

	void handleRayCastHitEntity(Game &game, const RayCastHit &hit, bool isPrimaryInteraction, GameWorldInteractionType interactionType,
		double ceilingScale, const VoxelChunkManager &voxelChunkManager, EntityChunkManager &entityChunkManager)
	{
		const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
		const ExeData &exeData = binaryAssetLibrary.getExeData();

		const RayCastEntityHit &entityHit = hit.entityHit;
		GameState &gameState = game.gameState;
		Player &player = game.player;
		const bool canPlayerMoveAndTurn = game.canPlayerMoveAndTurn();

		Random &random = game.random;
		ArenaRandom &arenaRandom = game.arenaRandom;

		if (isPrimaryInteraction)
		{
			const EntityInstanceID entityInstID = entityHit.id;
			const EntityInstance &entityInst = entityChunkManager.entities.get(entityInstID);
			const WorldDouble3 entityPosition = entityChunkManager.positions.get(entityInstID);
			const CoordDouble3 entityCoord = VoxelUtils::worldPointToCoord(entityPosition);
			const ChunkInt2 entityChunkPos = entityCoord.chunk;
			const VoxelInt3 entityVoxel = VoxelUtils::pointToVoxel(entityCoord.point, ceilingScale);

			const bool passesLootDistanceTest = hit.t <= ArenaSelectionUtils::LOOT_MAX_DISTANCE;
			const bool passesNpcDistanceTest = hit.t <= ArenaSelectionUtils::CITIZEN_MAX_DISTANCE;

			const EntityDefinition &entityDef = entityChunkManager.getEntityDef(entityInst.defID);
			const EntityDefinitionType entityType = entityDef.type;

			switch (entityType)
			{
			case EntityDefinitionType::Enemy:
			{
				const EnemyEntityDefinition &enemyDef = entityDef.enemy;
				EntityCombatState &combatState = entityChunkManager.combatStates.get(entityInst.combatStateID);
				if (!combatState.isDying)
				{
					if (interactionType != GameWorldInteractionType::Default)
					{
						GameWorldUI::setInteractionType(GameWorldInteractionType::Default);
					}
					
					GameWorldUiController::onEnemyAliveInspected(game, entityInstID, entityDef);
				}

				if (passesLootDistanceTest && canPlayerMoveAndTurn && combatState.isDead)
				{
					if (interactionType == GameWorldInteractionType::Default)
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
					else
					{
						GameWorldUI::setInteractionType(GameWorldInteractionType::Default);
					}
				}

				break;
			}
			case EntityDefinitionType::Citizen:
			{
				if (passesNpcDistanceTest && canPlayerMoveAndTurn)
				{
					if (interactionType == GameWorldInteractionType::Default)
					{
						GameWorldUiController::onCitizenInteracted(game, entityInstID);
					}
					else if (interactionType == GameWorldInteractionType::Thieving)
					{
						GameWorldUI::setInteractionType(GameWorldInteractionType::Default);

						const ArenaTemplateDat &templateDat = TextAssetLibrary::getInstance().templateDat;

						const LocationDefinition &locationDef = gameState.getLocationDefinition();
						const LocationCityDefinition &cityDef = locationDef.getCityDefinition();
						const ArenaCityType cityType = cityDef.type;
						const CharacterClassLibrary &charClassLibrary = CharacterClassLibrary::getInstance();
						const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(player.charClassDefID);
						int pickpocketGoldAmount;
						int pickpocketResultTemplateDatIndex;
						bool guardsAppear;

						bool pickPocketSuccess = ArenaPlayerUtils::attemptPickpocket(cityType, charClassDef.thievingDivisor, player.level, player.primaryAttributes, arenaRandom, exeData, &pickpocketResultTemplateDatIndex, &pickpocketGoldAmount, &guardsAppear);
						const ArenaTemplateDatEntry *pickpocketResultTemplateDatEntry = nullptr;
						if (pickPocketSuccess)
						{
							pickpocketResultTemplateDatEntry = &templateDat.getEntry(pickpocketResultTemplateDatIndex);
							player.gold += pickpocketGoldAmount;
						}
						else
						{
							if (guardsAppear)
							{
								gameState.queueCityGuardEncounter(game);
							}
							GameWorldUI::setActionText(exeData.thieving.thievingFailure.c_str());
						}

						if (pickpocketResultTemplateDatEntry != nullptr)
						{
							const int entryVariantIndex = random.next(static_cast<int>(pickpocketResultTemplateDatEntry->values.size()));
							DebugAssertIndex(pickpocketResultTemplateDatEntry->values, entryVariantIndex);
							std::string pickpocketResultString = pickpocketResultTemplateDatEntry->values[entryVariantIndex];
							pickpocketResultString = String::distributeNewlines(pickpocketResultString, 60);
							GameWorldUI::showTextPopUp(pickpocketResultString.c_str(), GameWorldUiView::StatusPopUpFontName, TextAlignment::TopLeft);
						}
					}
				}

				break;
			}
			case EntityDefinitionType::StaticNPC:
			{
				if (passesNpcDistanceTest && canPlayerMoveAndTurn)
				{
					if (interactionType == GameWorldInteractionType::Default)
					{
						const StaticNpcEntityDefinition &staticNpcDef = entityDef.staticNpc;
						GameWorldUiController::onStaticNpcInteracted(game, entityInstID, staticNpcDef.personalityType);
					}
					else
					{
						GameWorldUI::setInteractionType(GameWorldInteractionType::Default);
					}
				}
				
				break;
			}
			case EntityDefinitionType::Item:
			{
				if (passesLootDistanceTest && canPlayerMoveAndTurn)
				{
					if (interactionType == GameWorldInteractionType::Default)
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
							audioManager.playSoundOneShot(ArenaSoundName::Fanfare2);
							DebugLogFormat("Picked up quest item (entity %d).", entityInstID);
							entityChunkManager.queueEntityDestroy(entityInstID, &entityChunkPos);
						}
					}
					else
					{
						GameWorldUI::setInteractionType(GameWorldInteractionType::Default);
					}
				}

				break;
			}
			case EntityDefinitionType::Container:
			{
				if (passesLootDistanceTest && canPlayerMoveAndTurn)
				{
					const ContainerEntityDefinition &containerDef = entityDef.container;
					const ContainerEntityDefinitionType containerDefType = containerDef.type;

					if (interactionType == GameWorldInteractionType::Default)
					{
						bool isContainerInventoryAccessible = true;
						if (entityInst.canBeLocked())
						{
							const EntityLockState &lockState = entityChunkManager.lockStates.get(entityInst.lockStateID);
							isContainerInventoryAccessible = !lockState.isLocked;
						}

						if (isContainerInventoryAccessible)
						{
							ItemInventory &containerItemInventory = entityChunkManager.itemInventories.get(entityInst.itemInventoryInstID);
							constexpr bool destroyEntityIfEmpty = true; // Always for piles/chests.
							GameWorldUiController::onContainerInventoryOpened(game, entityInstID, containerItemInventory, destroyEntityIfEmpty);
						}
					}
					else if (interactionType == GameWorldInteractionType::Thieving)
					{
						GameWorldUI::setInteractionType(GameWorldInteractionType::Default);

						if (entityInst.canBeLocked())
						{
							EntityLockState &lockState = entityChunkManager.lockStates.get(entityInst.lockStateID);
							const bool canAttemptLockpicking = lockState.isLocked;
							if (canAttemptLockpicking)
							{
								const bool isLockpickingSuccessful = random.nextBool(); // @todo use original chances
								if (isLockpickingSuccessful)
								{
									lockState.isLocked = false;

									const EntityAnimationDefinition &entityAnimDef = entityDef.animDef;
									const std::optional<int> unlockedAnimStateIndex = entityAnimDef.findStateIndex(EntityAnimationUtils::STATE_UNLOCKED.c_str());
									DebugAssert(unlockedAnimStateIndex.has_value());

									EntityAnimationInstance &entityAnimInst = entityChunkManager.animInsts.get(entityInst.animInstID);
									entityAnimInst.setStateIndex(*unlockedAnimStateIndex);

									AudioManager &audioManager = game.audioManager;
									audioManager.playSoundOneShot(ArenaSoundName::Lock, entityPosition);

									GameWorldUI::setActionText(exeData.thieving.thievingSuccessChest.c_str());
								}
								else
								{
									GameWorldUI::setActionText(exeData.thieving.thievingFailureChest.c_str());
								}
							}
						}
					}
				}

				break;
			}
			case EntityDefinitionType::Transition:
			{
				// Wilderness dens are sprites, but entering one follows the same map transition path as a wall entrance. 
				// Synthesize its approached voxel face for the return point.
				if (passesNpcDistanceTest && canPlayerMoveAndTurn)
				{
					if (interactionType == GameWorldInteractionType::Default)
					{
						const TransitionEntityDefinition &transitionEntityDef = entityDef.transition;
						const MapDefinition &mapDef = gameState.getActiveMapDef();
						const LevelInfoDefinition &levelInfoDef = mapDef.getLevelInfoForLevel(gameState.getActiveLevelIndex());
						const TransitionDefinition &transitionDef = levelInfoDef.getTransitionDef(transitionEntityDef.transitionDefID);

						const WorldDouble3 playerPosition = player.getFeetPosition();
						const Double2 entityToPlayerPositionXZ = (playerPosition - entityPosition).getXZ();
						const double deltaX = entityToPlayerPositionXZ.x;
						const double deltaZ = entityToPlayerPositionXZ.y;
						const VoxelFacing3D facing = (std::abs(deltaX) >= std::abs(deltaZ)) ?
							((deltaX >= 0.0) ? VoxelFacing3D::PositiveX : VoxelFacing3D::NegativeX) :
							((deltaZ >= 0.0) ? VoxelFacing3D::PositiveZ : VoxelFacing3D::NegativeZ);
						const CoordInt3 entityVoxelCoord(entityChunkPos, entityVoxel);

						RayCastHit transitionHit;
						transitionHit.initVoxel(hit.t, hit.worldPoint, entityVoxelCoord, facing);

						MapLogic::handleMapTransition(game, transitionHit, transitionDef);
					}
					else
					{
						GameWorldUI::setInteractionType(GameWorldInteractionType::Default);
					}
				}

				break;
			}
			default:
				break;
			}
		}
		else
		{
			// Reset interaction on right click.
			GameWorldUI::setInteractionType(GameWorldInteractionType::Default);
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
	if (!game.canPlayerMoveAndTurn())
	{
		return Double2::Zero;
	}

	const InputManager &inputManager = game.inputManager;

	const Options &options = game.options;
	const bool modernInterface = options.getGraphics_ModernInterface();
	if (!modernInterface)
	{
		// Classic interface mode.
		const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);
		const bool left = inputManager.keyIsDown(SDL_SCANCODE_A);
		const bool right = inputManager.keyIsDown(SDL_SCANCODE_D);
		const bool lCtrl = inputManager.keyIsDown(SDL_SCANCODE_LCTRL);

		const Player &player = game.player;
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

	const bool canPlayerMoveAndTurn = game.canPlayerMoveAndTurn();

	const Options &options = game.options;
	const bool isGhostModeEnabled = options.getMisc_GhostMode();
	const bool modernInterface = options.getGraphics_ModernInterface();

	PlayerInputAcceleration inputAcceleration;
	if (!modernInterface)
	{
		inputAcceleration = PlayerLogic::getInputAccelerationClassic(player, maxMoveSpeed, isOnGround, canJump, isClimbing, canPlayerMoveAndTurn, ceilingScale, isGhostModeEnabled, inputManager, nativeCursorRegions);
	}
	else
	{
		inputAcceleration = PlayerLogic::getInputAccelerationModern(player, maxMoveSpeed, isOnGround, canJump, isClimbing, canPlayerMoveAndTurn, ceilingScale, isGhostModeEnabled, inputManager);
	}

	return inputAcceleration;
}

CardinalDirectionName PlayerLogic::getRandomMeleeSwingDirection(Random &random)
{
	constexpr int directionCount = static_cast<int>(std::size(CardinalDirection::DisplayNames));
	const int randomValue = random.next(directionCount);
	return static_cast<CardinalDirectionName>(randomValue);
}

bool PlayerLogic::canHearCreatureSound(const Double3 &playerPosition, const Double3 &soundPosition)
{
	constexpr double maxSoundDistanceSqr = PlayerConstants::CREATURE_SOUND_MAX_DISTANCE * PlayerConstants::CREATURE_SOUND_MAX_DISTANCE;
	const Double3 dirToSoundPosition = soundPosition - playerPosition;
	const double soundDistanceSqr = dirToSoundPosition.lengthSquared();
	return soundDistanceSqr <= maxSoundDistanceSqr;
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
	if (!game.canPlayerMoveAndTurn())
	{
		return;
	}

	Player &player = game.player;
	WeaponAnimationInstance &weaponAnimInst = player.weaponAnimInst;
	const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
	const WeaponAnimationDefinitionID weaponAnimDefID = player.getEquippedWeaponAnimationDefID();
	const WeaponAnimationDefinition &weaponAnimDef = weaponAnimLibrary.getDefinition(weaponAnimDefID);
	const WeaponAnimationDefinitionState &weaponAnimDefState = weaponAnimDef.states[weaponAnimInst.currentStateIndex];
	if (!WeaponAnimationUtils::isIdle(weaponAnimDefState))
	{
		return;
	}

	const Options &options = game.options;
	const bool isModernInterface = options.getGraphics_ModernInterface();
	const InputManager &inputManager = game.inputManager;
	GameState &gameState = game.gameState;
	const double ceilingScale = gameState.getActiveCeilingScale();
	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	AudioManager &audioManager = game.audioManager;
	const Window &window = game.window;
	Renderer &renderer = game.renderer;
	Random &random = game.random;
	ArenaRandom &arenaRandom = game.arenaRandom;
	SceneManager &sceneManager = game.sceneManager;
	VoxelChunkManager &voxelChunkManager = sceneManager.voxelChunkManager;
	EntityChunkManager &entityChunkManager = sceneManager.entityChunkManager;

	const ItemDefinitionID equippedItemDefID = player.getEquippedWeaponItemDefID();
	bool anyWeaponEquipped = equippedItemDefID >= 0;
	bool isEquippedWeaponRanged = false;

	if (anyWeaponEquipped)
	{
		const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
		const ItemDefinition &equippedItemDef = itemLibrary.getDefinition(equippedItemDefID);
		DebugAssert(equippedItemDef.type == ItemType::Weapon);
		const WeaponItemDefinition &weaponItemDef = equippedItemDef.weapon;
		isEquippedWeaponRanged = weaponItemDef.isRanged;
	}

	constexpr double playerMeleeSwingRange = PlayerConstants::MELEE_HIT_RANGE;
	constexpr double playerHitSearchRadius = PlayerConstants::MELEE_HIT_SEARCH_RADIUS;
	constexpr double playerHalfHeight = PlayerConstants::TOP_OF_HEAD_HEIGHT / 2.0;
	const WorldDouble3 playerFeetPosition = player.getFeetPosition();

	const bool isAttackMouseButtonDown = inputManager.mouseButtonIsDown(SDL_BUTTON_RIGHT);
	const int weaponAnimIdleStateIndex = weaponAnimInst.currentStateIndex;
	int newStateIndex = weaponAnimIdleStateIndex;
	int nextStateIndex = -1;
	const char *sfxFilename = nullptr;

	if (!isEquippedWeaponRanged)
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

			const WorldDouble3 hitSearchCenterPoint = playerFeetPosition + WorldDouble3(0.0, playerHalfHeight, 0.0) + (player.forward * playerMeleeSwingRange);
			CombatHitSearchResult hitSearchResult;
			CombatLogic::getHitSearchResult(hitSearchCenterPoint, playerHitSearchRadius, ceilingScale, voxelChunkManager, entityChunkManager, &hitSearchResult);

			for (const WorldInt3 hitWorldVoxel : hitSearchResult.getVoxels())
			{
				gameState.addCombatVoxelResult(hitWorldVoxel, anyWeaponEquipped);
			}

			for (const EntityInstanceID hitEntityInstID : hitSearchResult.getEntities())
			{
				constexpr bool isFromMeleeWeapon = true;
				gameState.addCombatEntityResult(hitEntityInstID, isFromMeleeWeapon);
			}
		}
	}
	else
	{
		bool isAttack = false;
		Double2 projectileDirection;
		if (isModernInterface)
		{
			isAttack = isAttackMouseButtonDown;
			projectileDirection = player.getGroundDirectionXZ();
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
			const Int2 mousePosition = inputManager.getMousePosition();
			const int originalCursorY = window.nativeToOriginal(mousePosition).y;
			const bool isCursorInSceneView = originalCursorY < (ArenaRenderUtils::SCREEN_HEIGHT - gameWorldInterfaceHeight);
			isAttack = isAttackMouseButtonDown && isCursorInSceneView;
			projectileDirection = GameWorldUiModel::screenToWorldRayDirection(game, mousePosition).getXZ().normalized();
		}

		if (isAttack)
		{
			weaponAnimDef.tryGetStateIndex(WeaponAnimationUtils::STATE_FIRING.c_str(), &newStateIndex);
			nextStateIndex = weaponAnimIdleStateIndex;
			sfxFilename = ArenaSoundName::ArrowFire;

			CombatLogic::spawnBowProjectile(player.getEyePosition(), projectileDirection, entityChunkManager, random, game.physicsSystem, renderer);
		}
	}

	if (newStateIndex != weaponAnimIdleStateIndex)
	{
		weaponAnimInst.setStateIndex(newStateIndex);
		weaponAnimInst.setNextStateIndex(nextStateIndex);

		if (sfxFilename != nullptr)
		{
			audioManager.playSoundOneShot(sfxFilename);
		}
	}
}

void PlayerLogic::handleScreenToWorldInteraction(Game &game, const Int2 &nativePoint, bool isPrimaryInteraction, bool debugFadeVoxel)
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
	const Double3 rayDirection = GameWorldUiModel::screenToWorldRayDirection(game, nativePoint);
	constexpr bool includeEntities = true;

	RayCastHit hit;
	const bool success = Physics::rayCast(rayStart, rayDirection, ceilingScale, cameraDirection,
		includeEntities, voxelChunkManager, entityChunkManager, collisionChunkManager,
		EntityDefinitionLibrary::getInstance(), hit);

	const GameWorldInteractionType interactionType = GameWorldUI::state.interactionType;
	if (!success)
	{
		if (interactionType != GameWorldInteractionType::Default)
		{
			GameWorldUI::setInteractionType(GameWorldInteractionType::Default);
		}
		
		return;
	}

	if (hit.type == RayCastHitType::Voxel)
	{
		PlayerLogic::handleRayCastHitVoxel(game, hit, isPrimaryInteraction, interactionType, debugFadeVoxel, ceilingScale, voxelChunkManager);
	}
	else if (hit.type == RayCastHitType::Entity)
	{
		PlayerLogic::handleRayCastHitEntity(game, hit, isPrimaryInteraction, interactionType, ceilingScale, voxelChunkManager, entityChunkManager);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(hit.type)));
	}
}
