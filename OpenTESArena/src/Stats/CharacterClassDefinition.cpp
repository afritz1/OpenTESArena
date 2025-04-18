#include <algorithm>
#include <cmath>
#include <cstdio>

#include "CharacterClassDefinition.h"

#include "components/debug/Debug.h"

CharacterClassDefinition::CharacterClassDefinition()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
	this->categoryID = -1;
	std::fill(std::begin(this->preferredAttributes), std::end(this->preferredAttributes), '\0');
	this->castsMagic = false;
	this->healthDie = -1;
	this->initialExpCap = -1;
	this->lockpickPercent = 0.0;
	this->criticalHit = false;
	this->originalClassIndex = -1;
}

void CharacterClassDefinition::init(const char *name, CharacterClassCategoryID categoryID, const char *preferredAttributes,
	BufferView<const int> allowedArmors, BufferView<const int> allowedShields, BufferView<const int> allowedWeapons,
	bool castsMagic, int healthDie, int initialExpCap, double lockpickPercent, bool criticalHit, int originalClassIndex)
{
	std::snprintf(this->name, std::size(this->name), "%s", name);
	this->categoryID = categoryID;
	std::snprintf(this->preferredAttributes, std::size(this->preferredAttributes), "%s", preferredAttributes);
	
	this->allowedArmors.resize(allowedArmors.getCount());
	this->allowedShields.resize(allowedShields.getCount());
	this->allowedWeapons.resize(allowedWeapons.getCount());
	std::copy(allowedArmors.begin(), allowedArmors.end(), this->allowedArmors.begin());
	std::copy(allowedShields.begin(), allowedShields.end(), this->allowedShields.begin());
	std::copy(allowedWeapons.begin(), allowedWeapons.end(), this->allowedWeapons.begin());

	this->castsMagic = castsMagic;
	this->healthDie = healthDie;
	this->initialExpCap = initialExpCap;
	this->lockpickPercent = lockpickPercent;
	this->criticalHit = criticalHit;
	this->originalClassIndex = originalClassIndex;
}

int CharacterClassDefinition::getAllowedArmorCount() const
{
	return static_cast<int>(this->allowedArmors.size());
}

int CharacterClassDefinition::getAllowedShieldCount() const
{
	return static_cast<int>(this->allowedShields.size());
}

int CharacterClassDefinition::getAllowedWeaponCount() const
{
	return static_cast<int>(this->allowedWeapons.size());
}

int CharacterClassDefinition::getAllowedArmor(int index) const
{
	DebugAssertIndex(this->allowedArmors, index);
	return this->allowedArmors[index];
}

int CharacterClassDefinition::getAllowedShield(int index) const
{
	DebugAssertIndex(this->allowedShields, index);
	return this->allowedShields[index];
}

int CharacterClassDefinition::getAllowedWeapon(int index) const
{
	DebugAssertIndex(this->allowedWeapons, index);
	return this->allowedWeapons[index];
}

int CharacterClassDefinition::getExperienceCap(int level, int initialExpCap)
{
	DebugAssert(level >= 0);

	if (level == 0)
	{
		return 0;
	}
	else if (level == 1)
	{
		return initialExpCap;
	}
	else
	{
		const int prevExperienceCap = CharacterClassDefinition::getExperienceCap(level - 1, initialExpCap);
		const double multiplier = ((level >= 2) && (level <= 8)) ? (30.0 / 16.0) : 1.50;
		return static_cast<int>(std::floor(static_cast<double>(prevExperienceCap) * multiplier));
	}
}
