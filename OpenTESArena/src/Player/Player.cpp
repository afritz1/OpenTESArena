#include <algorithm>
#include <cmath>
#include <limits>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"

#include "Player.h"
#include "WeaponAnimationLibrary.h"
#include "../Assets/ArenaSoundName.h"
#include "../Audio/MusicLibrary.h"
#include "../Audio/MusicUtils.h"
#include "../Collision/CollisionChunk.h"
#include "../Collision/CollisionChunkManager.h"
#include "../Collision/Physics.h"
#include "../Collision/PhysicsLayer.h"
#include "../Game/Game.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Items/ArenaItemUtils.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"
#include "../Math/Random.h"
#include "../Stats/CharacterClassDefinition.h"
#include "../Stats/CharacterClassLibrary.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/CardinalDirection.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"
#include "components/utilities/String.h"

namespace
{
	static constexpr JPH::ObjectLayer DEFAULT_PLAYER_LAYER = PhysicsLayers::MOVING;
	static constexpr JPH::ObjectLayer GHOST_MODE_LAYER = PhysicsLayers::SENSOR; // Hacky but mostly works

	bool TryCreatePhysicsCharacters(JPH::PhysicsSystem &physicsSystem, bool isGhostModeActive, JPH::Character **outCharacter,
		JPH::CharacterVirtual **outCharacterVirtual, JPH::CharacterVsCharacterCollisionSimple *outCharVsCharCollision)
	{
		if (*outCharacter != nullptr)
		{
			const JPH::BodyID &existingBodyID = (*outCharacter)->GetBodyID();
			if (!existingBodyID.IsInvalid())
			{
				JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
				bodyInterface.RemoveBody(existingBodyID);
				bodyInterface.DestroyBody(existingBodyID);
			}
		}

		// Create same capsule for physical and virtual collider.
		constexpr float playerColliderHeight = static_cast<float>(PlayerConstants::TOP_OF_HEAD_HEIGHT);
		constexpr float capsuleRadius = static_cast<float>(PlayerConstants::COLLIDER_RADIUS);
		constexpr float cylinderHalfHeight = static_cast<float>((playerColliderHeight / 2.0) - capsuleRadius);
		static_assert(cylinderHalfHeight >= 0.0f);
		static_assert(MathUtils::almostEqual((capsuleRadius * 2.0f) + (cylinderHalfHeight * 2.0f), playerColliderHeight));

		JPH::CapsuleShapeSettings capsuleShapeSettings(cylinderHalfHeight, capsuleRadius);
		capsuleShapeSettings.SetEmbedded(); // Marked embedded to prevent it from being freed when its ref count reaches 0.
		// @todo: make sure this ^ isn't leaking when we remove/destroy the body

		JPH::ShapeSettings::ShapeResult capsuleShapeResult = capsuleShapeSettings.Create();
		if (capsuleShapeResult.HasError())
		{
			DebugLogError("Couldn't create Jolt capsule collider settings: " + std::string(capsuleShapeResult.GetError().c_str()));
			return false;
		}

		constexpr float mass = 1.0f;
		constexpr float maxSlopeAngle = MathUtilsF::degToRad(45.0f); // Game world doesn't have slopes so this is unimportant
		const JPH::Plane supportingVolume(JPH::Vec3::sAxisY(), -1.0e10f); // Half space of the character that accepts collisions, we want 100% of them
		const JPH::ObjectLayer objectLayer = !isGhostModeActive ? DEFAULT_PLAYER_LAYER : GHOST_MODE_LAYER;

		// Jolt says "pair a CharacterVirtual with a Character that has no gravity and moves with the CharacterVirtual so other objects collide with it".
		// I just need a capsule that runs into things, jumps, and steps on stairs.
		JPH::CharacterSettings characterSettings;
		characterSettings.SetEmbedded();
		characterSettings.mMass = mass;
		characterSettings.mFriction = static_cast<float>(PlayerConstants::FRICTION);
		characterSettings.mGravityFactor = 0.0f; // Do gravity manually when paired w/ CharacterVirtual.
		characterSettings.mShape = capsuleShapeResult.Get();
		characterSettings.mLayer = objectLayer;
		characterSettings.mMaxSlopeAngle = maxSlopeAngle;
		characterSettings.mSupportingVolume = supportingVolume;

		JPH::CharacterVirtualSettings characterVirtualSettings;
		characterVirtualSettings.SetEmbedded();
		characterVirtualSettings.mMass = mass;
		characterVirtualSettings.mMaxSlopeAngle = maxSlopeAngle;
		characterVirtualSettings.mMaxStrength = 1.0f;
		characterVirtualSettings.mShape = capsuleShapeResult.Get();
		characterVirtualSettings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
		characterVirtualSettings.mCollisionTolerance = 0.001f;
		characterVirtualSettings.mCharacterPadding = 0.025f;
		characterVirtualSettings.mPenetrationRecoverySpeed = 1.0f; // All in one update.
		characterVirtualSettings.mPredictiveContactDistance = 0.035f;
		characterVirtualSettings.mSupportingVolume = supportingVolume;
		characterVirtualSettings.mEnhancedInternalEdgeRemoval = false;
		characterVirtualSettings.mInnerBodyShape = nullptr;
		characterVirtualSettings.mInnerBodyLayer = objectLayer;

		constexpr uint64_t characterUserData = 0;
		*outCharacter = new JPH::Character(&characterSettings, JPH::Vec3Arg::sZero(), JPH::QuatArg::sIdentity(), characterUserData, &physicsSystem);
		(*outCharacter)->AddToPhysicsSystem(JPH::EActivation::Activate);

		const JPH::BodyLockInterface &bodyLockInterface = physicsSystem.GetBodyLockInterface();
		const JPH::BodyLockWrite characterBodyLock(bodyLockInterface, (*outCharacter)->GetBodyID());
		DebugAssert(characterBodyLock.Succeeded());
		JPH::Body &characterBody = characterBodyLock.GetBody();
		characterBody.SetAllowSleeping(false); // Don't refire contact added when waking up inside sensor colliders

		constexpr uint64_t characterVirtualUserData = 0;
		*outCharacterVirtual = new JPH::CharacterVirtual(&characterVirtualSettings, JPH::Vec3Arg::sZero(), JPH::QuatArg::sIdentity(), characterVirtualUserData, &physicsSystem);
		(*outCharacterVirtual)->SetCharacterVsCharacterCollision(outCharVsCharCollision);
		outCharVsCharCollision->Add(*outCharacterVirtual);
		(*outCharacterVirtual)->SetListener(nullptr); // Doesn't seem necessary, Character contact listener is working

		return true;
	}

