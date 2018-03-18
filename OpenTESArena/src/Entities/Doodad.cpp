#include "Doodad.h"
#include "EntityType.h"

Doodad::Doodad(const Animation &animation, const Double3 &position,
	EntityManager &entityManager)
	: Entity(entityManager), animation(animation), position(position) { }

std::unique_ptr<Entity> Doodad::clone(EntityManager &entityManager) const
{
	return std::make_unique<Doodad>(this->animation, this->position, entityManager);
}

EntityType Doodad::getEntityType() const
{
	return EntityType::Doodad;
}

const Double3 &Doodad::getPosition() const
{
	return this->position;
}

bool Doodad::facesPlayer() const
{
	return true;
}

void Doodad::tick(Game &game, double dt)
{
	// Animate.
	this->animation.tick(dt);
	this->textureID = this->animation.getCurrentID();
}
