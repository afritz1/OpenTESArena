#include <algorithm>
#include <cmath>
#include <vector>

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"

#include "CharacterClassDefinition.h"
#include "CharacterClassLibrary.h"
#include "Player.h"
#include "../Collision/CollisionChunk.h"
#include "../Collision/CollisionChunkManager.h"
#include "../Collision/Physics.h"
#include "../Collision/PhysicsLayer.h"
#include "../Game/Game.h"
#include "../Game/GameState.h"
#include "../Game/Options.h"
#include "../Items/ItemLibrary.h"
#include "../Math/Constants.h"
#include "../Math/Quaternion.h"
#include "../Math/Random.h"
#include "../Voxels/VoxelChunkManager.h"
#include "../World/CardinalDirection.h"

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
		
		constexpr uint64_t characterVirtualUserData = 0;
		*outCharacterVirtual = new JPH::CharacterVirtual(&characterVirtualSettings, JPH::Vec3Arg::sZero(), JPH::QuatArg::sIdentity(), characterVirtualUserData, &physicsSystem);
		(*outCharacterVirtual)->SetCharacterVsCharacterCollision(outCharVsCharCollision);
		outCharVsCharCollision->Add(*outCharacterVirtual);
		(*outCharacterVirtual)->SetListener(nullptr); // @todo
		DebugLogError("\nNeed characterVirtual's contact listeners\n");

		return true;
	}

	std::string GetFirstName(const std::string &fullName)
	{
		Buffer<std::string> nameTokens = String::split(fullName);
		return nameTokens[0];
	}

	int GetRandomWeaponIdForClass(const CharacterClassDefinition &charClassDef, Random &random)
	{
		const int allowedWeaponCount = charClassDef.getAllowedWeaponCount();
		Buffer<int> weapons(allowedWeaponCount + 1);
		for (int i = 0; i < allowedWeaponCount; i++)
		{
			weapons.set(i, charClassDef.getAllowedWeapon(i));
		}

		// Fists.
		weapons.set(allowedWeaponCount, -1);

		const int randIndex = random.next(weapons.getCount());
		return weapons.get(randIndex);
	}
}

Player::Player()
{
	this->male = false;
	this->raceID = -1;
	this->charClassDefID = -1;
	this->portraitID = -1;
	this->setCameraFrame(-Double3::UnitX); // Avoids audio listener issues w/ uninitialized player.
	this->maxWalkSpeed = 0.0;
	this->physicsCharacter = nullptr;
	this->physicsCharacterVirtual = nullptr;
}

Player::~Player()
{
	DebugAssert(this->physicsCharacter == nullptr);
	DebugAssert(this->physicsCharacterVirtual == nullptr);
}

void Player::init(const std::string &displayName, bool male, int raceID, int charClassDefID,
	int portraitID, const CoordDouble3 &position, const Double3 &direction, const Double3 &velocity,
	double maxWalkSpeed, int weaponID, const ExeData &exeData, JPH::PhysicsSystem &physicsSystem, Random &random)
{
	this->displayName = displayName;
	this->firstName = GetFirstName(displayName);
	this->male = male;
	this->raceID = raceID;
	this->charClassDefID = charClassDefID;
	this->portraitID = portraitID;
	this->maxWalkSpeed = maxWalkSpeed;
	this->weaponAnimation.init(weaponID, exeData);
	this->primaryAttributes.init(raceID, male, exeData);
	this->inventory.clear();
	
	if (!TryCreatePhysicsCharacters(physicsSystem, &this->physicsCharacter, &this->physicsCharacterVirtual, &this->physicsCharVsCharCollision))
	{
		DebugCrash("Couldn't create player physics collider.");
	}

	this->setPhysicsPosition(VoxelUtils::coordToWorldPoint(position));
	this->setPhysicsVelocity(velocity);
	this->setCameraFrame(direction);
}

