#include <cassert>
#include <map>

#include "CharacterClassCategory.h"

const auto CharacterClassCategoryDisplayNames = std::map<CharacterClassCategoryName, std::string>
{
	{ CharacterClassCategoryName::Mage, "Mage" },
	{ CharacterClassCategoryName::Thief, "Thief" },
	{ CharacterClassCategoryName::Warrior, "Warrior" }
};

CharacterClassCategory::CharacterClassCategory(CharacterClassCategoryName categoryName)
{
	this->categoryName = categoryName;
}

CharacterClassCategory::~CharacterClassCategory()
{

}

CharacterClassCategoryName CharacterClassCategory::getCategoryName() const
{
	return this->categoryName;
}

std::string CharacterClassCategory::toString() const
{
	auto displayName = CharacterClassCategoryDisplayNames.at(this->getCategoryName());
	assert(displayName.size() > 0);
	return displayName;
}
