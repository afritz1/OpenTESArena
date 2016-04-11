#include <cassert>
#include <map>
#include <vector>

#include "CharacterClassCategory.h"

#include "CharacterClassName.h"

const auto CharacterClassCategoryDisplayNames = std::map<CharacterClassCategoryName, std::string>
{
	{ CharacterClassCategoryName::Mage, "Mage" },
	{ CharacterClassCategoryName::Thief, "Thief" },
	{ CharacterClassCategoryName::Warrior, "Warrior" }
};

const auto CharacterClassCategoryNames = std::map<CharacterClassCategoryName, std::vector<CharacterClassName>>
{
	{ CharacterClassCategoryName::Mage, { CharacterClassName::BattleMage,
	CharacterClassName::Healer, CharacterClassName::Mage, CharacterClassName::Nightblade,
	CharacterClassName::Sorcerer, CharacterClassName::Spellsword } },

	{ CharacterClassCategoryName::Thief, { CharacterClassName::Acrobat,
	CharacterClassName::Assassin, CharacterClassName::Bard, CharacterClassName::Burglar,
	CharacterClassName::Rogue, CharacterClassName::Thief } },

	{ CharacterClassCategoryName::Warrior, { CharacterClassName::Archer,
	CharacterClassName::Barbarian, CharacterClassName::Knight, CharacterClassName::Monk,
	CharacterClassName::Ranger, CharacterClassName::Warrior } }
};

CharacterClassCategory::CharacterClassCategory(CharacterClassCategoryName categoryName)
{
	this->categoryName = categoryName;
}

CharacterClassCategory::CharacterClassCategory(CharacterClassName className)
{
	// Find the parent name, given the child name.
	this->categoryName = [&className]()
	{
		for (auto &item : CharacterClassCategoryNames)
		{
			for (auto &name : item.second)
			{
				if (name == className)
				{
					return item.first;
				}
			}
		}

		// If not in the list, programmer error.
		throw std::exception("CharacterClassName has no parent.");
	}();
}

CharacterClassCategory::~CharacterClassCategory()
{

}

const CharacterClassCategoryName &CharacterClassCategory::getCategoryName() const
{
	return this->categoryName;
}

std::string CharacterClassCategory::toString() const
{
	auto displayName = CharacterClassCategoryDisplayNames.at(this->getCategoryName());
	assert(displayName.size() > 0);
	return displayName;
}
