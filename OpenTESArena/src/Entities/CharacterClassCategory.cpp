#include <unordered_map>

#include "CharacterClassCategory.h"
#include "CharacterClassCategoryName.h"

// @todo: very old, replace with ExeData.
const std::unordered_map<CharacterClassCategoryName, std::string> CharacterClassCategoryDisplayNames =
{
	{ CharacterClassCategoryName::Mage, "Mage" },
	{ CharacterClassCategoryName::Thief, "Thief" },
	{ CharacterClassCategoryName::Warrior, "Warrior" }
};

const std::string &CharacterClassCategory::toString(CharacterClassCategoryName categoryName)
{
	const std::string &displayName = CharacterClassCategoryDisplayNames.at(categoryName);
	return displayName;
}
