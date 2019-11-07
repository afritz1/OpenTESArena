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

	EntityType getEntityType() const override;
	StaticEntityType getDerivedType() const;

	void setDerivedType(StaticEntityType derivedType);

	virtual void reset() override;
};

#endif
