#ifndef LOCK_DEFINITION_H
#define LOCK_DEFINITION_H

#include "../Voxels/VoxelUtils.h"

enum class LockDefinitionType
{
	LeveledLock,
	KeyLock
};

struct LeveledLockDefinition
{
	int lockLevel;

	void init(int lockLevel);
};

struct KeyLockDefinition
{
	// @todo: key reference somewhere

	void init();
};

// Supports both locks that can be picked, and locks that require a key.
struct LockDefinition
{
	SNInt x;
	int y;
	WEInt z;
	LockDefinitionType type;

	union
	{
		LeveledLockDefinition leveledLock;
		KeyLockDefinition keyLock;
	};

	void initLeveledLock(SNInt x, int y, WEInt z, int lockLevel);
	void initKeyLock(SNInt x, int y, WEInt z);
};

#endif
