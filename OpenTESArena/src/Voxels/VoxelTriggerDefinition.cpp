#include <cstdio>

#include "VoxelTriggerDefinition.h"

#include "components/debug/Debug.h"

void VoxelTriggerSoundDefinition::init(const std::string &filename)
{
	this->filename = filename;
}

void VoxelTriggerTextDefinition::init(const std::string &text, bool isDisplayedOnce)
{
	this->text = text;
	this->isDisplayedOnce = isDisplayedOnce;
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

bool VoxelTriggerDefinition::hasSoundDef() const
{
	return !this->sound.filename.empty();
}

bool VoxelTriggerDefinition::hasTextDef() const
{
	return !this->text.text.empty();
}

bool VoxelTriggerDefinition::hasValidDef() const
{
	return this->hasSoundDef() || this->hasTextDef();
}
