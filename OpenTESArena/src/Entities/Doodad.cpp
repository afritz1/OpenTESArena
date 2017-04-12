#include "Doodad.h"

#include "EntityType.h"

Doodad::Doodad(const Double3 &position, const Animation &animation,
	EntityManager &entityManager)
	: Entity(entityManager), position(position), animation(animation) { }

Doodad::~Doodad()
{

}

std::unique_ptr<Entity> Doodad::clone(EntityManager &entityManager) const
{
	return std::unique_ptr<Doodad>(new Doodad(
		this->position, this->animation, entityManager));
}

EntityType Doodad::getEntityType() const
{
	return EntityType::Doodad;
}

const Double3 &Doodad::getPosition() const
{
	return this->position;
}

void Doodad::tick(Game &game, double dt)
{
	// Animate.
	this->animation.tick(dt);
	this->textureID = this->animation.getCurrentID();
}
