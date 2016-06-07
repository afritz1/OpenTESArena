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

const auto CharacterClassParserCategories = std::map<std::string, CharacterClassCategoryName>
{
	{ "Mage", CharacterClassCategoryName::Mage },
	{ "Thief", CharacterClassCategoryName::Thief },
	{ "Warrior", CharacterClassCategoryName::Warrior }
};

const auto CharacterClassParserMagicBooleans = std::map<std::string, bool>
{
	{ "True", true },
	{ "False", false }
};

const auto CharacterClassParserArmors = std::map<std::string, ArmorMaterialType>
{
	{ "Leather", ArmorMaterialType::Leather },
	{ "Chain", ArmorMaterialType::Chain },
	{ "Plate", ArmorMaterialType::Plate }
};

const auto CharacterClassParserShields = std::map<std::string, ShieldType>
{
	{ "Buckler", ShieldType::Buckler },
	{ "Round", ShieldType::Round },
	{ "Kite", ShieldType::Kite },
	{ "Tower", ShieldType::Tower }
};

const auto CharacterClassParserWeapons = std::map<std::string, WeaponType>
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

const std::string CharacterClassParser::PATH = "data/text/";
const std::string CharacterClassParser::FILENAME = "classes.txt";

std::vector<std::unique_ptr<CharacterClass>> CharacterClassParser::parse()
{
	// This parser is very simple right now. All text must have the exact amount
	// of spacing and commas, and there must be a new line at the end of the file.
	// Comment lines must have the comment symbol in the first column.

	auto fullPath = CharacterClassParser::PATH + CharacterClassParser::FILENAME;

	// Read the locations file into a string.
	auto text = File::toString(fullPath);

	// Relevant parsing symbols.
	const char comment = '#';
	const char comma = ',';
	const auto any = std::string("Any");
	const auto none = std::string("None");

	auto classes = std::vector<std::unique_ptr<CharacterClass>>();

	std::istringstream iss;
	iss.str(text);

	auto line = std::string();

	// For each line, get the substrings between commas.
	while (std::getline(iss, line))
	{
		// Ignore comments and blank lines.
		if ((line.at(0) == comment) || (line.at(0) == '\r') || (line.at(0) == '\n'))
		{
			continue;
		}

		// Get the display name.
		int index = 0;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto displayName = line.substr(0, index);


		// Get the category name.
		index += 2;
		int oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto category = line.substr(oldIndex, index - oldIndex);

		// Get the magic boolean.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto magicBoolean = line.substr(oldIndex, index - oldIndex);

		// Get the starting health.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto health = line.substr(oldIndex, index - oldIndex);

		// Get the health dice.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto dice = line.substr(oldIndex, index - oldIndex);

		// Get the set of armors.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto armors = line.substr(oldIndex, index - oldIndex);
		auto armorTokens = String::split(armors);

		// Get the set of shields.
		index += 2;
		oldIndex = index;
		while (line.at(index) != comma)
		{
			++index;
		}

		auto shields = line.substr(oldIndex, index - oldIndex);
		auto shieldTokens = String::split(shields);

		// Get the set of weapons (read until the end of the line).
		index += 2;
		oldIndex = index;
		while ((line.at(index) != '\r') && (line.at(index) != '\n'))
		{
			++index;
		}

		auto weapons = line.substr(oldIndex, index - oldIndex);
		auto weaponTokens = String::split(weapons);

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

		auto allowedArmors = std::vector<ArmorMaterialType>();
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

		auto allowedShields = std::vector<ShieldType>();
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

		auto allowedWeapons = std::vector<WeaponType>();
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
