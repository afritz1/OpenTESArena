#ifndef PLAYER_H
#define PLAYER_H

#include <memory>

#include "Camera3D.h"
#include "Entity.h"

// The player is an entity with no sprite, and has extra data pertaining to 
// selected spells, weapon animation, and more.

// Though the player is not rendered, they are still considered part of the
// entity manager for purposes such as physics calculation and AI behavior.

class CharacterClass;

enum class CharacterRaceName;
enum class GenderName;

class Player : public Entity
{
private:
	std::unique_ptr<CharacterClass> charClass;
	Camera3D camera;
	Double3 velocity;
	double maxWalkSpeed, maxRunSpeed;
	GenderName gender;
	CharacterRaceName raceName;
	std::string displayName;
	int portraitID;
	// Attributes can be put in an inherited class.
	// Other stats...
public:
	// Default constructor.
	Player(const std::string &displayName, GenderName gender,
		CharacterRaceName raceName, const CharacterClass &charClass, int portraitID,
		const Double3 &position, const Double3 &direction, const Double3 &velocity, 
		double maxWalkSpeed, double maxRunSpeed, EntityManager &entityManager);

	virtual ~Player();

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const override;
	virtual EntityType getEntityType() const override;
	virtual const Double3 &getPosition() const override;

	const std::string &getDisplayName() const;
	std::string getFirstName() const;
	int getPortraitID() const;
	GenderName getGenderName() const;
	CharacterRaceName getRaceName() const;
	const CharacterClass &getCharacterClass() const;

	// Gets the direction the player is facing.
	const Double3 &getDirection() const;

	// Gets the direction pointing right from the player's direction.
	const Double3 &getRight() const;

	// Gets the bird's eye view of the player's direction (in the XZ plane).
	Double2 getGroundDirection() const;

	// Teleports the player to a point.
	void teleport(const Double3 &position);

	// Rotates the player's camera based on some change in X (left/right) and Y (up/down).
	void rotate(double dx, double dy, double hSensitivity, double vSensitivity);

	// Changes the velocity (as a force) given a normalized direction, magnitude, 
	// and delta time, as well as whether the entity is running. The direction could 
	// have had its magnitude based on its length, but this way is more explicit.
	void accelerate(const Double3 &direction, double magnitude,
		bool isRunning, double dt);

	virtual void tick(Game &game, double dt) override;
};

#endif
