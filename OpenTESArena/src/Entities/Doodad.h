#ifndef DOODAD_H
#define DOODAD_H

#include "Animation.h"
#include "Entity.h"

// A doodad is an object, usually decorative, like furniture or the boiling pot
// in the blacksmith's shop. They may or may not have a looping animation.

// I haven't decided if I want lights, like torches and street lamps, to be doodads as well.

class Doodad : public Entity
{
private:
	Animation animation;
	Double3 position;
public:
	Doodad(const Animation &animation, const Double3 &position,
		EntityManager &entityManager);
	virtual ~Doodad();

	virtual std::unique_ptr<Entity> clone(EntityManager &entityManager) const override;

	virtual EntityType getEntityType() const override;
	virtual const Double3 &getPosition() const override;
	virtual bool facesPlayer() const override;

	virtual void tick(Game &game, double dt) override;
};

#endif
