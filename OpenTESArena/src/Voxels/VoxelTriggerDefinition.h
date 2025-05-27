#ifndef VOXEL_TRIGGER_DEFINITION_H
#define VOXEL_TRIGGER_DEFINITION_H

#include <string>

#include "VoxelUtils.h"

struct VoxelTriggerSoundDefinition
{
	std::string filename;

	void init(const std::string &filename);
};

struct VoxelTriggerLoreTextDefinition
{
	std::string text;
	bool isDisplayedOnce;

	VoxelTriggerLoreTextDefinition();

	void init(const std::string &text, bool isDisplayedOnce);
};

struct VoxelTriggerKeyDefinition
{
	int keyID; // For texture lookup

	VoxelTriggerKeyDefinition();

	void init(int keyID);
};

// @todo: use all these separately actually and just keep XYZ in each of them since keys aren't a physics zone like the other two

// Can have a sound and/or text definition.
struct VoxelTriggerDefinition
{
	SNInt x;
	int y;
	WEInt z;
	VoxelTriggerSoundDefinition sound;
	VoxelTriggerLoreTextDefinition loreText;
	VoxelTriggerKeyDefinition key;
	// @todo riddle def

	VoxelTriggerDefinition();

	void init(SNInt x, int y, WEInt z);

	bool hasSoundDef() const;
	bool hasLoreTextDef() const;
	bool hasKeyDef() const;
	// @todo hasRiddleDef()
	bool hasValidDefForPhysics() const;
};

#endif
