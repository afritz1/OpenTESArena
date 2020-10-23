#ifndef LOCK_DEFINITION_H
#define LOCK_DEFINITION_H

#include "VoxelUtils.h"

// Supports both locks that can be picked, and locks that require a key.

class LockDefinition
{
public:
	enum class Type { LeveledLock, KeyLock };

	class LeveledLockDef
	{
	private:
		int lockLevel;
	public:
		void init(int lockLevel);
	};

	class KeyLockDef
	{
	private:
		// @todo: key reference somewhere
	public:
		void init();
	};
private:
	SNInt x;
	int y;
	WEInt z;
	Type type;

	union
	{
		LeveledLockDef leveledLock;
		KeyLockDef keyLock;
	};

	void init(SNInt x, int y, WEInt z, Type type);
public:
	static LockDefinition makeLeveledLock(SNInt x, int y, WEInt z, int lockLevel);
	static LockDefinition makeKeyLock(SNInt x, int y, WEInt z);

	SNInt getX() const;
	int getY() const;
	WEInt getZ() const;
	Type getType() const;
	const LeveledLockDef &getLeveledLockDef() const;
	const KeyLockDef &getKeyLockDef() const;
};

#endif
