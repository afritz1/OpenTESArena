#ifndef PLAYER_H
#define PLAYER_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Character/Character.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "ArenaPlayerUtils.h"
#include "WeaponAnimation.h"
#include "../Assets/MIFUtils.h"
#include "../Items/ArenaItemUtils.h"
#include "../Items/ItemInventory.h"
#include "../Math/MathUtils.h"
#include "../Stats/PrimaryAttribute.h"
#include "../World/Coord.h"

class CharacterClassLibrary;
class CollisionChunkManager;
class Game;
class LevelDefinition;
class LevelInfoDefinition;
class Random;
class VoxelChunkManager;

struct ExeData;

namespace PlayerConstants
{
	constexpr double EYE_HEIGHT = static_cast<double>(ArenaPlayerUtils::EyeHeight) / MIFUtils::ARENA_UNITS;
	constexpr double TOP_OF_HEAD_HEIGHT = static_cast<double>(ArenaPlayerUtils::TopOfHeadHeight) / MIFUtils::ARENA_UNITS;
	constexpr double EYE_TO_TOP_OF_HEAD_DISTANCE = TOP_OF_HEAD_HEIGHT - EYE_HEIGHT;
	constexpr double STEPPING_HEIGHT = 0.25; // Stairsteps delta (used by Jolt CharacterVirtual::ExtendedUpdate()).
	constexpr double COLLIDER_RADIUS = 0.20; // Radius around the player they will collide at.
	constexpr double COLLIDER_CYLINDER_HALF_HEIGHT = (TOP_OF_HEAD_HEIGHT / 2.0) - COLLIDER_RADIUS;

	constexpr double MOVE_SPEED = 12.0;
	constexpr double SWIMMING_MOVE_SPEED = MOVE_SPEED * 0.5;
	constexpr double CLAMPED_MOVE_SPEED_PERCENT = 0.4; // Hack, this is less than max speed to retain snappiness of acceleration at 0
	constexpr double CLIMBING_SPEED = 100.0 / MIFUtils::ARENA_UNITS;
	constexpr double CLIMBING_FINAL_PUSH_SPEED = 2.0;
	constexpr double CLIMBING_RAISED_PLATFORM_GATHER_DISTANCE = COLLIDER_RADIUS * 1.15; // Raised platforms affect final climbing height.
	constexpr double GHOST_MODE_SPEED = 15.0; // When ghost mode option is enabled.
	constexpr double JUMP_SPEED = 3.0; // Instantaneous change in Y velocity when jumping.
	constexpr double MAX_SECONDS_SINCE_ON_GROUND = 0.1; // Insulates move sound accumulation from ghost collisions.

	constexpr double FRICTION = 0.30; // Slows down when on ground.

	constexpr double MELEE_HIT_RANGE = 0.50;
	constexpr double MELEE_HIT_SEARCH_RADIUS = 0.40;
}

enum class PlayerMovementType
{
	Default,
	Climbing
};

struct PlayerGroundState
{
	bool onGround;
	double secondsSinceOnGround;
	bool recentlyOnGround;
	bool isSwimming; // For swimming and splash SFXs
	bool hasSplashedInChasm;
	bool canJump;
	bool isFeetInsideChasm; // For restoring music

	PlayerGroundState();
};

struct PlayerClimbingState
{
	bool isAccelerationValidForClimbing; // Is force being applied in a direction that could start climbing?
	double shouldStartPercent; // Accumulate while pushing into wall, start climbing at 100%.

	PlayerClimbingState();
};

struct Player
{
	// Physics state.
	JPH::Character *physicsCharacter;
	JPH::CharacterVirtual *physicsCharacterVirtual;
	JPH::CharacterVsCharacterCollisionSimple physicsCharVsCharCollision;

	// Camera direction
	Double3 forward;
	Double3 right;
	Double3 up;
	Degrees angleX; // Horizontal angle (0-360)
	Degrees angleY; // Vertical angle (-90 to 90)

