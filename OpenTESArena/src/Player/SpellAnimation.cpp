#include <algorithm>

#include "SpellAnimation.h"

SpellAnimationDefinition::SpellAnimationDefinition()
{
	this->type = static_cast<SpellAnimationType>(-1);
	this->width = 0;
	this->height = 0;
}

SpellAnimationInstance::SpellAnimationInstance()
{
	this->defID = -1;
	this->currentSeconds = 0.0;
}

void SpellAnimationInstance::init(SpellAnimationDefinitionID defID, double currentSeconds)
{
	this->defID = defID;
	this->currentSeconds = currentSeconds;
}

bool SpellAnimationInstance::isFinished() const
{
	return this->currentSeconds >= SpellAnimationInstance::TARGET_SECONDS;
}

void SpellAnimationInstance::update(double dt)
{
	this->currentSeconds = std::min(this->currentSeconds + dt, SpellAnimationInstance::TARGET_SECONDS);
}
