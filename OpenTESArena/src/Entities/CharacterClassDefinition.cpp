#include <algorithm>
#include <cmath>

#include "CharacterClassDefinition.h"

#include "components/debug/Debug.h"

CharacterClassDefinition::CharacterClassDefinition()
{
	this->castsMagic = false;
	this->healthDie = -1;
	this->initialExpCap = -1;
	this->lockpickPercent = 0.0;
	this->criticalHit = false;
}

void CharacterClassDefinition::init(std::string &&name, CategoryID categoryID,
	std::string &&preferredAttributes, const int *allowedArmors, int allowedArmorCount,
	const int *allowedShields, int allowedShieldCount, const int *allowedWeapons,
	int allowedWeaponCount, bool castsMagic, int healthDie, int initialExpCap,
	double lockpickPercent, bool criticalHit, const std::optional<int> &originalClassIndex)
{
	this->name = std::move(name);
	this->categoryID = categoryID;
	this->preferredAttributes = std::move(preferredAttributes);
	
	this->allowedArmors.resize(allowedArmorCount);
	this->allowedShields.resize(allowedShieldCount);
	this->allowedWeapons.resize(allowedWeaponCount);
	std::copy(allowedArmors, allowedArmors + allowedArmorCount, this->allowedArmors.begin());
	std::copy(allowedShields, allowedShields + allowedShieldCount, this->allowedShields.begin());
	std::copy(allowedWeapons, allowedWeapons + allowedWeaponCount, this->allowedWeapons.begin());

	this->castsMagic = castsMagic;
	this->healthDie = healthDie;
	this->initialExpCap = initialExpCap;
	this->lockpickPercent = lockpickPercent;
	this->criticalHit = criticalHit;
	this->originalClassIndex = originalClassIndex;
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

const std::string &CharacterClassDefinition::getName() const
{
	return this->name;
}

CharacterClassDefinition::CategoryID CharacterClassDefinition::getCategoryID() const
{
	return this->categoryID;
}

const std::string &CharacterClassDefinition::getPreferredAttributes() const
{
	return this->preferredAttributes;
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

bool CharacterClassDefinition::canCastMagic() const
{
	return this->castsMagic;
}

int CharacterClassDefinition::getHealthDie() const
{
	return this->healthDie;
}

int CharacterClassDefinition::getInitialExperienceCap() const
{
	return this->initialExpCap;
}

double CharacterClassDefinition::getLockpickPercent() const
{
	return this->lockpickPercent;
}

bool CharacterClassDefinition::hasCriticalHit() const
{
	return this->criticalHit;
}

const std::optional<int> &CharacterClassDefinition::getOriginalClassIndex() const
{
	return this->originalClassIndex;
}
