#ifndef PLAYER_H
#define PLAYER_H

#include "Directable.h"
#include "Entity.h"
#include "Movable.h"

// The player is an entity with no sprite, and has extra data pertaining to 
// selected spells, weapon animation, and more.

// Though the player is not rendered, they are still considered part of the
// entity manager for purposes such as physics calculation and AI behavior.

class Player : public Entity, public Directable, public Movable
{
private:
	std::string displayName;
	// Attributes can be put in an inherited class.
	// Other stats...
public:
	Player(const std::string &displayName, const Float3d &position,
		const Float3d &direction, const Float3d &velocity, 
		EntityManager &entityManager);
	virtual ~Player();

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const override;

	const std::string &getDisplayName() const;

	virtual void tick(GameState *gameState, double dt) override;
};

#endif