	std::string GetFirstName(const std::string &fullName)
	{
		Buffer<std::string> nameTokens = String::split(fullName);
		return nameTokens[0];
	}

	void InitWeaponAnimationInstance(WeaponAnimationInstance &animInst, int weaponID)
	{
		const WeaponAnimationLibrary &weaponAnimLibrary = WeaponAnimationLibrary::getInstance();
		const WeaponAnimationDefinition &animDef = weaponAnimLibrary.getDefinition(weaponID);

		animInst.clear();
		for (int i = 0; i < animDef.stateCount; i++)
		{
			const WeaponAnimationDefinitionState &animDefState = animDef.states[i];
			animInst.addState(animDefState.seconds);
		}

		int defaultStateIndex = -1;
		if (!animDef.tryGetStateIndex(WeaponAnimationUtils::STATE_SHEATHED.c_str(), &defaultStateIndex))
		{
			DebugLogError("Couldn't get sheathed state for weapon ID " + std::to_string(weaponID) + ".");
			return;
		}

		animInst.setStateIndex(defaultStateIndex);
	}
}

PlayerGroundState::PlayerGroundState()
{
	this->onGround = false;
	this->secondsSinceOnGround = std::numeric_limits<double>::infinity();
	this->recentlyOnGround = false;
	this->isSwimming = false;
	this->hasSplashedInChasm = false;
	this->canJump = false;
	this->isFeetInsideChasm = false;
}

PlayerClimbingState::PlayerClimbingState()
{
	this->isAccelerationValidForClimbing = false;
	this->shouldStartPercent = 0.0;
}

Player::Player()
{
	this->physicsCharacter = nullptr;
	this->physicsCharacterVirtual = nullptr;
	this->setCameraFrameFromDirection(-Double3::UnitX); // Avoids audio listener issues w/ uninitialized player.
	this->movementType = PlayerMovementType::Default;
	this->movementSoundProgress = 0.0;
	this->male = false;
	this->raceID = -1;
	this->charClassDefID = -1;
	this->portraitID = -1;
	this->maxHealth = 0.0;
	this->currentHealth = 0.0;
	this->maxStamina = 0.0;
	this->currentStamina = 0.0;
	this->maxSpellPoints = 0.0;
	this->currentSpellPoints = 0.0;
	this->weaponAnimDefID = ArenaItemUtils::FistsWeaponID;
	this->queuedMeleeSwingDirection = -1;
	this->level = 0;
	this->experience = 0;
	this->gold = 0;
	this->clearKeyInventory();
}

Player::~Player()
{
	DebugAssert(this->physicsCharacter == nullptr);
	DebugAssert(this->physicsCharacterVirtual == nullptr);
}

void Player::init(const std::string &displayName, bool male, int raceID, int charClassDefID, int portraitID, const PrimaryAttributes &primaryAttributes,
	int maxHealth, int maxStamina, int maxSpellPoints, int gold, int weaponID, bool isGhostModeActive, const ExeData &exeData,
	JPH::PhysicsSystem &physicsSystem)
{
	this->displayName = displayName;
	this->firstName = GetFirstName(displayName);
	this->male = male;
	this->raceID = raceID;
	this->charClassDefID = charClassDefID;
	this->portraitID = portraitID;
	this->maxHealth = maxHealth;
	this->currentHealth = maxHealth;
	this->maxStamina = maxStamina;
	this->currentStamina = maxStamina;
	this->maxSpellPoints = maxSpellPoints;
	this->currentSpellPoints = maxSpellPoints;
	this->weaponAnimDefID = weaponID;
	InitWeaponAnimationInstance(this->weaponAnimInst, this->weaponAnimDefID);
	this->queuedMeleeSwingDirection = -1;
	this->level = 1;
	this->experience = 0;
	this->primaryAttributes = primaryAttributes;
	this->inventory.clear();
	this->gold = gold;
	this->clearKeyInventory();

	if (!TryCreatePhysicsCharacters(physicsSystem, isGhostModeActive, &this->physicsCharacter, &this->physicsCharacterVirtual, &this->physicsCharVsCharCollision))
	{
		DebugCrash("Couldn't create player physics collider.");
	}

	this->setPhysicsPositionRelativeToFeet(WorldDouble3::Zero);
	this->setPhysicsVelocity(Double3::Zero);

	const Double3 cameraDirection(CardinalDirection::North.x, 0.0, CardinalDirection::North.y);
	this->setCameraFrameFromDirection(cameraDirection);
	this->movementType = PlayerMovementType::Default;
	this->movementSoundProgress = 0.0;
}

