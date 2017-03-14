#ifndef CHARACTER_CLASS_CATEGORY_H
#define CHARACTER_CLASS_CATEGORY_H

#include <string>

// This static class exists mostly for implementing the "toString()" method for 
// class categories. Otherwise, the character class would have two methods which 
// fight over a similar name.

enum class CharacterClassCategoryName;

class CharacterClassCategory
{
private:
	CharacterClassCategory() = delete;
	~CharacterClassCategory() = delete;
public:
	static const std::string &toString(CharacterClassCategoryName categoryName);
};

#endif
