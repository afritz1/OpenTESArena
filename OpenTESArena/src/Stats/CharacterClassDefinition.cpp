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

void CharacterClassDefinition::init(const char *name, CharacterClassCategoryID categoryID, const char *categoryName, const char *preferredAttributes,
	Span<const ArenaArmorMaterialType> allowedArmors, Span<const ArenaArmorTypeID> allowedShields, Span<const ArenaWeaponTypeID> allowedWeapons,
	bool castsMagic, int healthDie, double spellPointsMultiplier, int initialExpCap, int thievingDivisor,
	bool criticalHit, double climbingSpeedScale, int originalClassIndex)
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

ArenaArmorMaterialType CharacterClassDefinition::getAllowedArmor(int index) const
{
	DebugAssertIndex(this->allowedArmors, index);
	return this->allowedArmors[index];
}

bool CharacterClassDefinition::isArmorMaterialAllowed(ArenaArmorMaterialType materialType) const
{
	return std::find(this->allowedArmors.begin(), this->allowedArmors.end(), materialType) != this->allowedArmors.end();
}

int CharacterClassDefinition::getAllowedShieldCount() const
{
	return static_cast<int>(this->allowedShields.size());
}

ArenaArmorTypeID CharacterClassDefinition::getAllowedShield(int index) const
{
	DebugAssertIndex(this->allowedShields, index);
	return this->allowedShields[index];
}

bool CharacterClassDefinition::isShieldTypeAllowed(ArenaArmorTypeID typeID) const
{
	return std::find(this->allowedShields.begin(), this->allowedShields.end(), typeID) != this->allowedShields.end();
}

int CharacterClassDefinition::getAllowedWeaponCount() const
{
	return static_cast<int>(this->allowedWeapons.size());
}

ArenaWeaponTypeID CharacterClassDefinition::getAllowedWeapon(int index) const
{
	DebugAssertIndex(this->allowedWeapons, index);
	return this->allowedWeapons[index];
}

bool CharacterClassDefinition::isWeaponTypeAllowed(ArenaWeaponTypeID typeID) const
{
	return std::find(this->allowedWeapons.begin(), this->allowedWeapons.end(), typeID) != this->allowedWeapons.end();
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
