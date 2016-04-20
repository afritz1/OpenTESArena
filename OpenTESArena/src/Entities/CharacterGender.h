#ifndef CHARACTER_GENDER_H
#define CHARACTER_GENDER_H

#include <string>

#include "CharacterGenderName.h"

class CharacterGender
{
private:
	CharacterGenderName genderName;
public:
	CharacterGender(CharacterGenderName genderName);
	~CharacterGender();

	const CharacterGenderName &getGenderName() const;
	std::string toString() const;
};

#endif
