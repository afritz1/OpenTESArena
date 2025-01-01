#include "LockDefinition.h"

#include "components/debug/Debug.h"

void LeveledLockDefinition::init(int lockLevel)
{
	this->lockLevel = lockLevel;
}

void KeyLockDefinition::init()
{
	// Do nothing.
}

void LockDefinition::initLeveledLock(SNInt x, int y, WEInt z, int lockLevel)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = LockDefinitionType::LeveledLock;
	this->leveledLock.init(lockLevel);
}

void LockDefinition::initKeyLock(SNInt x, int y, WEInt z)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->type = LockDefinitionType::KeyLock;
	this->keyLock.init();
}
