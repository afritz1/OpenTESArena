#ifndef PLAYER_H
#define PLAYER_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "Camera3D.h"
#include "PrimaryAttributeSet.h"
#include "WeaponAnimation.h"
#include "../Assets/MIFUtils.h"
#include "../World/Coord.h"

class CharacterClassLibrary;
class CollisionChunkManager;
class ExeData;
class Game;
class LevelDefinition;
class LevelInfoDefinition;
class Random;
class VoxelChunkManager;

class Player
{
private:
	std::string displayName;
	bool male;
	int raceID;
	int charClassDefID;
	int portraitID;
	Camera3D camera;
	VoxelDouble3 velocity;
	double maxWalkSpeed; // Eventually a function of 'Speed'.
	double friction;
	WeaponAnimation weaponAnimation;
	PrimaryAttributeSet attributes;
	// Other stats...
	JPH::BodyID physicsBodyID;

	// Gets the Y position of the player's feet.
	double getFeetY() const;

	// Changes the player's velocity based on collision with objects in the world.
	void handleCollision(double dt, const VoxelChunkManager &voxelChunkManager, const CollisionChunkManager &collisionChunkManager, double ceilingScale);

	// Updates the player's position and velocity based on interactions with the world.
	void updatePhysics(double dt, const VoxelChunkManager &voxelChunkManager, const CollisionChunkManager &collisionChunkManager, double ceilingScale);
public:
	// Distance from player's feet to head.
	static constexpr double HEIGHT = 60.0 / MIFUtils::ARENA_UNITS;

	// Arbitrary values for movement speed.
	static constexpr double DEFAULT_WALK_SPEED = 2.0;

	Player();
	~Player();

	// Make player with rolled attributes based on race & gender.
	void init(const std::string &displayName, bool male, int raceID, int charClassDefID,
		int portraitID, const CoordDouble3 &position, const Double3 &direction, const Double3 &velocity,
		double maxWalkSpeed, int weaponID, const ExeData &exeData, JPH::PhysicsSystem &physicsSystem, Random &random);

	// Make player with given attributes.
	void init(const std::string &displayName, bool male, int raceID, int charClassDefID, PrimaryAttributeSet &&attributes,
		int portraitID, const CoordDouble3 &position, const Double3 &direction, const Double3 &velocity,
		double maxWalkSpeed, int weaponID, const ExeData &exeData, JPH::PhysicsSystem &physicsSystem);

	// Initializes a random player for testing.
	void initRandom(const CharacterClassLibrary &charClassLibrary, const ExeData &exeData, JPH::PhysicsSystem &physicsSystem, Random &random);

	void freePhysicsBody(JPH::PhysicsSystem &physicsSystem);

	const CoordDouble3 &getPosition() const;
	const std::string &getDisplayName() const;
	std::string getFirstName() const;
	int getPortraitID() const;
	bool isMale() const;
	int getRaceID() const;
	int getCharacterClassDefID() const;
	const PrimaryAttributeSet &getAttributes() const;

	// Gets the direction the player is facing.
	const Double3 &getDirection() const;

	// Gets the direction pointing right from the player's direction.
	const Double3 &getRight() const;

	// Gets the bird's eye view of the player's direction (in the XZ plane).
	Double2 getGroundDirection() const;

	// Gets the velocity vector of the player.
	const VoxelDouble3 &getVelocity() const;

	// Gets the strength of the player's jump (i.e., instantaneous change in Y velocity).
	double getJumpMagnitude() const;

	// Gets the player's weapon animation for displaying on-screen.
	WeaponAnimation &getWeaponAnimation();
	const WeaponAnimation &getWeaponAnimation() const;

	JPH::BodyID getPhysicsBodyID() const;

	// Returns whether the player is standing on ground and with no Y velocity.
	bool onGround(const CollisionChunkManager &collisionChunkManager) const;

	// Teleports the player to a point.
	void teleport(const CoordDouble3 &position);

	// Rotates the player's camera based on some change in X (left/right) and Y (up/down).
	void rotate(double dx, double dy, double hSensitivity, double vSensitivity, double pitchLimit);

	// Recalculates the player's view so they look at a point.
	void lookAt(const CoordDouble3 &point);

	// Sets velocity vector to zero. Intended for stopping the player after level transitions.
	void setVelocityToZero();

	// Flattens direction vector to the horizon (used when switching classic/modern camera mode).
	void setDirectionToHorizon();

	// Changes the velocity (as a force) given a normalized direction, magnitude, 
	// and delta time, as well as whether the player is running.
	void accelerate(const Double3 &direction, double magnitude, double dt);

	// Changes the velocity instantly. Intended for instantaneous acceleration like 
	// jumping.
	void accelerateInstant(const Double3 &direction, double magnitude);

	// Tick the player by delta time for motion, etc..
	void tick(Game &game, double dt);
};

#endif
