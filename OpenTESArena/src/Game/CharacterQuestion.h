#ifndef CHARACTER_QUESTION_H
#define CHARACTER_QUESTION_H

#include <string>

// A definition for each question object in QUESTION.TXT. Each question has a 
// description and three choices. Each choice points to a particular class category 
// (mage, thief, warrior).

enum class CharacterClassCategoryName;

class CharacterQuestion
{
private:
	std::string description;
	std::pair<std::string, CharacterClassCategoryName> a, b, c;
public:
	CharacterQuestion(const std::string &description,
		const std::pair<std::string, CharacterClassCategoryName> &a,
		const std::pair<std::string, CharacterClassCategoryName> &b,
		const std::pair<std::string, CharacterClassCategoryName> &c);

	// Gets the description for the character question.
	const std::string &getDescription() const;

	// Gets the text and associated class category for a given option.
	const std::pair<std::string, CharacterClassCategoryName> &getA() const;
	const std::pair<std::string, CharacterClassCategoryName> &getB() const;
	const std::pair<std::string, CharacterClassCategoryName> &getC() const;
};

#endif
