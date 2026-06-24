#include <algorithm>
#include <cmath>
#include <cstdio>

#include "CharacterClassDefinition.h"

#include "components/debug/Debug.h"

namespace
{
	double GetExperienceMultiplierForLevel(int level)
	{
		constexpr double lowLevelMultiplier = 30.0 / 16.0;
		constexpr double highLevelMultiplier = 1.50;
		if ((level >= 2) && (level <= 8))
		{
			return lowLevelMultiplier;
		}
		else
		{
			return highLevelMultiplier;
		}
	}
}

CharacterClassDefinition::CharacterClassDefinition()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
	this->categoryID = -1;
	std::fill(std::begin(this->categoryName), std::end(this->categoryName), '\0');
	std::fill(std::begin(this->preferredAttributes), std::end(this->preferredAttributes), '\0');
	this->castsMagic = false;
	this->healthDie = -1;
	this->spellPointsMultiplier = 0.0;
	this->initialExpCap = -1;
	this->thievingDivisor = 0;
	this->criticalHit = false;
	this->climbingSpeedScale = 0.0;
	this->originalClassIndex = -1;
}

void CharacterClassDefinition::init(const char *name, CharacterClassCategoryID categoryID, const char *categoryName,
	const char *preferredAttributes, Span<const int> allowedArmors, Span<const int> allowedShields, Span<const int> allowedWeapons,
	bool castsMagic, int healthDie, double spellPointsMultiplier, int initialExpCap, int thievingDivisor, bool criticalHit,
	double climbingSpeedScale, int originalClassIndex)
{
	std::snprintf(this->name, std::size(this->name), "%s", name);
	this->categoryID = categoryID;
	std::snprintf(this->categoryName, std::size(this->categoryName), "%s", categoryName);
	std::snprintf(this->preferredAttributes, std::size(this->preferredAttributes), "%s", preferredAttributes);
	
	this->allowedArmors.resize(allowedArmors.getCount());
	this->allowedShields.resize(allowedShields.getCount());
	this->allowedWeapons.resize(allowedWeapons.getCount());
	std::copy(allowedArmors.begin(), allowedArmors.end(), this->allowedArmors.begin());
	std::copy(allowedShields.begin(), allowedShields.end(), this->allowedShields.begin());
	std::copy(allowedWeapons.begin(), allowedWeapons.end(), this->allowedWeapons.begin());

	this->castsMagic = castsMagic;
	this->healthDie = healthDie;
	this->spellPointsMultiplier = spellPointsMultiplier;
	this->initialExpCap = initialExpCap;
	this->thievingDivisor = thievingDivisor;
	this->criticalHit = criticalHit;
	this->climbingSpeedScale = climbingSpeedScale;
	this->originalClassIndex = originalClassIndex;
}

int CharacterClassDefinition::getAllowedArmorCount() const
{
	return static_cast<int>(this->allowedArmors.size());
}

int CharacterClassDefinition::getAllowedArmor(int index) const
{
	DebugAssertIndex(this->allowedArmors, index);
	return this->allowedArmors[index];
}

bool CharacterClassDefinition::isArmorAllowed(ArmorMaterialType materialType) const
{
	const int armorID = static_cast<int>(materialType);
	return std::find(this->allowedArmors.begin(), this->allowedArmors.end(), armorID) != this->allowedArmors.end();
}

int CharacterClassDefinition::getAllowedShieldCount() const
{
	return static_cast<int>(this->allowedShields.size());
}

int CharacterClassDefinition::getAllowedShield(int index) const
{
	DebugAssertIndex(this->allowedShields, index);
	return this->allowedShields[index];
}

bool CharacterClassDefinition::isShieldAllowed(int shieldID) const
{
	return std::find(this->allowedShields.begin(), this->allowedShields.end(), shieldID) != this->allowedShields.end();
}

int CharacterClassDefinition::getAllowedWeaponCount() const
{
	return static_cast<int>(this->allowedWeapons.size());
}

int CharacterClassDefinition::getAllowedWeapon(int index) const
{
	DebugAssertIndex(this->allowedWeapons, index);
	return this->allowedWeapons[index];
}

bool CharacterClassDefinition::isWeaponAllowed(int weaponID) const
{
	return std::find(this->allowedWeapons.begin(), this->allowedWeapons.end(), weaponID) != this->allowedWeapons.end();
}

int CharacterClassDefinition::getExperienceCap(int level) const
{
	if (level <= 0)
	{
		return 0;
	}

	int experienceCap = this->initialExpCap;
	for (int currentLevel = 2; currentLevel <= level; currentLevel++)
	{
		const double multiplier = GetExperienceMultiplierForLevel(currentLevel);
		experienceCap = static_cast<int>(std::floor(static_cast<double>(experienceCap) * multiplier));
	}

	return experienceCap;
}
