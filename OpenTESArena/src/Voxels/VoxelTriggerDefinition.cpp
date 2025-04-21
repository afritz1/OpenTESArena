#include <cstdio>

#include "VoxelTriggerDefinition.h"

#include "components/debug/Debug.h"

void VoxelTriggerSoundDefinition::init(const std::string &filename)
{
	this->filename = filename;
}

VoxelTriggerLoreTextDefinition::VoxelTriggerLoreTextDefinition()
{
	this->isDisplayedOnce = false;
}

void VoxelTriggerLoreTextDefinition::init(const std::string &text, bool isDisplayedOnce)
{
	this->text = text;
	this->isDisplayedOnce = isDisplayedOnce;
}

VoxelTriggerKeyDefinition::VoxelTriggerKeyDefinition()
{
	this->keyID = -1;
}

void VoxelTriggerKeyDefinition::init(int keyID)
{
	this->keyID = keyID;
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

bool VoxelTriggerDefinition::hasLoreTextDef() const
{
	return !this->loreText.text.empty();
}

bool VoxelTriggerDefinition::hasKeyDef() const
{
	return this->key.keyID >= 0;
}

bool VoxelTriggerDefinition::hasValidDefForPhysics() const
{
	return this->hasSoundDef() || this->hasLoreTextDef();
}
