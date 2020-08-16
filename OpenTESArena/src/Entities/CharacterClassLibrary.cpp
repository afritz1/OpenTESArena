#include <numeric>

#include "CharacterClassLibrary.h"
#include "../Assets/ExeData.h"
#include "../Game/CharacterClassGeneration.h"

#include "components/debug/Debug.h"

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
	const auto &lockpickingDivisorValues = exeData.charClasses.lockpickingDivisors;

	// Classes in original game.
	constexpr int classCount = 18;

	for (int i = 0; i < classCount; i++)
	{
		std::string name = classNameStrs.at(i);
		const int category = i / 6;
		std::string preferredAttributes = preferredAttributesStrs.at(i);

		const std::vector<int> allowedArmors = [&allowedArmorsValues, i]()
		{
			// Determine which armors are allowed based on a one-digit value.
			const uint8_t value = allowedArmorsValues.at(i);

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
			const int shieldIndex = allowedShieldsIndices.at(i);
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
				constexpr std::array<int, 4> ShieldIDMappings = { 0, 1, 2, 3 };

				const std::vector<uint8_t> &shieldsList = allowedShieldsLists.at(shieldIndex);
				std::vector<int> shields;

				for (const uint8_t shield : shieldsList)
				{
					shields.push_back(ShieldIDMappings.at(static_cast<int>(shield) - 7));
				}

				return shields;
			}
		}();

		const std::vector<int> allowedWeapons = [&allowedWeaponsLists, &allowedWeaponsIndices, i]()
		{
			// Get the pre-calculated weapon index.
			const int weaponIndex = allowedWeaponsIndices.at(i);
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
				const std::vector<uint8_t> &weaponsList = allowedWeaponsLists.at(weaponIndex);
				std::vector<int> weapons;

				for (const uint8_t weapon : weaponsList)
				{
					weapons.push_back(WeaponIDs.at(weapon));
				}

				return weapons;
			}
		}();

		const double lockpickPercent = [&lockpickingDivisorValues, i]()
		{
			const uint8_t divisor = lockpickingDivisorValues.at(i);
			return static_cast<double>(200 / divisor) / 100.0;
		}();

		const int healthDie = healthDiceValues.at(i);
		const int initialExperienceCap = initialExpCapValues.at(i);
		const int classNumberToID = classNumbersToIDsValues.at(i);

		const int classIndex = classNumberToID & CharacterClassGeneration::ID_MASK;
		const bool mage = (classNumberToID & CharacterClassGeneration::SPELLCASTER_MASK) != 0;
		const bool thief = (classNumberToID & CharacterClassGeneration::THIEF_MASK) != 0;
		const bool criticalHit = (classNumberToID & CharacterClassGeneration::CRITICAL_HIT_MASK) != 0;

		CharacterClassDefinition def;
		def.init(std::move(name), category, std::move(preferredAttributes),
			allowedArmors.data(), static_cast<int>(allowedArmors.size()),
			allowedShields.data(), static_cast<int>(allowedShields.size()),
			allowedWeapons.data(), static_cast<int>(allowedWeapons.size()),
			mage, healthDie, initialExperienceCap, lockpickPercent, criticalHit, classIndex);

		this->defs.push_back(std::move(def));
	}
}

int CharacterClassLibrary::getDefinitionCount() const
{
	return static_cast<int>(this->defs.size());
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

const CharacterClassDefinition &CharacterClassLibrary::getDefinition(int index) const
{
	DebugAssertIndex(this->defs, index);
	return this->defs[index];
}

bool CharacterClassLibrary::tryGetDefinitionIndex(const CharacterClassDefinition &def, int *outIndex) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		const CharacterClassDefinition &charClassDef = this->defs[i];
		if (charClassDef.getName() == def.getName())
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}
