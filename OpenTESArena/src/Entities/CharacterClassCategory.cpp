#include <unordered_map>

#include "CharacterClassCategory.h"
#include "CharacterClassCategoryName.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<CharacterClassCategoryName>
	{
		size_t operator()(const CharacterClassCategoryName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

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
