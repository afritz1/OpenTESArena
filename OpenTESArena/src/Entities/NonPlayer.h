#ifndef NON_PLAYER_H
#define NON_PLAYER_H

#include <vector>

#include "Camera2D.h"
#include "Entity.h"

// Essentially an actor class, a non-player is an NPC or creature, usually with
// an AI for movement and/or combat, whose texture depends on their position
// relative to the player.

class Animation;

class NonPlayer : public Entity
{
private:
	std::vector<Animation> animations;
	Camera2D camera;
public:
	NonPlayer(const Double3 &position, const Double2 &direction, 
		const std::vector<Animation> &animations, EntityManager &entityManager);
	virtual ~NonPlayer();

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const override;
	
	virtual EntityType getEntityType() const override;
	virtual const Double3 &getPosition() const override;

	virtual void tick(Game &game, double dt) override;
};

#endif
