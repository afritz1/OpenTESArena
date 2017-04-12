#include "NonPlayer.h"

#include "Animation.h"
#include "EntityType.h"

NonPlayer::NonPlayer(const Double3 &position, const Double2 &direction,
	const std::vector<Animation> &animations, EntityManager &entityManager)
	: Entity(entityManager), camera(position, direction), animations(animations) { }

NonPlayer::~NonPlayer()
{

}

std::unique_ptr<Entity> NonPlayer::clone(EntityManager &entityManager) const
{
	return std::unique_ptr<NonPlayer>(new NonPlayer(
		this->camera.position, this->camera.direction, this->animations, entityManager));
}

EntityType NonPlayer::getEntityType() const
{
	return EntityType::NonPlayer;
}

const Double3 &NonPlayer::getPosition() const
{
	return this->camera.position;
}

void NonPlayer::tick(Game &game, double dt)
{
	// Animate first animation for now. It will depend on player position eventually.
	Animation &animation = this->animations.at(0);
	animation.tick(dt);
	this->textureID = animation.getCurrentID();
}