void Player::freePhysicsBody(JPH::PhysicsSystem &physicsSystem)
{
	if (this->physicsCharacter != nullptr)
	{
		this->physicsCharacter->Release();
		this->physicsCharacter = nullptr;
	}

	if (this->physicsCharacterVirtual != nullptr)
	{
		this->physicsCharacterVirtual->Release();
		this->physicsCharacterVirtual = nullptr;
	}
}

void Player::addToKeyInventory(int keyID)
{
	DebugAssert(keyID >= 0);

	int insertIndex = -1;
	for (int i = 0; i < static_cast<int>(std::size(this->keyInventory)); i++)
	{
		const int currentKeyID = this->keyInventory[i];
		if (currentKeyID == keyID)
		{
			DebugLogFormat("Already have key %d in key inventory.", keyID);
			return;
		}
		else if (currentKeyID == ArenaItemUtils::InvalidDoorKeyID)
		{
			insertIndex = i;
			break;
		}
	}

	if (insertIndex < 0)
	{
		DebugLogWarningFormat("No room in key inventory for key %d.", keyID);
		return;
	}

	this->keyInventory[insertIndex] = keyID;
}

void Player::removeFromKeyInventory(int keyID)
{
	DebugAssert(keyID >= 0);

	int removeIndex = -1;
	for (int i = 0; i < static_cast<int>(std::size(this->keyInventory)); i++)
	{
		const int currentKeyID = this->keyInventory[i];
		if (currentKeyID == keyID)
		{
			removeIndex = i;
			break;
		}
	}

	if (removeIndex < 0)
	{
		DebugLogWarningFormat("Key %d not found for removal.", keyID);
		return;
	}

	this->keyInventory[removeIndex] = ArenaItemUtils::InvalidDoorKeyID;

	// Shift other keys down.
	for (int i = removeIndex; i < (static_cast<int>(std::size(this->keyInventory)) - 1); i++)
	{
		this->keyInventory[i] = this->keyInventory[i + 1];
	}

	const int lastIndex = static_cast<int>(std::size(this->keyInventory) - 1);
	this->keyInventory[lastIndex] = ArenaItemUtils::InvalidDoorKeyID;
}

bool Player::isIdInKeyInventory(int keyID) const
{
	for (int i = 0; i < static_cast<int>(std::size(this->keyInventory)); i++)
	{
		if (this->keyInventory[i] == keyID)
		{
			return true;
		}
	}

	return false;
}

void Player::clearKeyInventory()
{
	std::fill(std::begin(this->keyInventory), std::end(this->keyInventory), ArenaItemUtils::InvalidDoorKeyID);
}

void Player::setCameraFrameFromAngles(Degrees yaw, Degrees pitch)
{
	this->angleX = yaw;
	this->angleY = pitch;
	MathUtils::populateCoordinateFrameFromAngles(yaw, pitch, &this->forward, &this->right, &this->up);
}

void Player::setCameraFrameFromDirection(const Double3 &forward)
{
	DebugAssert(forward.isNormalized());

	Radians newAngleXRadians = std::atan2(forward.x, forward.z);
	if (newAngleXRadians < 0.0)
	{
		newAngleXRadians += Constants::TwoPi;
	}

	const Radians newAngleYRadians = std::asin(-forward.y);

	const Degrees newAngleXDegrees = MathUtils::radToDeg(newAngleXRadians);
	const Degrees newAngleYDegrees = MathUtils::radToDeg(newAngleYRadians);
	this->setCameraFrameFromAngles(newAngleXDegrees, newAngleYDegrees);
}

void Player::rotateX(Degrees deltaX)
{
	const Degrees oldAngleX = this->angleX;
	Degrees newAngleX = std::fmod(oldAngleX + deltaX, 360.0);
	if (newAngleX < 0.0)
	{
		newAngleX += 360.0;
	}

	if (newAngleX != oldAngleX)
	{
		this->setCameraFrameFromAngles(newAngleX, this->angleY);
	}
}

void Player::rotateY(Degrees deltaY, Degrees pitchLimit)
{
	DebugAssert(pitchLimit >= 0.0);
	DebugAssert(pitchLimit <= 90.0);

	const Degrees oldAngleY = this->angleY;
	const Degrees newAngleY = std::clamp(oldAngleY + deltaY, -pitchLimit, pitchLimit);
	if (newAngleY != oldAngleY)
	{
		this->setCameraFrameFromAngles(this->angleX, newAngleY);
	}
}

