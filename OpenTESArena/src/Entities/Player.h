#ifndef PLAYER_H
#define PLAYER_H

#include "Camera3D.h"
#include "WeaponAnimation.h"
#include "../Assets/MIFUtils.h"

class CharacterClassLibrary;
class ExeData;
class Game;
class Random;
class WorldData;

class Player
{
private:
	static constexpr double STEPPING_HEIGHT = 0.25; // Allowed change in height for stepping on stairs.
	static constexpr double JUMP_VELOCITY = 3.0; // Instantaneous change in Y velocity when jumping.
	
	// Magnitude of -Y acceleration in the air.
	static constexpr double GRAVITY = 9.81;

	// Friction for slowing the player down on ground.
	static constexpr double FRICTION = 4.0;

	std::string displayName;
	bool male;
	int raceID;
	int charClassDefID;
	int portraitID;
	Camera3D camera;
	Double3 velocity;
	double maxWalkSpeed, maxRunSpeed; // Eventually a function of 'Speed'.
	WeaponAnimation weaponAnimation;
	// Other stats...

	// Gets the Y position of the player's feet.
	double getFeetY() const;

	// Changes the player's velocity based on collision with objects in the world.
	void handleCollision(const WorldData &worldData, double dt);

	// Updates the player's position and velocity based on interactions with the world.
	void updatePhysics(const WorldData &worldData, bool collision, double dt);
public:
	Player(const std::string &displayName, bool male, int raceID, int charClassDefID,
		int portraitID, const CoordDouble3 &position, const Double3 &direction, const Double3 &velocity,
		double maxWalkSpeed, double maxRunSpeed, int weaponID, const ExeData &exeData);

	// Distance from player's feet to head.
	static constexpr double HEIGHT = 60.0 / MIFUtils::ARENA_UNITS;

	// Arbitrary values for movement speed.
	static constexpr double DEFAULT_WALK_SPEED = 2.0;
	static constexpr double DEFAULT_RUN_SPEED = 8.0;

	const CoordDouble3 &getPosition() const;
	const std::string &getDisplayName() const;
	std::string getFirstName() const;
	int getPortraitID() const;
	bool isMale() const;
	int getRaceID() const;
	int getCharacterClassDefID() const;

	// Generates a random player for testing.
	static Player makeRandom(const CharacterClassLibrary &charClassLibrary,
		const ExeData &exeData, Random &random);

	// Gets the direction the player is facing.
	const Double3 &getDirection() const;

	// Gets the direction pointing right from the player's direction.
	const Double3 &getRight() const;

	// Gets the bird's eye view of the player's direction (in the XZ plane).
	Double2 getGroundDirection() const;

	// Gets the strength of the player's jump (i.e., instantaneous change in Y velocity).
	double getJumpMagnitude() const;

	// Gets the player's weapon animation for displaying on-screen.
	WeaponAnimation &getWeaponAnimation();
	const WeaponAnimation &getWeaponAnimation() const;

	// Returns whether the player is standing on ground and with no Y velocity.
	bool onGround(const WorldData &worldData) const;

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
	void accelerate(const Double3 &direction, double magnitude,
		bool isRunning, double dt);

	// Changes the velocity instantly. Intended for instantaneous acceleration like 
	// jumping.
	void accelerateInstant(const Double3 &direction, double magnitude);

	// Tick the player by delta time for motion, etc..
	void tick(Game &game, double dt);
};

#endif
