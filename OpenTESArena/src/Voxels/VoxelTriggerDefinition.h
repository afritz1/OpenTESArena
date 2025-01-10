#ifndef VOXEL_TRIGGER_DEFINITION_H
#define VOXEL_TRIGGER_DEFINITION_H

#include <string>

#include "VoxelUtils.h"

struct VoxelTriggerSoundDefinition
{
	std::string filename;

	void init(const std::string &filename);
};

struct VoxelTriggerTextDefinition
{
	std::string text;
	bool isDisplayedOnce;

	void init(const std::string &text, bool isDisplayedOnce);
};

// Can have a sound and/or text definition.
struct VoxelTriggerDefinition
{
	SNInt x;
	int y;
	WEInt z;
	VoxelTriggerSoundDefinition sound;
	VoxelTriggerTextDefinition text;

	VoxelTriggerDefinition();

	void init(SNInt x, int y, WEInt z);

	bool hasSoundDef() const;
	bool hasTextDef() const;
	bool hasValidDef() const;
};

#endif
