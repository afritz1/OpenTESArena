#include <numeric>

#include "CharacterClassLibrary.h"
#include "../Assets/ExeData.h"
#include "../Player/CharacterClassGeneration.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

void CharacterClassLibrary::init(const ExeData &exeData)
{
	const Span<const std::string> classNameStrs = exeData.charClasses.classNames;
	const Span<const uint8_t> allowedArmorsValues = exeData.charClasses.allowedArmors;
	const Span<const std::vector<uint8_t>> allowedShieldsLists = exeData.charClasses.allowedShieldsLists;
	const Span<const int> allowedShieldsIndices = exeData.charClasses.allowedShieldsIndices;
	const Span<const std::vector<uint8_t>> allowedWeaponsLists = exeData.charClasses.allowedWeaponsLists;
	const Span<const int> allowedWeaponsIndices = exeData.charClasses.allowedWeaponsIndices;
	const Span<const std::string> preferredAttributesStrs = exeData.charClasses.preferredAttributes;
	const Span<const uint8_t> classNumbersToIDsValues = exeData.charClasses.classNumbersToIDs;
	const Span<const uint16_t> initialExpCapValues = exeData.charClasses.initialExperienceCaps;
	const Span<const uint8_t> healthDiceValues = exeData.charClasses.healthDice;
	const Span<const uint8_t> spellPointMultiplierValues = exeData.charClasses.magicClassIntelligenceMultipliers;
	const Span<const uint8_t> thievingDivisorValues = exeData.charClasses.thievingDivisors;

	constexpr int originalClassCount = 18;

	for (int i = 0; i < originalClassCount; i++)
	{
		DebugAssertIndex(classNameStrs, i);
		std::string name = classNameStrs[i];
		const int category = i / 6;

		DebugAssertIndex(preferredAttributesStrs, i);
		std::string preferredAttributes = preferredAttributesStrs[i];

		const std::vector<ArenaArmorMaterialType> allowedArmors = [&allowedArmorsValues, i]()
		{
			// Determine which armors are allowed based on a one-digit value.
			DebugAssertIndex(allowedArmorsValues, i);
			const uint8_t value = allowedArmorsValues[i];

			if (value == 0)
			{
				return std::vector<ArenaArmorMaterialType> { ArenaArmorMaterialType::Leather, ArenaArmorMaterialType::Chain, ArenaArmorMaterialType::Plate };
			}
			else if (value == 1)
			{
				return std::vector<ArenaArmorMaterialType> { ArenaArmorMaterialType::Leather, ArenaArmorMaterialType::Chain };
			}
			else if (value == 2)
			{
				return std::vector<ArenaArmorMaterialType> { ArenaArmorMaterialType::Leather };
			}
			else if (value == 3)
			{
				return std::vector<ArenaArmorMaterialType>();
			}
			else
			{
				DebugUnhandledReturnMsg(std::vector<ArenaArmorMaterialType>, std::to_string(value));
			}
		}();

		const std::vector<ArenaArmorTypeID> allowedShields = [&allowedShieldsLists, &allowedShieldsIndices, i]()
		{
			// Get the pre-calculated shield index.
			DebugAssertIndex(allowedShieldsIndices, i);
			const int shieldIndex = allowedShieldsIndices[i];
			constexpr int NO_INDEX = -1;

			// If the index is "null" (-1), that means all shields are allowed for this class.
			if (shieldIndex == NO_INDEX)
			{
				return std::vector<ArenaArmorTypeID> { 7, 8, 9, 10 };
			}

			DebugAssertIndex(allowedShieldsLists, shieldIndex);
			const Span<const uint8_t> shieldsList = allowedShieldsLists[shieldIndex];

			std::vector<ArenaArmorTypeID> shields;
			for (const uint8_t armorTypeID : shieldsList)
			{
				shields.emplace_back(armorTypeID);
			}

			return shields;
		}();

		const std::vector<ArenaWeaponTypeID> allowedWeapons = [&allowedWeaponsLists, &allowedWeaponsIndices, i]()
		{
			// Get the pre-calculated weapon index.
			DebugAssertIndex(allowedWeaponsIndices, i);
			const int weaponIndex = allowedWeaponsIndices[i];
			constexpr int NO_INDEX = -1;

			// Weapon IDs as they are shown in the executable (staff, sword, ..., long bow).
			const std::vector<ArenaWeaponTypeID> WeaponIDs = []()
			{
				std::vector<ArenaWeaponTypeID> weapons(18);
				std::iota(weapons.begin(), weapons.end(), 0);
				return weapons;
			}();

			// If the index is "null" (-1), that means all weapons are allowed for this class.
			if (weaponIndex == NO_INDEX)
			{
				return WeaponIDs;
			}

			DebugAssertIndex(allowedWeaponsLists, weaponIndex);
			const Span<const uint8_t> weaponsList = allowedWeaponsLists[weaponIndex];

			std::vector<ArenaWeaponTypeID> weapons;
			for (const uint8_t weaponTypeID : weaponsList)
			{
				weapons.emplace_back(weaponTypeID);
			}

			return weapons;
		}();

		DebugAssertIndex(thievingDivisorValues, i);
		const uint8_t thievingDivisor = thievingDivisorValues[i];

		DebugAssertIndex(healthDiceValues, i);
		const int healthDie = healthDiceValues[i];

		DebugAssertIndex(initialExpCapValues, i);
		const int initialExperienceCap = initialExpCapValues[i];

		DebugAssertIndex(classNumbersToIDsValues, i);
		const int classNumberToID = classNumbersToIDsValues[i];

		const int classIndex = classNumberToID & CharacterClassGeneration::ID_MASK;
		const bool mage = (classNumberToID & CharacterClassGeneration::SPELLCASTER_MASK) != 0;
		const bool thief = (classNumberToID & CharacterClassGeneration::THIEF_MASK) != 0;
		const bool criticalHit = (classNumberToID & CharacterClassGeneration::CRITICAL_HIT_MASK) != 0;

		double spellPointsMultiplier = 0.0;
		if (mage)
		{
			spellPointsMultiplier = 1.0;

			if (classNumberToID != 0xE6)
			{
				double spellPointsMultiplierBonus = 1.0;

				DebugAssertIndex(spellPointMultiplierValues, classIndex);
				const uint8_t spellPointModifier = spellPointMultiplierValues[classIndex];

				if (classNumberToID == 0x23)
				{
					spellPointsMultiplierBonus = 2.0;
				}
				else if (spellPointModifier != 2)
				{
					if (spellPointModifier != 0)
					{
						spellPointsMultiplier += 0.25;
					}

					spellPointsMultiplierBonus = 0.50;
				}

				spellPointsMultiplier += spellPointsMultiplierBonus;
			}
		}

		constexpr const char *categoryNames[] = { "Mage", "Thief", "Warrior" }; // There doesn't appear to be a way to read this from game data.
		DebugAssertIndex(categoryNames, category);
		const char *categoryName = categoryNames[category];

		double climbingSpeedScale = 1.0;
		if (classIndex == 9)
		{
			climbingSpeedScale = 4.0;
		}

		bool canRecoverSpellPoints = mage;
		if (classIndex == 3)
		{
			canRecoverSpellPoints = false;
		}

		int restHealingBonus = 0;
		if (classIndex == 4)
		{
			restHealingBonus = 20;
		}

		CharacterClassDefinition def;
		def.init(name.c_str(), category, categoryName, preferredAttributes.c_str(), allowedArmors, allowedShields, allowedWeapons,
			mage, healthDie, spellPointsMultiplier, initialExperienceCap, thievingDivisor, criticalHit, climbingSpeedScale, canRecoverSpellPoints,
			restHealingBonus, classIndex);

		this->defs.emplace_back(std::move(def));
	}
}

int CharacterClassLibrary::getDefinitionCount() const
{
	return static_cast<int>(this->defs.size());
}

const CharacterClassDefinition &CharacterClassLibrary::getDefinition(int index) const
{
	DebugAssertIndex(this->defs, index);
	return this->defs[index];
}

bool CharacterClassLibrary::findDefinitionIndexIf(const Predicate &predicate, int *outIndex) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		const CharacterClassDefinition &def = this->defs[i];
		if (predicate(def))
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

bool CharacterClassLibrary::tryGetDefinitionIndex(const CharacterClassDefinition &def, int *outIndex) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		const CharacterClassDefinition &charClassDef = this->defs[i];
		if (StringView::equals(charClassDef.name, def.name))
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}
