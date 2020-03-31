#ifndef CHARACTER_CLASS_CATEGORY_H
#define CHARACTER_CLASS_CATEGORY_H

#include <string>

// This namespace exists mostly for implementing the "toString()" method for 
// class categories. Otherwise, the character class would have two methods which 
// fight over a similar name.

enum class CharacterClassCategoryName;

namespace CharacterClassCategory
{
	const std::string &toString(CharacterClassCategoryName categoryName);
}

#endif
