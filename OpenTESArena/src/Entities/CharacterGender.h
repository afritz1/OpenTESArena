#ifndef CHARACTER_GENDER_H
#define CHARACTER_GENDER_H

#include <string>

enum class CharacterGenderName;

class CharacterGender
{
private:
	CharacterGenderName genderName;
public:
	CharacterGender(CharacterGenderName genderName);
	~CharacterGender();

	CharacterGenderName getGenderName() const;
	std::string toString() const;
};

#endif
