#include "Doodad.h"
#include "EntityType.h"

Doodad::Doodad(const Animation &animation, const Double3 &position)
	: Entity(EntityType::Doodad), animation(animation), position(position) { }

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
