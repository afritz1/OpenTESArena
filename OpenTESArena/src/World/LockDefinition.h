#ifndef LOCK_DEFINITION_H
#define LOCK_DEFINITION_H

#include "VoxelUtils.h"

// Supports both locks that can be picked, and locks that require a key.

class LockDefinition
{
public:
	enum class Type { Lock, KeyLock };

	class LockDef
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
	WEInt x;
	int y;
	SNInt z;
	Type type;

	union
	{
		LockDef lock;
		KeyLockDef keyLock;
	};

	void init(WEInt x, int y, SNInt z, Type type);
public:
	static LockDefinition makeLock(WEInt x, int y, SNInt z, int lockLevel);
	static LockDefinition makeKeyLock(WEInt x, int y, SNInt z);

	WEInt getX() const;
	int getY() const;
	SNInt getZ() const;
	Type getType() const;
	const LockDef &getLockDef() const;
	const KeyLockDef &getKeyLockDef() const;
};

#endif
