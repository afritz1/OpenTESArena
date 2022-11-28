#include <cstdio>

#include "VoxelTriggerDefinition.h"

#include "components/debug/Debug.h"

void VoxelTriggerDefinition::SoundDef::init(std::string &&filename)
{
	this->filename = std::move(filename);
}

const std::string &VoxelTriggerDefinition::SoundDef::getFilename() const
{
	return this->filename;
}

void VoxelTriggerDefinition::TextDef::init(std::string &&text, bool displayedOnce)
{
	this->text = std::move(text);
	this->displayedOnce = displayedOnce;
}

const std::string &VoxelTriggerDefinition::TextDef::getText() const
{
	return this->text;
}

bool VoxelTriggerDefinition::TextDef::isDisplayedOnce() const
{
	return this->displayedOnce;
}

VoxelTriggerDefinition::VoxelTriggerDefinition()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
}

void VoxelTriggerDefinition::init(SNInt x, int y, WEInt z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

SNInt VoxelTriggerDefinition::getX() const
{
	return this->x;
}

int VoxelTriggerDefinition::getY() const
{
	return this->y;
}

WEInt VoxelTriggerDefinition::getZ() const
{
	return this->z;
}

bool VoxelTriggerDefinition::hasSoundDef() const
{
	return this->sound.getFilename().size() > 0;
}

bool VoxelTriggerDefinition::hasTextDef() const
{
	return this->text.getText().size() > 0;
}

const VoxelTriggerDefinition::SoundDef &VoxelTriggerDefinition::getSoundDef() const
{
	return this->sound;
}

const VoxelTriggerDefinition::TextDef &VoxelTriggerDefinition::getTextDef() const
{
	return this->text;
}

void VoxelTriggerDefinition::setSoundDef(std::string &&filename)
{
	this->sound.init(std::move(filename));
}

void VoxelTriggerDefinition::setTextDef(std::string &&text, bool displayedOnce)
{
	this->text.init(std::move(text), displayedOnce);
}
