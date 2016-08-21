#include <cassert>
#include <map>
#include <sstream>

#include "CharacterClassParser.h"

#include "CharacterClass.h"
#include "CharacterClassCategoryName.h"
#include "../Items/ArmorMaterialType.h"
#include "../Items/ShieldType.h"
#include "../Items/WeaponType.h"
#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/String.h"

const std::map<std::string, CharacterClassCategoryName> CharacterClassParserCategories =
{
	{ "Mage", CharacterClassCategoryName::Mage },
	{ "Thief", CharacterClassCategoryName::Thief },
	{ "Warrior", CharacterClassCategoryName::Warrior }
};

const std::map<std::string, bool> CharacterClassParserMagicBooleans =
{
	{ "True", true },
	{ "False", false }
};

const std::map<std::string, ArmorMaterialType> CharacterClassParserArmors =
{
	{ "Leather", ArmorMaterialType::Leather },
	{ "Chain", ArmorMaterialType::Chain },
	{ "Plate", ArmorMaterialType::Plate }
};

const std::map<std::string, ShieldType> CharacterClassParserShields =
{
	{ "Buckler", ShieldType::Buckler },
	{ "Round", ShieldType::Round },
	{ "Kite", ShieldType::Kite },
	{ "Tower", ShieldType::Tower }
};

const std::map<std::string, WeaponType> CharacterClassParserWeapons =
{
	{ "BattleAxe", WeaponType::BattleAxe },
	{ "Broadsword", WeaponType::Broadsword },
	{ "Claymore", WeaponType::Claymore },
	{ "Dagger", WeaponType::Dagger },
	{ "DaiKatana", WeaponType::DaiKatana },
	{ "Flail", WeaponType::Flail },
	{ "Katana", WeaponType::Katana },
	{ "LongBow", WeaponType::LongBow },
	{ "Longsword", WeaponType::Longsword },
	{ "Mace", WeaponType::Mace },
	{ "Saber", WeaponType::Saber },
	{ "ShortBow", WeaponType::ShortBow },
	{ "Shortsword", WeaponType::Shortsword },
	{ "Staff", WeaponType::Staff },
	{ "Tanto", WeaponType::Tanto },
	{ "Wakizashi", WeaponType::Wakizashi },
	{ "WarAxe", WeaponType::WarAxe },
	{ "Warhammer", WeaponType::Warhammer }
};

// These paths might be obsolete soon.
const std::string CharacterClassParser::PATH = "data/text/";
const std::string CharacterClassParser::FILENAME = "classes.txt";

