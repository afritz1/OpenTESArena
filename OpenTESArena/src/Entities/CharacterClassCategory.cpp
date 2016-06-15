#include <cassert>
#include <map>

#include "CharacterClassCategory.h"

#include "CharacterClassCategoryName.h"

const std::map<CharacterClassCategoryName, std::string> CharacterClassCategoryDisplayNames =
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
	return displayName;
}
