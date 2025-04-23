#include <algorithm>
#include <cmath>
#include <vector>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"

#include "Player.h"
#include "WeaponAnimationLibrary.h"
#include "../Assets/ArenaSoundName.h"
#include "../Collision/CollisionChunk.h"
#include "../Collision/CollisionChunkManager.h"
#include "../Collision/Physics.h"
#include "../Collision/PhysicsLayer.h"
#include "../Game/Game.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Items/ArenaItemUtils.h"
#include "../Items/ItemLibrary.h"
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
	bool TryCreatePhysicsCharacters(JPH::PhysicsSystem &physicsSystem, JPH::Character **outCharacter, JPH::CharacterVirtual **outCharacterVirtual,
		JPH::CharacterVsCharacterCollisionSimple *outCharVsCharCollision)
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
		constexpr float playerHeight = static_cast<float>(PlayerConstants::HEIGHT);
		constexpr float capsuleRadius = static_cast<float>(PlayerConstants::COLLIDER_RADIUS);
		constexpr float cylinderHalfHeight = static_cast<float>((playerHeight / 2.0) - capsuleRadius);
		static_assert(cylinderHalfHeight >= 0.0f);
		static_assert(MathUtils::almostEqual((capsuleRadius * 2.0f) + (cylinderHalfHeight * 2.0f), playerHeight));

		JPH::CapsuleShapeSettings capsuleShapeSettings(cylinderHalfHeight, capsuleRadius);
		capsuleShapeSettings.SetEmbedded(); // Marked embedded to prevent it from being freed when its ref count reaches 0.
		// @todo: make sure this ^ isn't leaking when we remove/destroy the body

		JPH::ShapeSettings::ShapeResult capsuleShapeResult = capsuleShapeSettings.Create();
		if (capsuleShapeResult.HasError())
		{
			DebugLogError("Couldn't create Jolt capsule collider settings: " + std::string(capsuleShapeResult.GetError().c_str()));
			return false;
		}

		constexpr float collisionTolerance = 0.05f; // from Jolt example
		constexpr float maxSlopeAngle = MathUtilsF::degToRad(1.0f); // Game world doesn't have slopes, so this should be very small, but 0 causes 500 foot jumps off walls.
		constexpr float maxStrength = 100.0f; // from Jolt example
		constexpr float characterPadding = 0.02f; // from Jolt example
		constexpr float penetrationRecoverySpeed = 1.0f; // from Jolt example
		constexpr float predictiveContactDistance = 0.1f; // from Jolt example
		const JPH::Plane supportingVolume(JPH::Vec3::sAxisY(), -1.0e10f); // from Jolt default values (half space of the character that accepts collisions, we want 100%)

		// Jolt says "pair a CharacterVirtual with a Character that has no gravity and moves with the CharacterVirtual so other objects collide with it".
		// I just need a capsule that runs into things, jumps, and steps on stairs.
		JPH::CharacterSettings characterSettings;
		characterSettings.SetEmbedded();
		characterSettings.mMass = 1.0f;
		characterSettings.mFriction = static_cast<float>(PlayerConstants::FRICTION);
		characterSettings.mGravityFactor = 0.0f; // Do gravity manually when paired w/ CharacterVirtual.
		characterSettings.mShape = capsuleShapeResult.Get();
		characterSettings.mLayer = PhysicsLayers::MOVING;
		characterSettings.mMaxSlopeAngle = maxSlopeAngle;
		characterSettings.mSupportingVolume = supportingVolume;

		JPH::CharacterVirtualSettings characterVirtualSettings;
		characterVirtualSettings.SetEmbedded();
		characterVirtualSettings.mMass = 1.0f;
		characterVirtualSettings.mMaxSlopeAngle = maxSlopeAngle;
		characterVirtualSettings.mMaxStrength = maxStrength;
		characterVirtualSettings.mShape = capsuleShapeResult.Get();
		characterVirtualSettings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
		characterVirtualSettings.mCharacterPadding = characterPadding;
		characterVirtualSettings.mPenetrationRecoverySpeed = penetrationRecoverySpeed;
		characterVirtualSettings.mPredictiveContactDistance = predictiveContactDistance;
		characterVirtualSettings.mSupportingVolume = supportingVolume;
		characterVirtualSettings.mEnhancedInternalEdgeRemoval = false;
		characterVirtualSettings.mInnerBodyShape = nullptr;
		characterVirtualSettings.mInnerBodyLayer = PhysicsLayers::MOVING;

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
	this->isSwimming = false;
	this->enteredWater = false;
	this->canJump = false;
}

