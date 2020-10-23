#include <cstdio>

#include "TriggerDefinition.h"

#include "components/debug/Debug.h"

void TriggerDefinition::SoundDef::init(std::string &&filename)
{
	this->filename = std::move(filename);
}

const std::string &TriggerDefinition::SoundDef::getFilename() const
{
	return this->filename;
}

void TriggerDefinition::TextDef::init(std::string &&text, bool displayedOnce)
{
	this->text = std::move(text);
	this->displayedOnce = displayedOnce;
}

const std::string &TriggerDefinition::TextDef::getText() const
{
	return this->text;
}

bool TriggerDefinition::TextDef::isDisplayedOnce() const
{
	return this->displayedOnce;
}

TriggerDefinition::TriggerDefinition()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
}

void TriggerDefinition::init(SNInt x, int y, WEInt z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

SNInt TriggerDefinition::getX() const
{
	return this->x;
}

int TriggerDefinition::getY() const
{
	return this->y;
}

WEInt TriggerDefinition::getZ() const
{
	return this->z;
}

const TriggerDefinition::SoundDef &TriggerDefinition::getSoundDef() const
{
	return this->sound;
}

const TriggerDefinition::TextDef &TriggerDefinition::getTextDef() const
{
	return this->text;
}

void TriggerDefinition::setSoundDef(std::string &&filename)
{
	this->sound.init(std::move(filename));
}

void TriggerDefinition::setTextDef(std::string &&text, bool displayedOnce)
{
	this->text.init(std::move(text), displayedOnce);
}
