#ifndef CHARACTER_QUESTION_H
#define CHARACTER_QUESTION_H

#include "../Stats/CharacterClassDefinition.h"

struct CharacterQuestionChoice
{
	char text[512];
	CharacterClassCategoryID categoryID;

	CharacterQuestionChoice();

	void init(const char *text, CharacterClassCategoryID categoryID);
};

struct CharacterQuestion
{
	char description[1024];
	CharacterQuestionChoice a, b, c;

	CharacterQuestion();

	void init(const char *description, const CharacterQuestionChoice &a, const CharacterQuestionChoice &b, const CharacterQuestionChoice &c);
};

#endif
