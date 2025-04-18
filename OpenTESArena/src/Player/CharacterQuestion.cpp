#include <algorithm>
#include <cstring>

#include "CharacterQuestion.h"

CharacterQuestionChoice::CharacterQuestionChoice()
{
	this->categoryID = -1;
}

void CharacterQuestionChoice::init(const char *text, CharacterClassCategoryID categoryID)
{
	std::snprintf(this->text, std::size(this->text), "%s", text);
	this->categoryID = categoryID;
}

CharacterQuestion::CharacterQuestion()
{
	std::fill(std::begin(this->description), std::end(this->description), '\0');
}

void CharacterQuestion::init(const char *description, const CharacterQuestionChoice &a, const CharacterQuestionChoice &b, const CharacterQuestionChoice &c)
{
	std::snprintf(this->description, std::size(this->description), "%s", description);
	this->a = a;
	this->b = b;
	this->c = c;
}
