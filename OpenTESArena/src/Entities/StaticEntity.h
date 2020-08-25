#ifndef STATIC_ENTITY_H
#define STATIC_ENTITY_H

#include "Entity.h"
#include "StaticEntityType.h"

// An entity that stays in place and may or may not have an animation.

class StaticEntity final : public Entity
{
private:
	StaticEntityType derivedType;
public:
	StaticEntity();
	virtual ~StaticEntity() = default;

	void initNPC(EntityDefID defID, const EntityAnimationInstance &animInst);
	void initDoodad(EntityDefID defID, const EntityAnimationInstance &animInst);
	void initContainer(EntityDefID defID, const EntityAnimationInstance &animInst);
	void initTransition(EntityDefID defID, const EntityAnimationInstance &animInst);

	EntityType getEntityType() const override;
	StaticEntityType getDerivedType() const;

	virtual void reset() override;
};

#endif
