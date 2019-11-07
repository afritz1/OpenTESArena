#include "EntityManager.h"
#include "EntityType.h"
#include "StaticEntity.h"

StaticEntity::StaticEntity()
{
	this->derivedType = static_cast<StaticEntityType>(-1);
}

EntityType StaticEntity::getEntityType() const
{
	return EntityType::Static;
}

StaticEntityType StaticEntity::getDerivedType() const
{
	return this->derivedType;
}

void StaticEntity::setDerivedType(StaticEntityType derivedType)
{
	this->derivedType = derivedType;
}

void StaticEntity::reset()
{
	Entity::reset();
	this->derivedType = static_cast<StaticEntityType>(-1);
}
