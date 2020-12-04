#include "TransitionDefinition.h"

#include "components/debug/Debug.h"

void TransitionDefinition::InteriorEntranceDef::init(MapGeneration::InteriorGenInfo &&interiorGenInfo)
{
	this->interiorGenInfo = std::move(interiorGenInfo);
}

void TransitionDefinition::LevelChangeDef::init(bool isLevelUp)
{
	this->isLevelUp = isLevelUp;
}

TransitionDefinition::TransitionDefinition()
{
	this->type = static_cast<TransitionType>(-1);
}

void TransitionDefinition::init(TransitionType type)
{
	this->type = type;
}

void TransitionDefinition::initCityGate()
{
	this->init(TransitionType::CityGate);
}

void TransitionDefinition::initInteriorEntrance(MapGeneration::InteriorGenInfo &&interiorGenInfo)
{
	this->init(TransitionType::EnterInterior);
	this->interiorEntrance.init(std::move(interiorGenInfo));
}

void TransitionDefinition::initInteriorExit()
{
	this->init(TransitionType::ExitInterior);
}

void TransitionDefinition::initLevelChange(bool isLevelUp)
{
	this->init(TransitionType::LevelChange);
	this->levelChange.init(isLevelUp);
}

TransitionType TransitionDefinition::getType() const
{
	return this->type;
}

const TransitionDefinition::InteriorEntranceDef &TransitionDefinition::getInteriorEntrance() const
{
	DebugAssert(this->type == TransitionType::EnterInterior);
	return this->interiorEntrance;
}

const TransitionDefinition::LevelChangeDef &TransitionDefinition::getLevelChange() const
{
	DebugAssert(this->type == TransitionType::LevelChange);
	return this->levelChange;
}
