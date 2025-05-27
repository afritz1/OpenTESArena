#include "LockDefinition.h"

#include "components/debug/Debug.h"

LockDefinition::LockDefinition()
{
	this->x = 0;
	this->y = 0;
	this->z = 0;
	this->lockLevel = -1;
	this->keyID = -1;
}

void LockDefinition::init(SNInt x, int y, WEInt z, int lockLevel)
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->lockLevel = lockLevel;
	this->keyID = (lockLevel - 1) % 12;
}
