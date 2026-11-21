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

bool SpellAnimationInstance::isValid() const
{
	return this->defID >= 0;
}

bool SpellAnimationInstance::isFinished() const
{
	return this->currentSeconds >= SpellAnimationInstance::TOTAL_SECONDS;
}

double SpellAnimationInstance::getProgressPercent() const
{
	return std::clamp(this->currentSeconds / SpellAnimationInstance::TOTAL_SECONDS, 0.0, 1.0);
}

void SpellAnimationInstance::clear()
{
	this->defID = -1;
	this->currentSeconds = 0.0;
}

void SpellAnimationInstance::update(double dt)
{
	this->currentSeconds = std::min(this->currentSeconds + dt, SpellAnimationInstance::TOTAL_SECONDS);
}
