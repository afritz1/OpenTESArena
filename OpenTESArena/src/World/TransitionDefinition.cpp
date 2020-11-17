#include "TransitionDefinition.h"

#include "components/debug/Debug.h"

void TransitionDefinition::LevelChangeDef::init(bool isLevelUp)
{
	this->isLevelUp = isLevelUp;
}

TransitionDefinition::TransitionDefinition()
{
	this->type = static_cast<TransitionDefinition::Type>(-1);
}

void TransitionDefinition::init(Type type)
{
	this->type = type;
}

void TransitionDefinition::initCityGate()
{
	this->init(Type::CityGate);
}

void TransitionDefinition::initInteriorEntrance()
{
	this->init(Type::EnterInterior);
}

void TransitionDefinition::initInteriorExit()
{
	this->init(Type::ExitInterior);
}

void TransitionDefinition::initLevelChange(bool isLevelUp)
{
	this->init(Type::LevelChange);
	this->levelChange.init(isLevelUp);
}

TransitionDefinition::Type TransitionDefinition::getType() const
{
	return this->type;
}

const TransitionDefinition::LevelChangeDef &TransitionDefinition::getLevelChange() const
{
	DebugAssert(this->type == Type::LevelChange);
	return this->levelChange;
}