void Player::lookAt(const WorldDouble3 &targetPosition)
{
	const Double3 newForward = (targetPosition - this->getEyePosition()).normalized();
	this->setCameraFrameFromDirection(newForward);
}

void Player::setDirectionToHorizon()
{
	this->setCameraFrameFromAngles(this->angleX, 0.0);
}

bool Player::isPhysicsInited() const
{
	return (this->physicsCharacter != nullptr) && (this->physicsCharacterVirtual != nullptr);
}

WorldDouble3 Player::getPhysicsPosition() const
{
	if (!this->isPhysicsInited())
	{
		return WorldDouble3::Zero;
	}

	const JPH::RVec3 physicsPosition = this->physicsCharacter->GetPosition();
	return WorldDouble3(
		static_cast<SNDouble>(physicsPosition.GetX()),
		static_cast<double>(physicsPosition.GetY()),
		static_cast<WEDouble>(physicsPosition.GetZ()));
}

Double3 Player::getPhysicsVelocity() const
{
	if (!this->isPhysicsInited())
	{
		return Double3::Zero;
	}

	const JPH::RVec3 physicsVelocity = this->physicsCharacter->GetLinearVelocity();
	return WorldDouble3(
		static_cast<SNDouble>(physicsVelocity.GetX()),
		static_cast<double>(physicsVelocity.GetY()),
		static_cast<WEDouble>(physicsVelocity.GetZ()));
}

void Player::setPhysicsPosition(const WorldDouble3 &position)
{
	DebugAssert(this->isPhysicsInited());
	const JPH::RVec3 physicsPosition(
		static_cast<float>(position.x),
		static_cast<float>(position.y),
		static_cast<float>(position.z));
	this->physicsCharacter->SetPosition(physicsPosition);
	this->physicsCharacterVirtual->SetPosition(physicsPosition);
}

void Player::setPhysicsPositionRelativeToFeet(const WorldDouble3 &feetPosition)
{
	DebugAssert(this->isPhysicsInited());
	const JPH::Shape *colliderShape = this->physicsCharacter->GetShape();
	const JPH::AABox colliderBBox = colliderShape->GetLocalBounds();
	const float colliderHeight = colliderBBox.GetSize().GetY();
	const WorldDouble3 newPhysicsPosition(
		feetPosition.x,
		feetPosition.y + (static_cast<double>(colliderHeight) * 0.5),
		feetPosition.z);
	this->setPhysicsPosition(newPhysicsPosition);
}

void Player::setPhysicsVelocity(const Double3 &velocity)
{
	DebugAssert(this->isPhysicsInited());
	const JPH::RVec3 physicsVelocity(
		static_cast<float>(velocity.x),
		static_cast<float>(velocity.y),
		static_cast<float>(velocity.z));
	this->physicsCharacter->SetLinearVelocity(physicsVelocity);
	this->physicsCharacterVirtual->SetLinearVelocity(physicsVelocity);
}

void Player::setPhysicsVelocityY(double velocityY)
{
	const Double3 currentVelocity = this->getPhysicsVelocity();
	this->setPhysicsVelocity(Double3(currentVelocity.x, velocityY, currentVelocity.z));
}

WorldDouble3 Player::getEyePosition() const
{
	const WorldDouble3 physicsPosition = this->getPhysicsPosition();
	const double topOfHeadY = physicsPosition.y + (PlayerConstants::TOP_OF_HEAD_HEIGHT * 0.50);
	return WorldDouble3(physicsPosition.x, topOfHeadY - PlayerConstants::EYE_TO_TOP_OF_HEAD_DISTANCE, physicsPosition.z);
}

CoordDouble3 Player::getEyeCoord() const
{
	const WorldDouble3 eyePosition = this->getEyePosition();
	return VoxelUtils::worldPointToCoord(eyePosition);
}

WorldDouble3 Player::getFeetPosition() const
{
	const WorldDouble3 physicsPosition = this->getPhysicsPosition();
	return WorldDouble3(physicsPosition.x, physicsPosition.y - (PlayerConstants::TOP_OF_HEAD_HEIGHT * 0.50), physicsPosition.z);
}

Double3 Player::getGroundDirection() const
{
	const Radians angleXRadians = MathUtils::degToRad(this->angleX);
	const double sineYaw = std::sin(angleXRadians);
	const double cosineYaw = std::cos(angleXRadians);
	return Double3(sineYaw, 0.0, cosineYaw).normalized();
}

Double2 Player::getGroundDirectionXZ() const
{
	const Double3 groundDir = this->getGroundDirection();
	return groundDir.getXZ();
}

double Player::getJumpMagnitude() const
{
	return PlayerConstants::JUMP_SPEED;
}

double Player::getMaxMoveSpeed() const
{
	if (!this->groundState.isSwimming || this->raceID == static_cast<int>(Race::Argonian))
		return PlayerConstants::MOVE_SPEED;
	else
		return PlayerConstants::SWIMMING_MOVE_SPEED;
}

bool Player::isMoving() const
{
	const JPH::RVec3 physicsVelocity = this->physicsCharacter->GetLinearVelocity();
	return physicsVelocity.LengthSq() >= ConstantsF::Epsilon;
}

