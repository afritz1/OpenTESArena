#include <cassert>

#include "CharacterQuestion.h"
#include "../Entities/CharacterClassCategoryName.h"

CharacterQuestion::CharacterQuestion(const std::string &description,
	const std::pair<std::string, CharacterClassCategoryName> &a,
	const std::pair<std::string, CharacterClassCategoryName> &b,
	const std::pair<std::string, CharacterClassCategoryName> &c)
	: description(description), a(a), b(b), c(c)
{
	// Make sure none of the class categories match.
	assert(a.second != b.second);
	assert(a.second != c.second);
	assert(b.second != c.second);
}

CharacterQuestion::~CharacterQuestion()
{

}

const std::string &CharacterQuestion::getDescription() const
{
	return this->description;
}

const std::pair<std::string, CharacterClassCategoryName> &CharacterQuestion::getA() const
{
	return this->a;
}

const std::pair<std::string, CharacterClassCategoryName> &CharacterQuestion::getB() const
{
	return this->b;
}

const std::pair<std::string, CharacterClassCategoryName> &CharacterQuestion::getC() const
{
	return this->c;
}
