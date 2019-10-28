#include "Doodad.h"
#include "EntityType.h"

Doodad::Doodad(const Animation &animation)
	: Entity(EntityType::Doodad), animation(animation) { }

Doodad::Doodad()
	: Doodad(Animation({}, 0.0, false)) { }

Double3 Doodad::getPosition() const
{
	// @todo: remove/revise Double3 getPosition() since I guess this depends on ceiling
	// height of the active level. Pass ceilingHeight as a parameter? Ehh...
	return Double3(this->positionXZ.x, 0.0, this->positionXZ.y);
}

void Doodad::tick(Game &game, double dt)
{
	// Animate.
	this->animation.tick(dt);
	this->textureID = this->animation.getCurrentID();
}