	PlayerMovementType movementType;
	PlayerGroundState groundState, prevGroundState;
	PlayerClimbingState climbingState;
	double movementSoundProgress;

	std::string displayName;
	std::string firstName;
	bool male;
	int raceID;
	int charClassDefID;
	int portraitID;

	double maxHealth;
	double currentHealth;
	double maxStamina;
	double currentStamina;
	double maxSpellPoints;
	double currentSpellPoints;

	// Player always has a weapon animation even if it's just fists
	int weaponAnimDefID;
	WeaponAnimationInstance weaponAnimInst;
	int queuedMeleeSwingDirection; // Non-negative if player is attempting attack this frame.

	int level;
	int experience;
	PrimaryAttributes primaryAttributes;
	ItemInventory inventory;
	int gold;
	int keyInventory[ArenaItemUtils::DoorKeyCount];

	Player();
	~Player();

	void init(const std::string &displayName, bool male, int raceID, int charClassDefID, int portraitID, const PrimaryAttributes &primaryAttributes,
		int maxHealth, int maxStamina, int maxSpellPoints, int gold, int weaponID, bool isGhostModeActive, const ExeData &exeData,
		JPH::PhysicsSystem &physicsSystem);

	void freePhysicsBody(JPH::PhysicsSystem &physicsSystem);

	void addToKeyInventory(int keyID);
	bool isIdInKeyInventory(int keyID) const;
	void clearKeyInventory();

	void setCameraFrameFromAngles(Degrees yaw, Degrees pitch);
	void setCameraFrameFromDirection(const Double3 &forward);

	// Pitches and yaws relative to global up vector.
	void rotateX(Degrees deltaX);
	void rotateY(Degrees deltaY, Degrees pitchLimit);

	// Recalculates the player's view so they look at a point. The global up vector is used when generating the new 3D frame,
	// so don't give a point directly above or below the camera.
	void lookAt(const WorldDouble3 &targetPosition);

	// Flattens direction vector to the horizon (used when switching classic/modern camera mode).
	void setDirectionToHorizon();

	bool isPhysicsInited() const;
	WorldDouble3 getPhysicsPosition() const; // Center of the character collider (halfway between eyes and feet).
	Double3 getPhysicsVelocity() const;
	void setPhysicsPosition(const WorldDouble3 &position); // Instantly sets collider position. Drives where camera eye is next.
	void setPhysicsPositionRelativeToFeet(const WorldDouble3 &feetPosition); // Instantly sets collider position using new feet as reference.
	void setPhysicsVelocity(const Double3 &velocity);
	void setPhysicsVelocityY(double velocityY); // For jumping
	WorldDouble3 getEyePosition() const;
	CoordDouble3 getEyeCoord() const;
	WorldDouble3 getFeetPosition() const;

	// Gets the bird's eye view of the player's direction.
	Double3 getGroundDirection() const;
	Double2 getGroundDirectionXZ() const;

	// Gets the strength of the player's jump (i.e., instantaneous change in Y velocity).
	double getJumpMagnitude() const;

	double getMaxMoveSpeed() const;
	bool isMoving() const;
	// Changes the velocity (as a force) given a normalized direction, magnitude, and delta time.
	void accelerate(const Double3 &direction, double magnitude, double dt); // @todo: this will give CharacterVirtual a force probably?

	// Changes the velocity instantly. Intended for instantaneous acceleration like jumping.
	void accelerateInstant(const Double3 &direction, double magnitude); // @todo: CharacterVirtual should treat this like a jump

	void setGhostModeActive(bool active, JPH::PhysicsSystem &physicsSystem);

	void updateGroundState(double dt, Game &game, const JPH::PhysicsSystem &physicsSystem);
	void prePhysicsStep(double dt, Game &game);
	void postPhysicsStep(double dt, Game &game);
};

#endif
