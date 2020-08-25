#include "EntityManager.h"
#include "EntityType.h"
#include "StaticEntity.h"

StaticEntity::StaticEntity()
{
	this->derivedType = static_cast<StaticEntityType>(-1);
}

void StaticEntity::initNPC(EntityDefID defID, const EntityAnimationInstance &animInst)
{
	this->init(defID, animInst);
	this->derivedType = StaticEntityType::NPC;
}

void StaticEntity::initDoodad(EntityDefID defID, const EntityAnimationInstance &animInst)
{
	this->init(defID, animInst);
	this->derivedType = StaticEntityType::Doodad;
}

void StaticEntity::initContainer(EntityDefID defID, const EntityAnimationInstance &animInst)
{
	this->init(defID, animInst);
	this->derivedType = StaticEntityType::Container;
}

void StaticEntity::initTransition(EntityDefID defID, const EntityAnimationInstance &animInst)
{
	this->init(defID, animInst);
	this->derivedType = StaticEntityType::Transition;
}

EntityType StaticEntity::getEntityType() const
{
	return EntityType::Static;
}

StaticEntityType StaticEntity::getDerivedType() const
{
	return this->derivedType;
}

void StaticEntity::reset()
{
	Entity::reset();
	this->derivedType = static_cast<StaticEntityType>(-1);
}