Player::Player()
{
	this->physicsCharacter = nullptr;
	this->physicsCharacterVirtual = nullptr;
	this->setCameraFrame(-Double3::UnitX); // Avoids audio listener issues w/ uninitialized player.
	this->movementSoundProgress = 0.0;
	this->male = false;
	this->raceID = -1;
	this->charClassDefID = -1;
	this->portraitID = -1;
	this->weaponAnimDefID = ArenaItemUtils::FistsWeaponID;
	this->level = 0;
	this->experience = 0;
	this->clearKeyInventory();
}

Player::~Player()
{
	DebugAssert(this->physicsCharacter == nullptr);
	DebugAssert(this->physicsCharacterVirtual == nullptr);
}

void Player::init(const std::string &displayName, bool male, int raceID, int charClassDefID,
	const PrimaryAttributes &primaryAttributes, int portraitID, int weaponID, const ExeData &exeData,
	JPH::PhysicsSystem &physicsSystem)
{
	this->displayName = displayName;
	this->firstName = GetFirstName(displayName);
	this->male = male;
	this->raceID = raceID;
	this->charClassDefID = charClassDefID;
	this->portraitID = portraitID;
	this->weaponAnimDefID = weaponID;
	InitWeaponAnimationInstance(this->weaponAnimInst, this->weaponAnimDefID);
	this->level = 1;
	this->experience = 0;
	this->primaryAttributes = primaryAttributes;
	this->inventory.clear();
	this->clearKeyInventory();

	const ItemLibrary &itemLibrary = ItemLibrary::getInstance();
	for (int i = 0; i < itemLibrary.getCount(); i++)
	{
		const ItemDefinitionID itemDefID = static_cast<ItemDefinitionID>(i);
		this->inventory.insert(itemDefID);
	}
	
	if (!TryCreatePhysicsCharacters(physicsSystem, &this->physicsCharacter, &this->physicsCharacterVirtual, &this->physicsCharVsCharCollision))
	{
		DebugCrash("Couldn't create player physics collider.");
	}

	this->setPhysicsPositionRelativeToFeet(WorldDouble3::Zero);
	this->setPhysicsVelocity(Double3::Zero);

	const Double3 cameraDirection(CardinalDirection::North.x, 0.0, CardinalDirection::North.y);
	this->setCameraFrame(cameraDirection);
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
			DebugLogWarningFormat("Already have key %d in key inventory.", keyID);
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

void Player::setCameraFrame(const Double3 &forward)
{
	DebugAssert(forward.isNormalized());
	this->forward = forward;
	this->right = this->forward.cross(Double3::UnitY).normalized();
	this->up = this->right.cross(this->forward).normalized();
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

WorldDouble3 Player::getEyePosition() const
{
	const WorldDouble3 physicsPosition = this->getPhysicsPosition();
	return WorldDouble3(physicsPosition.x, physicsPosition.y + (PlayerConstants::HEIGHT * 0.50), physicsPosition.z);
}

CoordDouble3 Player::getEyeCoord() const
{
	const WorldDouble3 eyePosition = this->getEyePosition();
	return VoxelUtils::worldPointToCoord(eyePosition);
}

WorldDouble3 Player::getFeetPosition() const
{
	const WorldDouble3 physicsPosition = this->getPhysicsPosition();
	return WorldDouble3(physicsPosition.x, physicsPosition.y - (PlayerConstants::HEIGHT * 0.50), physicsPosition.z);
}

Double3 Player::getGroundDirection() const
{
	return Double3(this->forward.x, 0.0, this->forward.z).normalized();
}

Double2 Player::getGroundDirectionXZ() const
{
	return Double2(this->forward.x, this->forward.z).normalized();
}

double Player::getJumpMagnitude() const
{
	return PlayerConstants::JUMP_VELOCITY;
}

bool Player::isMoving() const
{
	const JPH::RVec3 physicsVelocity = this->physicsCharacter->GetLinearVelocity();
	return physicsVelocity.LengthSq() >= ConstantsF::Epsilon;
}

void Player::rotateX(Degrees deltaX)
{
	DebugAssert(std::isfinite(this->forward.length()));
	const Radians deltaAsRadians = MathUtils::safeDegToRad(deltaX);
	const Quaternion quat = Quaternion::fromAxisAngle(-Double3::UnitY, deltaAsRadians) * Quaternion(this->forward, 0.0);	
	const Double3 newForward = Double3(quat.x, quat.y, quat.z).normalized();
	this->setCameraFrame(newForward);
}

void Player::rotateY(Degrees deltaY, Degrees pitchLimit)
{
	DebugAssert(std::isfinite(this->forward.length()));
	DebugAssert(pitchLimit >= 0.0);
	DebugAssert(pitchLimit < 90.0);

	const Radians deltaAsRadians = MathUtils::safeDegToRad(deltaY);
	const Radians currentAngle = std::acos(this->forward.normalized().y);
	const Radians requestedAngle = currentAngle - deltaAsRadians;

	// Clamp to avoid breaking cross product.
	const Radians maxAngle = MathUtils::degToRad(90.0 - pitchLimit);
	const Radians minAngle = MathUtils::degToRad(90.0 + pitchLimit);
	const Radians actualDeltaAngle = (requestedAngle > minAngle) ? (currentAngle - minAngle) : ((requestedAngle < maxAngle) ? (currentAngle - maxAngle) : deltaAsRadians);

	const Quaternion quat = Quaternion::fromAxisAngle(this->right, actualDeltaAngle) * Quaternion(this->forward, 0.0);
	const Double3 newForward = Double3(quat.x, quat.y, quat.z).normalized();
	this->setCameraFrame(newForward);
}

void Player::lookAt(const CoordDouble3 &targetCoord)
{
	const Double3 newForward = (targetCoord - this->getEyeCoord()).normalized();
	this->setCameraFrame(newForward);
}

void Player::setDirectionToHorizon()
{
	const Double3 groundDirection = this->getGroundDirection();
	this->setCameraFrame(groundDirection);
}

void Player::accelerate(const Double3 &direction, double magnitude, double dt)
{
	DebugAssert(dt >= 0.0);
	DebugAssert(magnitude >= 0.0);
	DebugAssert(std::isfinite(magnitude));
	DebugAssert(direction.isNormalized());

	const Double3 oldVelocity = this->getPhysicsVelocity();
	Double3 newVelocity = oldVelocity + (direction * (magnitude * dt));
	if (!std::isfinite(newVelocity.length()))
	{
		return;
	}

	const double clampedSpeed = PlayerConstants::CLAMPED_MOVE_SPEED;
	Double2 newVelocityXZ(newVelocity.x, newVelocity.z);
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

	this->setPhysicsVelocity(newVelocity);
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

void Player::updateGroundState(Game &game, const JPH::PhysicsSystem &physicsSystem)
{
	PlayerGroundState newGroundState;

	const JPH::CharacterBase::EGroundState physicsGroundState = this->physicsCharacter->GetGroundState();
	if ((physicsGroundState == JPH::CharacterBase::EGroundState::OnGround) || (physicsGroundState == JPH::CharacterBase::EGroundState::OnSteepGround))
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

	const WorldDouble3 playerFeetPosition = this->getFeetPosition();
	const CoordDouble3 playerFeetCoord = VoxelUtils::worldPointToCoord(playerFeetPosition);
	const CoordInt3 playerFeetVoxelCoord(playerFeetCoord.chunk, VoxelUtils::pointToVoxel(playerFeetCoord.point));
	const VoxelInt3 playerFeetVoxel = playerFeetVoxelCoord.voxel;
	const JPH::RVec3 physicsVelocity = this->physicsCharacter->GetLinearVelocity();

	const VoxelChunkManager &voxelChunkManager = game.sceneManager.voxelChunkManager;
	const VoxelChunk *voxelChunk = voxelChunkManager.tryGetChunkAtPosition(playerFeetVoxelCoord.chunk);
	if (voxelChunk != nullptr)
	{
		VoxelChasmDefID chasmDefID;
		if (voxelChunk->tryGetChasmDefID(playerFeetVoxel.x, playerFeetVoxel.y, playerFeetVoxel.z, &chasmDefID))
		{
			const VoxelChasmDefinition &chasmDef = voxelChunk->getChasmDef(chasmDefID);
			newGroundState.isSwimming = newGroundState.onGround && chasmDef.allowsSwimming;

			const bool isFallingFastEnoughToSplash = physicsVelocity.GetY() <= -0.8f;
			newGroundState.enteredWater = !this->prevGroundState.isSwimming && newGroundState.isSwimming && isFallingFastEnoughToSplash;
		}
	}

	if (newGroundState.onGround && !newGroundState.isSwimming)
	{
		constexpr float tinyEpsilon = 1e-8f;
		newGroundState.canJump = std::abs(physicsVelocity.GetY()) <= tinyEpsilon;
	}

	this->prevGroundState = this->groundState;
	this->groundState = newGroundState;
}

void Player::prePhysicsStep(double dt, Game &game)
{
	if (game.options.getMisc_GhostMode())
	{
		// Prevent leftover momentum when switching cheat modes.
		this->setPhysicsVelocity(Double3::Zero);
		return;
	}

	const JPH::PhysicsSystem &physicsSystem = game.physicsSystem;
	this->updateGroundState(game, physicsSystem);

	if (!this->groundState.onGround)
	{
		// Apply gravity to Character as gravity factor is 0 when with CharacterVirtual.
		this->accelerate(-Double3::UnitY, Physics::GRAVITY, dt);		
	}

	if (this->groundState.enteredWater)
	{
		AudioManager &audioManager = game.audioManager;
		audioManager.playSound(ArenaSoundName::Splash);
	}

	const Double3 velocity = this->getPhysicsVelocity();
	const bool isMovementSoundAccumulating = this->groundState.onGround && this->isMoving();
	if (isMovementSoundAccumulating)
	{
		const Double3 groundVelocity = Double3(velocity.x, 0.0, velocity.z);

		constexpr double tempRateBias = 2.0; // @temp due to Jolt movement still being bouncy even with enhanced internal edge removal fix
		const double movementSoundProgressRate = (groundVelocity.length() * tempRateBias) / PlayerConstants::CLAMPED_MOVE_SPEED; // ~2 steps/second

		constexpr double maxMovementSoundProgress = 1.0;
		constexpr double leftStepSoundProgress = maxMovementSoundProgress / 2.0;
		constexpr double rightStepSoundProgress = maxMovementSoundProgress;
		const double prevMovementSoundProgress = this->movementSoundProgress;
		this->movementSoundProgress = std::min(this->movementSoundProgress + (movementSoundProgressRate * dt), maxMovementSoundProgress);

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

			const GameState &gameState = game.gameState;
			const MapType activeMapType = gameState.getActiveMapType();

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
				AudioManager &audioManager = game.audioManager;
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

	const JPH::Vec3Arg physicsGravity = -this->physicsCharacter->GetUp() * physicsSystem.GetGravity().Length();
	JPH::CharacterVirtual::ExtendedUpdateSettings extendedUpdateSettings; // @todo: for stepping up/down stairs
	const JPH::BroadPhaseLayerFilter &broadPhaseLayerFilter = physicsSystem.GetDefaultBroadPhaseLayerFilter(PhysicsLayers::MOVING);
	const JPH::ObjectLayerFilter &objectLayerFilter = physicsSystem.GetDefaultLayerFilter(PhysicsLayers::MOVING);
	const JPH::BodyFilter bodyFilter; // Nothing
	const JPH::ShapeFilter shapeFilter; // Nothing

	// Update + stick to floor + walk stairs
	this->physicsCharacterVirtual->ExtendedUpdate(
		static_cast<float>(dt),
		physicsGravity,
		extendedUpdateSettings,
		broadPhaseLayerFilter,
		objectLayerFilter,
		bodyFilter,
		shapeFilter,
		*game.physicsTempAllocator);
}

void Player::postPhysicsStep(Game &game)
{
	if (game.options.getMisc_GhostMode())
	{
		return;
	}

	// @todo: not completely understanding the character + charactervirtual synergy yet
	// - i think charactervirtual is for stairsteps and 'weird interactions' that character gets driven by?

	constexpr float maxSeparationDistance = ConstantsF::Epsilon;
	this->physicsCharacter->PostSimulation(maxSeparationDistance);
}
