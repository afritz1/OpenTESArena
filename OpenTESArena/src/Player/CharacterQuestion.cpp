#include "CharacterQuestion.h"

CharacterQuestion::CharacterQuestion(std::string &&description, Choice &&a,
	Choice &&b, Choice &&c)
	: description(std::move(description)), a(std::move(a)), b(std::move(b)), c(std::move(c)) { }

const std::string &CharacterQuestion::getDescription() const
{
	return this->description;
}

const CharacterQuestion::Choice &CharacterQuestion::getA() const
{
	return this->a;
}

const CharacterQuestion::Choice &CharacterQuestion::getB() const
{
	return this->b;
}

const CharacterQuestion::Choice &CharacterQuestion::getC() const
{
	return this->c;
}
