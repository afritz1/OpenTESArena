#ifndef CHARACTER_CLASS_CATEGORY_H
#define CHARACTER_CLASS_CATEGORY_H

#include <string>

// This class is mostly for implementing the "toString()" method for class categories. 
// Otherwise, the character class has two methods which fight over a similar name.

enum class CharacterClassCategoryName;

class CharacterClassCategory
{
private:
	CharacterClassCategoryName categoryName;
public:
	CharacterClassCategory(CharacterClassCategoryName categoryName);
	~CharacterClassCategory();

	CharacterClassCategoryName getCategoryName() const;
	std::string toString() const;
};

#endif