void Player::accelerate(const Double3 &direction, double magnitude, double dt)
{
	DebugAssert(dt >= 0.0);
	DebugAssert(magnitude >= 0.0);
	DebugAssert(std::isfinite(magnitude));
	DebugAssert(direction.isNormalized());

	const Double2 directionXZ = Double2(direction.x, direction.z).normalized();
	const Double2 forwardXZ = this->getGroundDirectionXZ();
	const bool isAccelerationForward = directionXZ.dot(forwardXZ) >= 0.90;
	const bool isPushingEnoughToClimb = magnitude >= 1.0;

	if (this->movementType == PlayerMovementType::Default)
	{
		const Double3 oldVelocity = this->getPhysicsVelocity();
		Double3 newVelocity = oldVelocity + (direction * (magnitude * dt));
		if (!std::isfinite(newVelocity.length()))
		{
			return;
		}

		const double moveSpeed = this->getMaxMoveSpeed();
		const double clampedSpeed = moveSpeed * PlayerConstants::CLAMPED_MOVE_SPEED_PERCENT;
		Double2 newVelocityXZ = newVelocity.getXZ();
		if (newVelocityXZ.length() > clampedSpeed)
		{
			newVelocityXZ = newVelocityXZ.normalized() * clampedSpeed;
		}

		newVelocity.x = newVelocityXZ.x;
		newVelocity.z = newVelocityXZ.y;
		if (newVelocity.length() < Constants::Epsilon)
		{
			newVelocity = Double3::Zero;
		}

		this->climbingState.isAccelerationValidForClimbing = this->groundState.onGround && isAccelerationForward && isPushingEnoughToClimb;

		this->setPhysicsVelocity(newVelocity);
	}
	else if (this->movementType == PlayerMovementType::Climbing)
	{
		this->climbingState.isAccelerationValidForClimbing = isAccelerationForward && isPushingEnoughToClimb;
	}
	else
	{
		DebugNotImplemented();
	}
}

void Player::accelerateInstant(const Double3 &direction, double magnitude)
{
	DebugAssert(direction.isNormalized());
	DebugAssert(magnitude >= 0.0);

	const Double3 oldVelocity = this->getPhysicsVelocity();
	const Double3 newVelocity = oldVelocity + (direction * magnitude);
	if (!std::isfinite(newVelocity.length()))
	{
		return;
	}

	this->setPhysicsVelocity(newVelocity);
}

void Player::setGhostModeActive(bool active, JPH::PhysicsSystem &physicsSystem)
{
	JPH::ObjectLayer objectLayer;
	if (active)
	{
		objectLayer = GHOST_MODE_LAYER;
	}
	else
	{
		objectLayer = DEFAULT_PLAYER_LAYER;
	}

	JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
	bodyInterface.SetObjectLayer(this->physicsCharacter->GetBodyID(), objectLayer);

	// Prevent leftover momentum.
	this->setPhysicsVelocity(Double3::Zero);
}

void Player::updateGroundState(double dt, Game &game, const JPH::PhysicsSystem &physicsSystem)
{
	PlayerGroundState newGroundState;

	if (this->physicsCharacter->IsSupported())
	{
		const JPH::Vec3 groundNormal = this->physicsCharacter->GetGroundNormal();
		const JPH::Vec3 upVector = JPH::Vec3::sAxisY();
		const bool isOnFlatGround = groundNormal.Dot(upVector) >= 0.95f;
		if (isOnFlatGround)
		{
			const JPH::BodyID groundBodyID = this->physicsCharacter->GetGroundBodyID();
			if (!groundBodyID.IsInvalid())
			{
				const JPH::BodyLockInterface &bodyInterface = physicsSystem.GetBodyLockInterface();
				const JPH::BodyLockRead groundBodyLock(bodyInterface, groundBodyID);
				if (groundBodyLock.Succeeded())
				{
					const JPH::Body &groundBody = groundBodyLock.GetBody();
					newGroundState.onGround = !groundBody.IsSensor();
				}
			}
		}
	}

	if (newGroundState.onGround)
	{
		newGroundState.secondsSinceOnGround = 0.0;
	}
	else
	{
		newGroundState.secondsSinceOnGround = this->groundState.secondsSinceOnGround + dt;
	}

	newGroundState.recentlyOnGround = newGroundState.secondsSinceOnGround <= PlayerConstants::MAX_SECONDS_SINCE_ON_GROUND;

	const double ceilingScale = game.gameState.getActiveCeilingScale();
	const WorldDouble3 playerFeetPosition = this->getFeetPosition();
	const CoordDouble3 playerFeetCoord = VoxelUtils::worldPointToCoord(playerFeetPosition);
	const CoordInt3 playerFeetVoxelCoord(playerFeetCoord.chunk, VoxelUtils::pointToVoxel(playerFeetCoord.point, ceilingScale));
	const VoxelInt3 playerFeetVoxel = playerFeetVoxelCoord.voxel;
	const VoxelInt3 clampedPlayerFeetVoxel(playerFeetVoxel.x, std::max(playerFeetVoxel.y, 0), playerFeetVoxel.z);
	const JPH::RVec3 physicsVelocity = this->physicsCharacter->GetLinearVelocity();

	const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
	const VoxelChunk *voxelChunk = voxelChunkManager.findChunkAtPosition(playerFeetVoxelCoord.chunk);
	if (voxelChunk != nullptr)
	{
		VoxelChasmDefID chasmDefID;
		if (voxelChunk->tryGetChasmDefID(playerFeetVoxel.x, clampedPlayerFeetVoxel.y, playerFeetVoxel.z, &chasmDefID))
		{
			const VoxelChasmDefinition &chasmDef = voxelChunkManager.getChasmDef(chasmDefID);
			const VoxelShapeDefID chasmFloorShapeDefID = voxelChunk->shapeDefIDs.get(playerFeetVoxel.x, clampedPlayerFeetVoxel.y, playerFeetVoxel.z);
			const VoxelShapeDefinition &chasmFloorShapeDef = voxelChunk->shapeDefs[chasmFloorShapeDefID];
			DebugAssert(chasmFloorShapeDef.type == VoxelShapeType::Box);
			const double chasmFloorShapeYPos = chasmFloorShapeDef.box.yOffset + chasmFloorShapeDef.box.height;
			const double chasmBottomY = static_cast<double>(clampedPlayerFeetVoxel.y) + MeshUtils::getScaledVertexY(chasmFloorShapeYPos, chasmFloorShapeDef.scaleType, ceilingScale);
			const double chasmTopY = static_cast<double>(clampedPlayerFeetVoxel.y + 1) * ceilingScale;
			const double chasmMiddleY = chasmBottomY + ((chasmTopY - chasmBottomY) * 0.50);
			const double chasmLowerPortionY = chasmBottomY + ((chasmMiddleY - chasmBottomY) * 0.50);
			const bool areFeetInChasm = playerFeetPosition.y <= chasmMiddleY; // Arbitrary "deep enough"
			const bool areFeetInWater = (playerFeetPosition.y <= chasmLowerPortionY) && chasmDef.allowsSwimming;

			newGroundState.isSwimming = newGroundState.recentlyOnGround && chasmDef.allowsSwimming && areFeetInWater;
			newGroundState.hasSplashedInChasm = this->groundState.hasSplashedInChasm;
			newGroundState.isFeetInsideChasm = areFeetInChasm;
		}
	}

	newGroundState.canJump = newGroundState.onGround && !newGroundState.isSwimming;

	this->prevGroundState = this->groundState;
	this->groundState = newGroundState;
}

