#include <cstdio>

#include "TriggerDefinition.h"

#include "components/debug/Debug.h"

void TriggerDefinition::SoundDef::init(const char *name)
{
	std::snprintf(this->name, std::size(this->name), "%s", name);
}

std::string_view TriggerDefinition::SoundDef::getName() const
{
	return this->name;
}

void TriggerDefinition::TextDef::init(int textID, bool displayedOnce)
{
	this->textID = textID;
	this->displayedOnce = displayedOnce;
}

int TriggerDefinition::TextDef::getTextID() const
{
	return this->textID;
}

bool TriggerDefinition::TextDef::isDisplayedOnce() const
{
	return this->displayedOnce;
}

void TriggerDefinition::init(WEInt x, int y, SNInt z, Type type)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = type;
}

TriggerDefinition TriggerDefinition::makeSound(WEInt x, int y, SNInt z, const char *name)
{
	TriggerDefinition triggerDef;
	triggerDef.init(x, y, z, Type::Sound);
	triggerDef.sound.init(name);
	return triggerDef;
}

TriggerDefinition TriggerDefinition::makeText(WEInt x, int y, SNInt z, int textID,
	bool displayedOnce)
{
	TriggerDefinition triggerDef;
	triggerDef.init(x, y, z, Type::Text);
	triggerDef.text.init(textID, displayedOnce);
	return triggerDef;
}

WEInt TriggerDefinition::getX() const
{
	return this->x;
}

int TriggerDefinition::getY() const
{
	return this->y;
}

SNInt TriggerDefinition::getZ() const
{
	return this->z;
}

TriggerDefinition::Type TriggerDefinition::getType() const
{
	return this->type;
}

const TriggerDefinition::SoundDef &TriggerDefinition::getSoundDef() const
{
	DebugAssert(this->type == Type::Sound);
	return this->sound;
}

const TriggerDefinition::TextDef &TriggerDefinition::getTextDef() const
{
	DebugAssert(this->type == Type::Text);
	return this->text;
}
