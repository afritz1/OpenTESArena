#ifndef PLAYER_H
#define PLAYER_H

#include <memory>

#include "Directable.h"
#include "Entity.h"
#include "Movable.h"

// The player is an entity with no sprite, and has extra data pertaining to 
// selected spells, weapon animation, and more.

// Though the player is not rendered, they are still considered part of the
// entity manager for purposes such as physics calculation and AI behavior.

class CharacterClass;

enum class CharacterGenderName;
enum class CharacterRaceName;

class Player : public Entity, public Directable, public Movable
{
private:
	std::unique_ptr<CharacterClass> charClass;
	CharacterGenderName gender;
	CharacterRaceName raceName;
	std::string displayName;
	int portraitID;
	// Attributes can be put in an inherited class.
	// Other stats...

	// Helper methods for Player::rotate().
	void pitch(double radians);
	void yaw(double radians);
public:
	// Default constructor.
	Player(const std::string &displayName, CharacterGenderName gender,
		CharacterRaceName raceName, const CharacterClass &charClass, int portraitID,
		const Float3d &position, const Float3d &direction, const Float3d &velocity, 
		double maxWalkSpeed, double maxRunSpeed, EntityManager &entityManager);

	virtual ~Player();

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const override;
	virtual EntityType getEntityType() const override;

	const std::string &getDisplayName() const;
	std::string getFirstName() const;
	int getPortraitID() const;
	CharacterGenderName getGenderName() const;
	CharacterRaceName getRaceName() const;
	const CharacterClass &getCharacterClass() const;

	// Camera turning.
	void rotate(double dx, double dy, double hSensitivity, double vSensitivity,
		double verticalFOV);

	virtual void tick(GameState *gameState, double dt) override;
};

#endif
