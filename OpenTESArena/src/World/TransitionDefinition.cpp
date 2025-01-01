#include "TransitionDefinition.h"

#include "components/debug/Debug.h"

void InteriorEntranceTransitionDefinition::init(MapGeneration::InteriorGenInfo &&interiorGenInfo)
{
	this->interiorGenInfo = std::move(interiorGenInfo);
}

void LevelChangeTransitionDefinition::init(bool isLevelUp)
{
	this->isLevelUp = isLevelUp;
}

TransitionDefinition::TransitionDefinition()
{
	this->type = static_cast<TransitionType>(-1);
}

void TransitionDefinition::initCityGate()
{
	this->type = TransitionType::CityGate;
}

void TransitionDefinition::initInteriorEntrance(MapGeneration::InteriorGenInfo &&interiorGenInfo)
{
	this->type = TransitionType::EnterInterior;
	this->interiorEntrance.init(std::move(interiorGenInfo));
}

void TransitionDefinition::initInteriorExit()
{
	this->type = TransitionType::ExitInterior;
}

void TransitionDefinition::initLevelChange(bool isLevelUp)
{
	this->type = TransitionType::LevelChange;
	this->levelChange.init(isLevelUp);
}