std::vector<std::unique_ptr<CharacterClass>> CharacterClassParser::parse()
{
	// This parser is very simple right now. All text must have the exact amount
	// of spacing and commas, and there must be a new line at the end of the file.
	// Comment lines must have the comment symbol in the first column.

	std::string fullPath(CharacterClassParser::PATH + CharacterClassParser::FILENAME);

	// Read the locations file into a string.
	std::string text = File::toString(fullPath);

	// Relevant parsing symbols.
	const char comment = '#';
	const char comma = ',';
	const std::string any = "Any";
	const std::string none = "None";

	std::vector<std::unique_ptr<CharacterClass>> classes;
	std::istringstream iss(text);
	std::string line;

	// For each line, get the substrings between commas.
	while (std::getline(iss, line))
	{
		const char &firstColumn = line.at(0);

		// Ignore comments and blank lines.
		if ((firstColumn == comment) ||
			(firstColumn == '\r') ||
			(firstColumn == '\n'))
		{
			continue;
		}

		// Get the display name.
		int index = 0;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string displayName = line.substr(0, index);

		// Get the category name.
		index += 2;
		int oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string category = line.substr(oldIndex, index - oldIndex);

		// Get the magic boolean.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string magicBoolean = line.substr(oldIndex, index - oldIndex);

		// Get the starting health.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string health = line.substr(oldIndex, index - oldIndex);

		// Get the health dice.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string dice = line.substr(oldIndex, index - oldIndex);

		// Get the set of armors.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string armors = line.substr(oldIndex, index - oldIndex);
		std::vector<std::string> armorTokens = String::split(armors);

		// Get the set of shields.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		std::string shields = line.substr(oldIndex, index - oldIndex);
		std::vector<std::string> shieldTokens = String::split(shields);

		// Get the set of weapons (read until the end of the line).
		index += 2;
		oldIndex = index;
		while ((line.at(index) != '\r') && (line.at(index) != '\n'))
		{
			++index;
		}

		std::string weapons = line.substr(oldIndex, index - oldIndex);
		std::vector<std::string> weaponTokens = String::split(weapons);

		// Verify that the strings each have a mapping.
		Debug::check(CharacterClassParserCategories.find(category) !=
			CharacterClassParserCategories.end(), "Character Class Parser",
			"Invalid class category \"" + category + "\".");
		Debug::check(CharacterClassParserMagicBooleans.find(magicBoolean) !=
			CharacterClassParserMagicBooleans.end(), "Character Class Parser",
			"Invalid magic boolean \"" + magicBoolean + "\".");

		for (const auto &armor : armorTokens)
		{
			bool condition = (armor == none) || (armor == any) ||
				(CharacterClassParserArmors.find(armor) != CharacterClassParserArmors.end());
			Debug::check(condition, "Character Class Parser",
				"Invalid armor \"" + armor + "\".");
		}

		for (const auto &shield : shieldTokens)
		{
			bool condition = (shield == none) || (shield == any) ||
				(CharacterClassParserShields.find(shield) != CharacterClassParserShields.end());
			Debug::check(condition, "Character Class Parser",
				"Invalid shield \"" + shield + "\".");
		}

		for (const auto &weapon : weaponTokens)
		{
			bool condition = (weapon == none) || (weapon == any) ||
				(CharacterClassParserWeapons.find(weapon) != CharacterClassParserWeapons.end());
			Debug::check(condition, "Character Class Parser",
				"Invalid weapon \"" + weapon + "\".");
		}

		// Convert the strings to recognized types.
		auto categoryName = CharacterClassParserCategories.at(category);
		bool castsMagic = CharacterClassParserMagicBooleans.at(magicBoolean);
		int startingHealth = std::stoi(health);
		int healthDice = std::stoi(dice);

		std::vector<ArmorMaterialType> allowedArmors;
		for (const auto &armor : armorTokens)
		{
			if (armor == none)
			{
				// Stop converting armors.
				break;
			}
			else if (armor == any)
			{
				// Add all armors to the allowed armors list and then stop.
				for (const auto &pair : CharacterClassParserArmors)
				{
					allowedArmors.push_back(pair.second);
				}

				break;
			}
			else
			{
				// Add the armor to the allowed armors list.
				allowedArmors.push_back(CharacterClassParserArmors.at(armor));
			}
		}

		std::vector<ShieldType> allowedShields;
		for (const auto &shield : shieldTokens)
		{
			if (shield == none)
			{
				// Stop converting shields.
				break;
			}
			else if (shield == any)
			{
				// Add all shields to the allowed shields list and then stop.
				for (const auto &pair : CharacterClassParserShields)
				{
					allowedShields.push_back(pair.second);
				}

				break;
			}
			else
			{
				// Add the shield to the allowed shields list.
				allowedShields.push_back(CharacterClassParserShields.at(shield));
			}
		}

		std::vector<WeaponType> allowedWeapons;
		for (const auto &weapon : weaponTokens)
		{
			if (weapon == none)
			{
				// Stop converting weapons.
				break;
			}
			else if (weapon == any)
			{
				// Add all weapons to the allowed weapons list and then stop.
				for (const auto &pair : CharacterClassParserWeapons)
				{
					allowedWeapons.push_back(pair.second);
				}

				break;
			}
			else
			{
				// Add the weapon to the allowed weapons list.
				allowedWeapons.push_back(CharacterClassParserWeapons.at(weapon));
			}
		}

		classes.push_back(std::unique_ptr<CharacterClass>(new CharacterClass(
			displayName, categoryName, castsMagic, startingHealth, healthDice,
			allowedArmors, allowedShields, allowedWeapons)));
	}

	return classes;
}
