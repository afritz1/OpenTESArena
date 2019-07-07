#include "CharacterQuestion.h"
#include "../Entities/CharacterClassCategoryName.h"

#include "components/debug/Debug.h"

CharacterQuestion::CharacterQuestion(const std::string &description,
	const std::pair<std::string, CharacterClassCategoryName> &a,
	const std::pair<std::string, CharacterClassCategoryName> &b,
	const std::pair<std::string, CharacterClassCategoryName> &c)
	: description(description), a(a), b(b), c(c)
{
	// Make sure none of the class categories match.
	DebugAssert(a.second != b.second);
	DebugAssert(a.second != c.second);
	DebugAssert(b.second != c.second);
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
