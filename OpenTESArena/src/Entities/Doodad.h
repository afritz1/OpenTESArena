#ifndef DOODAD_H
#define DOODAD_H

#include "Animation.h"
#include "Entity.h"

// A doodad is an object, usually decorative, like furniture or the boiling pot
// in the blacksmith's shop. They may or may not have a looping animation.

class Doodad : public Entity
{
private:
	Animation animation;
public:
	Doodad(const Animation &animation);
	Doodad();
	virtual ~Doodad() = default;

	virtual Double3 getPosition() const override;

	virtual void tick(Game &game, double dt) override;
};

#endif
