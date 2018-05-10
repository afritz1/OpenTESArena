#ifndef NON_PLAYER_H
#define NON_PLAYER_H

#include <vector>

#include "Animation.h"
#include "Camera2D.h"
#include "Entity.h"

// Essentially an actor class, a non-player is an NPC or creature, usually with
// an AI for movement and/or combat, whose texture depends on their position
// relative to the player.

class NonPlayer : public Entity
{
private:
	enum class AnimationType
	{
		Idle,
		Move,
		Attack,
		Death
	};

	// Idle and move animations for each direction the entity is facing.
	// Some animations are duplicates, but they are flipped by the renderer.
	std::vector<Animation> idleAnimations, moveAnimations;

	Animation attackAnimation, deathAnimation;
	Camera2D camera;
	Double2 velocity;

	// Gets the current animation type, dependent on the entity's state.
	NonPlayer::AnimationType getAnimationType() const;
public:
	NonPlayer(const Double3 &position, const Double2 &direction,
		const std::vector<Animation> &idleAnimations,
		const std::vector<Animation> &moveAnimations,
		const Animation &attackAnimation, const Animation &deathAnimation,
		EntityManager &entityManager);
	virtual ~NonPlayer() = default;

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const override;

	virtual EntityType getEntityType() const override;
	virtual const Double3 &getPosition() const override;

	virtual void tick(Game &game, double dt) override;
};

#endif
