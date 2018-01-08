#ifndef PLAYER_H
#define PLAYER_H

#include "Camera3D.h"
#include "CharacterClass.h"
#include "WeaponAnimation.h"

class Game;
class WorldData;

enum class GenderName;

class Player
{
private:
	static const double HEIGHT; // Distance from player's feet to head.
	static const double STEPPING_HEIGHT; // Allowed change in height for stepping on stairs.
	static const double JUMP_VELOCITY; // Instantaneous change in Y velocity when jumping.
	
	// Magnitude of -Y acceleration in the air.
	static const double GRAVITY;

	// Friction for slowing the player down on ground.
	static const double FRICTION;

	std::string displayName;
	GenderName gender;
	int raceID;
	CharacterClass charClass;
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
	Player(const std::string &displayName, GenderName gender, int raceID,
		const CharacterClass &charClass, int portraitID, const Double3 &position,
		const Double3 &direction, const Double3 &velocity, double maxWalkSpeed,
		double maxRunSpeed, WeaponType weaponType);
	~Player();

	const Double3 &getPosition() const;
	const std::string &getDisplayName() const;
	std::string getFirstName() const;
	int getPortraitID() const;
	GenderName getGenderName() const;
	int getRaceID() const;
	const CharacterClass &getCharacterClass() const;

	// Gets the direction the player is facing.
	const Double3 &getDirection() const;

	// Gets the direction pointing right from the player's direction.
	const Double3 &getRight() const;

	// Gets the bird's eye view of the player's direction (in the XZ plane).
	Double2 getGroundDirection() const;

	// Gets the strength of the player's jump (i.e., instantaneous change in Y velocity).
	double getJumpMagnitude() const;

	// Gets the voxel coordinates of the player.
	Int3 getVoxelPosition() const;

	// Gets the player's weapon animation for displaying on-screen.
	WeaponAnimation &getWeaponAnimation();

	// Returns whether the player is standing on ground and with no Y velocity.
	bool onGround(const WorldData &worldData) const;

	// Teleports the player to a point.
	void teleport(const Double3 &position);

	// Rotates the player's camera based on some change in X (left/right) and Y (up/down).
	void rotate(double dx, double dy, double hSensitivity, double vSensitivity);

	// Recalculates the player's view so they look at a point.
	void lookAt(const Double3 &point);

	// Sets velocity vector to zero. Intended for stopping the player after level transitions.
	void setVelocityToZero();

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
