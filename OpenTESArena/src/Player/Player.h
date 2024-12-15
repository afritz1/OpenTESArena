#ifndef PLAYER_H
#define PLAYER_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Character/Character.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "WeaponAnimation.h"
#include "../Assets/MIFUtils.h"
#include "../Items/ItemInventory.h"
#include "../Math/MathUtils.h"
#include "../Stats/PrimaryAttribute.h"
#include "../World/Coord.h"

class CharacterClassLibrary;
class CollisionChunkManager;
class ExeData;
class Game;
class LevelDefinition;
class LevelInfoDefinition;
class Random;
class VoxelChunkManager;

namespace PlayerConstants
{
	constexpr double HEIGHT = 60.0 / MIFUtils::ARENA_UNITS; // Distance from feet to head.
	constexpr double DEFAULT_WALK_SPEED = 15.0;
	constexpr double FRICTION = 0.30; // Slows down when on ground.
	constexpr double COLLIDER_RADIUS = 0.15; // Radius around the player they will collide at.
	constexpr double COLLIDER_CYLINDER_HALF_HEIGHT = (HEIGHT / 2.0) - COLLIDER_RADIUS;
	constexpr double STEPPING_HEIGHT = 0.25; // Stairsteps delta (used by Jolt CharacterVirtual::ExtendedUpdate()).
	constexpr double JUMP_VELOCITY = 3.0; // Instantaneous change in Y velocity when jumping.
}

struct Player
{
	// Physics state.
	JPH::Character *physicsCharacter;
	JPH::CharacterVirtual *physicsCharacterVirtual;
	JPH::CharacterVsCharacterCollisionSimple physicsCharVsCharCollision;
	Double3 forward; // Camera direction
	Double3 right;
	Double3 up;
	// @todo: polar coordinates (XYZ angles)

	std::string displayName;
	std::string firstName;
	bool male;
	int raceID;
	int charClassDefID;
	int portraitID;
	double maxWalkSpeed; // Eventually a function of 'Speed' attribute
	WeaponAnimation weaponAnimation;
	PrimaryAttributes primaryAttributes;
	ItemInventory inventory;

	Player();
	~Player();

	// Make player with given attributes.
	void init(const std::string &displayName, bool male, int raceID, int charClassDefID, const PrimaryAttributes &primaryAttributes,
		int portraitID, const CoordDouble3 &feetCoord, const Double3 &direction, const Double3 &velocity,
		double maxWalkSpeed, int weaponID, const ExeData &exeData, JPH::PhysicsSystem &physicsSystem);

	// Initializes a random player for testing.
	void initRandom(const CharacterClassLibrary &charClassLibrary, const ExeData &exeData, JPH::PhysicsSystem &physicsSystem, Random &random);

	void freePhysicsBody(JPH::PhysicsSystem &physicsSystem);

	void setCameraFrame(const Double3 &forward);

	bool isPhysicsInited() const;
	WorldDouble3 getPhysicsPosition() const; // Center of the character collider (halfway between eyes and feet).
	Double3 getPhysicsVelocity() const;
	void setPhysicsPosition(const WorldDouble3 &position); // Instantly sets collider position. Drives where camera eye is next.
	void setPhysicsPositionRelativeToFeet(const WorldDouble3 &feetPosition); // Instantly sets collider position using new feet as reference.
	void setPhysicsVelocity(const Double3 &velocity);
	WorldDouble3 getEyePosition() const;
	CoordDouble3 getEyeCoord() const;
	WorldDouble3 getFeetPosition() const;

	// Gets the bird's eye view of the player's direction.
	Double2 getGroundDirection() const;

	// Gets the strength of the player's jump (i.e., instantaneous change in Y velocity).
	double getJumpMagnitude() const;

	bool onGround() const;
	bool isMoving() const;
	bool canJump() const;

	// Pitches and yaws relative to global up vector.
	void rotateX(Degrees deltaX);
	void rotateY(Degrees deltaY, Degrees pitchLimit);

	// Recalculates the player's view so they look at a point. The global up vector is used when generating the new 3D frame,
	// so don't give a point directly above or below the camera.
	void lookAt(const CoordDouble3 &targetCoord);

	// Flattens direction vector to the horizon (used when switching classic/modern camera mode).
	void setDirectionToHorizon();

	// Changes the velocity (as a force) given a normalized direction, magnitude, and delta time.
	void accelerate(const Double3 &direction, double magnitude, double dt); // @todo: this will give CharacterVirtual a force probably?

	// Changes the velocity instantly. Intended for instantaneous acceleration like jumping.
	void accelerateInstant(const Double3 &direction, double magnitude); // @todo: CharacterVirtual should treat this like a jump

	void prePhysicsStep(double dt, Game &game);
	void postPhysicsStep(Game &game);
};

#endif
