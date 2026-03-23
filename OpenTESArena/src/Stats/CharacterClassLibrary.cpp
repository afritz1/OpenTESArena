#include <numeric>

#include "CharacterClassLibrary.h"
#include "../Assets/ExeData.h"
#include "../Player/CharacterClassGeneration.h"

#include "components/debug/Debug.h"
#include "components/utilities/StringView.h"

void CharacterClassLibrary::init(const ExeData &exeData)
{
	const auto &classNameStrs = exeData.charClasses.classNames;
	const auto &allowedArmorsValues = exeData.charClasses.allowedArmors;
	const auto &allowedShieldsLists = exeData.charClasses.allowedShieldsLists;
	const auto &allowedShieldsIndices = exeData.charClasses.allowedShieldsIndices;
	const auto &allowedWeaponsLists = exeData.charClasses.allowedWeaponsLists;
	const auto &allowedWeaponsIndices = exeData.charClasses.allowedWeaponsIndices;
	const auto &preferredAttributesStrs = exeData.charClasses.preferredAttributes;
	const auto &classNumbersToIDsValues = exeData.charClasses.classNumbersToIDs;
	const auto &initialExpCapValues = exeData.charClasses.initialExperienceCaps;
	const auto &healthDiceValues = exeData.charClasses.healthDice;
	const auto &spellPointMultiplierValues = exeData.charClasses.magicClassIntelligenceMultipliers;
	const auto &thievingDivisorValues = exeData.charClasses.thievingDivisors;

	constexpr int originalClassCount = 18;

	for (int i = 0; i < originalClassCount; i++)
	{
		DebugAssertIndex(classNameStrs, i);
		std::string name = classNameStrs[i];
		const int category = i / 6;

		DebugAssertIndex(preferredAttributesStrs, i);
		std::string preferredAttributes = preferredAttributesStrs[i];

		const std::vector<int> allowedArmors = [&allowedArmorsValues, i]()
		{
			// Determine which armors are allowed based on a one-digit value.
			DebugAssertIndex(allowedArmorsValues, i);
			const uint8_t value = allowedArmorsValues[i];

			if (value == 0)
			{
				return std::vector<int> { 0, 1, 2 };
			}
			else if (value == 1)
			{
				return std::vector<int> { 0, 1 };
			}
			else if (value == 2)
			{
				return std::vector<int> { 0 };
			}
			else if (value == 3)
			{
				return std::vector<int>();
			}
			else
			{
				DebugUnhandledReturnMsg(std::vector<int>, std::to_string(value));
			}
		}();

		const std::vector<int> allowedShields = [&allowedShieldsLists, &allowedShieldsIndices, i]()
		{
			// Get the pre-calculated shield index.
			DebugAssertIndex(allowedShieldsIndices, i);
			const int shieldIndex = allowedShieldsIndices[i];
			constexpr int NO_INDEX = -1;

			// If the index is "null" (-1), that means all shields are allowed for this class.
			if (shieldIndex == NO_INDEX)
			{
				return std::vector<int> { 0, 1, 2, 3 };
			}
			else
			{
				// Mappings of shield IDs to shield types. The index in the array is the ID 
				// minus 7 because shields and armors are treated as the same type in Arena,
				// so they're in the same array, but we separate them here because that seems 
				// more object-oriented.
				constexpr int ShieldIDMappings[] = { 0, 1, 2, 3 };

				DebugAssertIndex(allowedShieldsLists, shieldIndex);
				const Span<const uint8_t> shieldsList = allowedShieldsLists[shieldIndex];

				std::vector<int> shields;
				for (const uint8_t shield : shieldsList)
				{
					const int shieldIdMappingsIndex = static_cast<int>(shield) - 7;
					DebugAssertIndex(ShieldIDMappings, shieldIdMappingsIndex);
					shields.emplace_back(ShieldIDMappings[shieldIdMappingsIndex]);
				}

				return shields;
			}
		}();

		const std::vector<int> allowedWeapons = [&allowedWeaponsLists, &allowedWeaponsIndices, i]()
		{
			// Get the pre-calculated weapon index.
			DebugAssertIndex(allowedWeaponsIndices, i);
			const int weaponIndex = allowedWeaponsIndices[i];
			constexpr int NO_INDEX = -1;

			// Weapon IDs as they are shown in the executable (staff, sword, ..., long bow).
			const std::vector<int> WeaponIDs = []()
			{
				std::vector<int> weapons(18);
				std::iota(weapons.begin(), weapons.end(), 0);
				return weapons;
			}();

			// If the index is "null" (-1), that means all weapons are allowed for this class.
			if (weaponIndex == NO_INDEX)
			{
				return WeaponIDs;
			}
			else
			{
				DebugAssertIndex(allowedWeaponsLists, weaponIndex);
				const Span<const uint8_t> weaponsList = allowedWeaponsLists[weaponIndex];
				
				std::vector<int> weapons;
				for (const uint8_t weapon : weaponsList)
				{
					DebugAssertIndex(WeaponIDs, weapon);
					weapons.emplace_back(WeaponIDs[weapon]);
				}

				return weapons;
			}
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

		CharacterClassDefinition def;
		def.init(name.c_str(), category, categoryName, preferredAttributes.c_str(), allowedArmors, allowedShields, allowedWeapons,
			mage, healthDie, spellPointsMultiplier, initialExperienceCap, thievingDivisor, criticalHit, climbingSpeedScale, classIndex);

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