void Player::init(const std::string &displayName, bool male, int raceID, int charClassDefID,
	const PrimaryAttributes &primaryAttributes, int portraitID, const CoordDouble3 &position, const Double3 &direction,
	const Double3 &velocity, double maxWalkSpeed, int weaponID, const ExeData &exeData, JPH::PhysicsSystem &physicsSystem)
{
	this->displayName = displayName;
	this->firstName = GetFirstName(displayName);
	this->male = male;
	this->raceID = raceID;
	this->charClassDefID = charClassDefID;
	this->portraitID = portraitID;
	this->maxWalkSpeed = maxWalkSpeed;
	this->weaponAnimation.init(weaponID, exeData);	
	this->primaryAttributes = primaryAttributes;
	this->inventory.clear();
	
	if (!TryCreatePhysicsCharacters(physicsSystem, &this->physicsCharacter, &this->physicsCharacterVirtual, &this->physicsCharVsCharCollision))
	{
		DebugCrash("Couldn't create player physics collider.");
	}

	this->setPhysicsPosition(VoxelUtils::coordToWorldPoint(position));
	this->setPhysicsVelocity(velocity);
	this->setCameraFrame(direction);
}

void Player::initRandom(const CharacterClassLibrary &charClassLibrary, const ExeData &exeData, JPH::PhysicsSystem &physicsSystem, Random &random)
{
	this->displayName = "Player";
	this->firstName = GetFirstName(this->displayName);
	this->male = random.next(2) == 0;
	this->raceID = random.next(8);
	this->charClassDefID = random.next(charClassLibrary.getDefinitionCount());
	this->portraitID = random.next(10);
	this->maxWalkSpeed = PlayerConstants::DEFAULT_WALK_SPEED;

	const CharacterClassDefinition &charClassDef = charClassLibrary.getDefinition(this->charClassDefID);
	const int weaponID = GetRandomWeaponIdForClass(charClassDef, random);
	this->weaponAnimation.init(weaponID, exeData);
	this->primaryAttributes.init(this->raceID, this->male, exeData);	
	this->inventory.clear();
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

	const CoordDouble3 position(ChunkInt2::Zero, VoxelDouble3::Zero);
	this->setPhysicsPosition(VoxelUtils::coordToWorldPoint(position));
	this->setPhysicsVelocity(Double3::Zero);

	const Double3 direction(CardinalDirection::North.x, 0.0, CardinalDirection::North.y);
	this->setCameraFrame(direction);
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

Double2 Player::getGroundDirection() const
{
	return Double2(this->forward.x, this->forward.z).normalized();
}

double Player::getJumpMagnitude() const
{
	return PlayerConstants::JUMP_VELOCITY;
}

bool Player::onGround() const
{
	// @todo: not sure we should ever be on steep ground in this engine. "maxSlopeAngle" affects that, and 0 and 90 don't seem perfect.
	const JPH::CharacterBase::EGroundState groundState = this->physicsCharacter->GetGroundState();
	return (groundState == JPH::CharacterBase::EGroundState::OnGround) || (groundState == JPH::CharacterBase::EGroundState::OnSteepGround);
}

bool Player::isMoving() const
{
	const JPH::RVec3 physicsVelocity = this->physicsCharacter->GetLinearVelocity();
	return physicsVelocity.LengthSq() >= ConstantsF::Epsilon;
}

bool Player::canJump() const
{
	const JPH::RVec3 physicsVelocity = this->physicsCharacter->GetLinearVelocity();
	constexpr float tinyEpsilon = 1e-8f;
	return this->onGround() && (std::abs(physicsVelocity.GetY()) <= tinyEpsilon);
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
	const Double2 groundDirection = this->getGroundDirection();
	const Double3 newForward = Double3(groundDirection.x, 0.0, groundDirection.y).normalized();
	this->setCameraFrame(newForward);
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

	Double2 newVelocityXZ(newVelocity.x, newVelocity.z);
	if (newVelocityXZ.length() > this->maxWalkSpeed)
	{
		newVelocityXZ = newVelocityXZ.normalized() * this->maxWalkSpeed; // @todo: this is doing nothing but looks important
	}

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

void Player::prePhysicsStep(double dt, Game &game)
{
	if (game.options.getMisc_GhostMode())
	{
		// Prevent leftover momentum when switching cheat modes.
		this->setPhysicsVelocity(Double3::Zero);
		return;
	}

	const Double3 oldVelocity = this->getPhysicsVelocity();
	if (!this->onGround())
	{
		// Need to apply gravity to Character as its gravity factor is 0 when with CharacterVirtual.
		this->accelerate(-Double3::UnitY, Physics::GRAVITY, dt);
	}

	JPH::PhysicsSystem &physicsSystem = game.physicsSystem;
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
