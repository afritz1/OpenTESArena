#include "LockDefinition.h"

#include "components/debug/Debug.h"

void LockDefinition::LockDef::init(int lockLevel)
{
	this->lockLevel = lockLevel;
}

void LockDefinition::KeyLockDef::init()
{
	// Do nothing.
}

void LockDefinition::init(WEInt x, int y, SNInt z, Type type)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = type;
}

LockDefinition LockDefinition::makeLock(WEInt x, int y, SNInt z, int lockLevel)
{
	LockDefinition lockDef;
	lockDef.init(x, y, z, Type::Lock);
	lockDef.lock.init(lockLevel);
	return lockDef;
}

LockDefinition LockDefinition::makeKeyLock(WEInt x, int y, SNInt z)
{
	LockDefinition lockDef;
	lockDef.init(x, y, z, Type::KeyLock);
	lockDef.keyLock.init();
	return lockDef;
}

WEInt LockDefinition::getX() const
{
	return this->x;
}

int LockDefinition::getY() const
{
	return this->y;
}

SNInt LockDefinition::getZ() const
{
	return this->z;
}

LockDefinition::Type LockDefinition::getType() const
{
	return this->type;
}

const LockDefinition::LockDef &LockDefinition::getLockDef() const
{
	DebugAssert(this->type == Type::Lock);
	return this->lock;
}

const LockDefinition::KeyLockDef &LockDefinition::getKeyLockDef() const
{
	DebugAssert(this->type == Type::KeyLock);
	return this->keyLock;
}
