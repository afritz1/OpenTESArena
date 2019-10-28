#include "EntityType.h"
#include "NonPlayer.h"
#include "../Math/Constants.h"

NonPlayer::NonPlayer(const Double3 &position, const Double2 &direction,
	const std::vector<Animation> &idleAnimations,
	const std::vector<Animation> &moveAnimations,
	const Animation &attackAnimation, const Animation &deathAnimation)
	: Entity(EntityType::NonPlayer), idleAnimations(idleAnimations), moveAnimations(moveAnimations),
	attackAnimation(attackAnimation), deathAnimation(deathAnimation),
	camera(position, direction), velocity(0.0, 0.0) { }

NonPlayer::AnimationType NonPlayer::getAnimationType() const
{
	// Death animation should override moving animation.

	// If moving, return Move.
	if (this->velocity.length() > Constants::Epsilon)
	{
		return NonPlayer::AnimationType::Move;
	}
	else
	{
		return NonPlayer::AnimationType::Idle;
	}

	// @todo: Check combat state eventually.
}

Double3 NonPlayer::getPosition() const
{
	return this->camera.position;
}

void NonPlayer::tick(Game &game, double dt)
{
	// Animate first animation for now. It will depend on player position eventually.
	Animation &animation = this->idleAnimations.at(0);
	animation.tick(dt);
	this->textureID = animation.getCurrentID();
}