void Player::prePhysicsStep(double dt, Game &game)
{
	if (game.options.getMisc_GhostMode())
	{
		return;
	}

	if (this->movementType == PlayerMovementType::Default && !this->groundState.onGround)
	{
		// Apply gravity to Character as gravity factor is 0 when with CharacterVirtual.
		this->accelerate(-Double3::UnitY, Physics::GRAVITY, dt);
	}

	// @todo: disabling ExtendedUpdate() fixes the "drift" on level start, not sure if we'll ever need
	// CharacterVirtual. Keeping around until stairstepping is figured out.
	/*const JPH::PhysicsSystem &physicsSystem = game.physicsSystem;
	const JPH::Vec3Arg physicsGravity = -this->physicsCharacter->GetUp() * physicsSystem.GetGravity().Length();
	JPH::CharacterVirtual::ExtendedUpdateSettings extendedUpdateSettings; // @todo: for stepping up/down stairs
	const JPH::BroadPhaseLayerFilter &broadPhaseLayerFilter = physicsSystem.GetDefaultBroadPhaseLayerFilter(PhysicsLayers::MOVING);
	const JPH::ObjectLayerFilter &objectLayerFilter = physicsSystem.GetDefaultLayerFilter(PhysicsLayers::MOVING);
	const JPH::BodyFilter bodyFilter; // Nothing
	const JPH::ShapeFilter shapeFilter; // Nothing

	// Update + stick to floor + walk stairs
	// @todo pretty sure CharacterVirtual is contributing to all the ghost collisions, probably need to configure this better
	this->physicsCharacterVirtual->ExtendedUpdate(
		static_cast<float>(dt),
		physicsGravity,
		extendedUpdateSettings,
		broadPhaseLayerFilter,
		objectLayerFilter,
		bodyFilter,
		shapeFilter,
		*game.physicsTempAllocator);*/
}

