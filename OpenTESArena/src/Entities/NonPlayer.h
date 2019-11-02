#ifndef NON_PLAYER_H
#define NON_PLAYER_H

#include <vector>

#include "Camera2D.h"
#include "Entity.h"

// @todo: serious refactor. Probably throw away most things and design for new entity
// manager design.

// Essentially an actor class, a non-player is an NPC or creature, usually with
// an AI for movement and/or combat, whose texture depends on their position
// relative to the player.

class NonPlayer : public Entity
{
private:
	Camera2D camera;
	Double2 velocity;
public:
	NonPlayer(const Double2 &position, const Double2 &direction);
	NonPlayer();
	virtual ~NonPlayer() = default;

	virtual void tick(Game &game, double dt) override;
};

#endif
