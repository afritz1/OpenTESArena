#ifndef CHARACTER_QUESTION_H
#define CHARACTER_QUESTION_H

#include <string>

#include "../Entities/CharacterClassDefinition.h"

// A definition for each question object in QUESTION.TXT. Each question has a 
// description and three choices. Each choice points to a particular class category 
// (mage, thief, warrior).

class CharacterQuestion
{
public:
	using Choice = std::pair<std::string, CharacterClassDefinition::CategoryID>;
private:
	std::string description;
	Choice a, b, c;
public:
	CharacterQuestion(std::string &&description, Choice &&a, Choice &&b, Choice &&c);

	// Gets the description for the character question.
	const std::string &getDescription() const;

	// Gets the text and associated class category for a given option.
	const Choice &getA() const;
	const Choice &getB() const;
	const Choice &getC() const;
};

#endif