void Player::postPhysicsStep(double dt, Game &game)
{
	constexpr float maxSeparationDistance = 1e-5f;
	this->physicsCharacter->PostSimulation(maxSeparationDistance);
	const Double3 physicsVelocity = this->getPhysicsVelocity();

	const JPH::PhysicsSystem &physicsSystem = game.physicsSystem;
	this->updateGroundState(dt, game, physicsSystem);

	AudioManager &audioManager = game.audioManager;
	const GameState &gameState = game.gameState;
	const double ceilingScale = gameState.getActiveCeilingScale();
	const MapType activeMapType = gameState.getActiveMapType();
	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();

	if (this->groundState.isSwimming)
	{
		if (!this->groundState.hasSplashedInChasm)
		{
			this->groundState.hasSplashedInChasm = true;
			audioManager.playSound(ArenaSoundName::Splash);

			if (activeMapType != MapType::Interior)
			{
				const MusicDefinition *swimmingMusicDef = musicLibrary.getRandomMusicDefinition(MusicType::Swimming, game.random);
				audioManager.setMusic(swimmingMusicDef);
			}
		}
	}
	else if (!this->groundState.isFeetInsideChasm)
	{
		this->groundState.hasSplashedInChasm = false;

		if (this->prevGroundState.isFeetInsideChasm)
		{
			if (activeMapType != MapType::Interior)
			{
				const MusicDefinition *exteriorMusicDef = MusicUtils::getExteriorMusicDefinition(gameState.getWeatherDefinition(), gameState.getClock(), game.random);
				audioManager.setMusic(exteriorMusicDef);
			}
		}
	}

	const bool isMovementSoundAccumulating = (this->movementType != PlayerMovementType::Climbing) && this->groundState.recentlyOnGround && this->isMoving();
	if (isMovementSoundAccumulating)
	{
		const Double2 physicsVelocityXZ = physicsVelocity.getXZ();

		const double clampedMoveSpeed = this->getMaxMoveSpeed() * PlayerConstants::CLAMPED_MOVE_SPEED_PERCENT;
		const double movementSoundAccumulationRate = physicsVelocityXZ.length() / clampedMoveSpeed; // ~2 steps/second

		constexpr double maxMovementSoundProgress = 1.0;
		constexpr double leftStepSoundProgress = maxMovementSoundProgress / 2.0;
		constexpr double rightStepSoundProgress = maxMovementSoundProgress;
		const double prevMovementSoundProgress = this->movementSoundProgress;
		this->movementSoundProgress = std::min(this->movementSoundProgress + (movementSoundAccumulationRate * dt), maxMovementSoundProgress);

		const bool isLeftStepReady = (prevMovementSoundProgress < leftStepSoundProgress) && (this->movementSoundProgress >= leftStepSoundProgress);
		const bool isRightStepReady = (prevMovementSoundProgress < rightStepSoundProgress) && (this->movementSoundProgress >= rightStepSoundProgress);

		if (isLeftStepReady || isRightStepReady)
		{
			// Always left step sound in original game
			constexpr const char *movementSoundNames[] =
			{
				nullptr, // Exterior (no sound)
				ArenaSoundName::DirtLeft, // Interior
				ArenaSoundName::MudLeft, // Exterior rain (unused)
				ArenaSoundName::SnowLeft, // Exterior snow (unused)
				ArenaSoundName::Swim // Swimming
			};

			int movementSoundNameIndex = 0;
			if (this->groundState.isSwimming)
			{
				movementSoundNameIndex = 4;
			}
			else if (activeMapType == MapType::Interior)
			{
				movementSoundNameIndex = 1;
			}

			DebugAssertIndex(movementSoundNames, movementSoundNameIndex);
			const char *movementSoundName = movementSoundNames[movementSoundNameIndex];
			if (!String::isNullOrEmpty(movementSoundName))
			{
				audioManager.playSound(movementSoundName);
			}
		}

		const bool shouldResetMovementSoundProgress = isRightStepReady;
		if (shouldResetMovementSoundProgress)
		{
			this->movementSoundProgress = std::fmod(this->movementSoundProgress, maxMovementSoundProgress);
		}
	}
	else
	{
		this->movementSoundProgress = 0.0;
	}

	const WorldDouble3 feetPosition = this->getFeetPosition();
	const CoordDouble3 feetCoord = VoxelUtils::worldPointToCoord(feetPosition);
	if (this->movementType == PlayerMovementType::Default)
	{
		const double isSlowEnoughToStartClimbing = physicsVelocity.length() < 0.01;
		if (!isSlowEnoughToStartClimbing)
		{
			this->climbingState.isAccelerationValidForClimbing = false;
		}

		if (this->climbingState.isAccelerationValidForClimbing)
		{
			const VoxelInt3 feetVoxel = VoxelUtils::pointToVoxel(feetCoord.point, ceilingScale);
			const bool feetInChasmVoxel = feetVoxel.y <= 0; // Dry chasms might be -1

			const bool canAccumulateStartClimbing = this->groundState.onGround && feetInChasmVoxel;
			if (canAccumulateStartClimbing)
			{
				constexpr double startClimbingAccumulationRate = 20.0;
				this->climbingState.shouldStartPercent += startClimbingAccumulationRate * dt;

				if (this->climbingState.shouldStartPercent >= 1.0)
				{
					this->movementType = PlayerMovementType::Climbing;
					this->climbingState.shouldStartPercent = 0.0;
				}
			}
		}
		else
		{
			this->climbingState.shouldStartPercent = 0.0;
		}
	}
	else if (this->movementType == PlayerMovementType::Climbing)
	{
		Double3 newVelocity = Double3::Zero;

		if (this->climbingState.isAccelerationValidForClimbing)
		{
			const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
			const CollisionChunkManager &collisionChunkManager = game.sceneManager.collisionChunkManager;
			const Double3 groundDirection = this->getGroundDirection();
			double climbingFeetTargetY = ceilingScale;

			// If there's a raised platform close by, set its top as the target. Assume they only exist in Y=1.
			constexpr double raisedPlatformGatherDistance = PlayerConstants::CLIMBING_RAISED_PLATFORM_GATHER_DISTANCE;
			constexpr Double3 raisedPlatformGatherDistanceVector(raisedPlatformGatherDistance, 0.0, raisedPlatformGatherDistance);
			const WorldDouble3 raisedPlatformGatherMin = feetPosition - raisedPlatformGatherDistanceVector;
			const WorldDouble3 raisedPlatformGatherMax = raisedPlatformGatherMin + (raisedPlatformGatherDistanceVector * 2.0);
			const WorldInt3 raisedPlatformGatherWorldVoxelMin = VoxelUtils::pointToVoxel(raisedPlatformGatherMin, ceilingScale);
			const WorldInt3 raisedPlatformGatherWorldVoxelMax = VoxelUtils::pointToVoxel(raisedPlatformGatherMax, ceilingScale);
			for (WEInt gatherWorldVoxelZ = raisedPlatformGatherWorldVoxelMin.z; gatherWorldVoxelZ <= raisedPlatformGatherWorldVoxelMax.z; gatherWorldVoxelZ++)
			{
				for (SNInt gatherWorldVoxelX = raisedPlatformGatherWorldVoxelMin.x; gatherWorldVoxelX <= raisedPlatformGatherWorldVoxelMax.x; gatherWorldVoxelX++)
				{
					const WorldInt3 gatherWorldVoxel(gatherWorldVoxelX, 1, gatherWorldVoxelZ);
					const CoordInt3 gatherVoxelCoord = VoxelUtils::worldVoxelToCoord(gatherWorldVoxel);
					const VoxelInt3 gatherVoxel = gatherVoxelCoord.voxel;
					const VoxelChunk &gatherVoxelChunk = voxelChunkManager.getChunkAtPosition(gatherVoxelCoord.chunk);
					const VoxelShapeDefID gatherVoxelShapeDefID = gatherVoxelChunk.shapeDefIDs.get(gatherVoxel.x, gatherVoxel.y, gatherVoxel.z);
					const VoxelShapeDefinition &gatherVoxelShapeDef = gatherVoxelChunk.shapeDefs[gatherVoxelShapeDefID];
					if (gatherVoxelShapeDef.isElevatedPlatform)
					{
						DebugAssert(gatherVoxelShapeDef.type == VoxelShapeType::Box);
						const VoxelBoxShapeDefinition &boxShape = gatherVoxelShapeDef.box;
						const bool isPlatformOnFloor = boxShape.yOffset == 0.0;
						if (isPlatformOnFloor)
						{
							climbingFeetTargetY = std::max(ceilingScale + boxShape.height, climbingFeetTargetY);
						}
					}
				}
			}

			// Extra bias to allow final push to have some air time.
			climbingFeetTargetY += 0.05;

			const WorldDouble3 eyePosition = this->getEyePosition();
			const WorldInt3 eyeWorldVoxel = VoxelUtils::pointToVoxel(eyePosition, ceilingScale);
			const double ceilingCheckY = eyePosition.y + PlayerConstants::EYE_TO_TOP_OF_HEAD_DISTANCE + Constants::Epsilon;
			const WorldInt3 ceilingCheckWorldVoxel = VoxelUtils::pointToVoxel(VoxelDouble3(eyePosition.x, ceilingCheckY, eyePosition.z), ceilingScale);
			const CoordInt3 ceilingCheckVoxelCoord = VoxelUtils::worldVoxelToCoord(ceilingCheckWorldVoxel);
			const VoxelInt3 ceilingCheckVoxel = ceilingCheckVoxelCoord.voxel;
			const CollisionChunk &ceilingCheckCollisionChunk = collisionChunkManager.getChunkAtPosition(ceilingCheckVoxelCoord.chunk);
			const bool isCeilingCheckColliderEnabled = ceilingCheckCollisionChunk.enabledColliders.get(ceilingCheckVoxel.x, ceilingCheckVoxel.y, ceilingCheckVoxel.z);
			const bool isHeadHittingCeiling = (ceilingCheckWorldVoxel.y > eyeWorldVoxel.y) && isCeilingCheckColliderEnabled;
			const bool isDoneClimbing = feetCoord.point.y >= climbingFeetTargetY;

			if (isHeadHittingCeiling)
			{
				this->movementType = PlayerMovementType::Default;
				this->climbingState.isAccelerationValidForClimbing = false;
			}
			else if (!isDoneClimbing)
			{
				constexpr double baseClimbingSpeed = PlayerConstants::CLIMBING_SPEED;
				const double speedMultiplier = (this->charClassDefID == static_cast<int>(Class::Acrobat) || this->raceID == static_cast<int>(Race::Khajiit)) ? 4.0 : 1.0;
				const Double3 climbingVelocity(0.0, baseClimbingSpeed * speedMultiplier, 0.0);
				newVelocity = climbingVelocity;
			}
			else
			{
				// Done climbing.
				this->movementType = PlayerMovementType::Default;
				this->climbingState.isAccelerationValidForClimbing = false;

				const Double3 finalPushVelocity = groundDirection * PlayerConstants::CLIMBING_FINAL_PUSH_SPEED;
				newVelocity = finalPushVelocity;
			}
		}
		else
		{
			this->movementType = PlayerMovementType::Default;
			this->climbingState.isAccelerationValidForClimbing = false;
		}

		this->setPhysicsVelocity(newVelocity);
	}
}
