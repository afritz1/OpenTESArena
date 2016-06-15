#include <cassert>
#include <map>

#include "CharacterGender.h"

#include "CharacterGenderName.h"

const std::map<CharacterGenderName, std::string> CharacterGenderDisplayNames =
{
	{ CharacterGenderName::Female, "Female" },
	{ CharacterGenderName::Male, "Male" }
};

CharacterGender::CharacterGender(CharacterGenderName genderName)
{
	this->genderName = genderName;
}

CharacterGender::~CharacterGender()
{

}

CharacterGenderName CharacterGender::getGenderName() const
{
	return this->genderName;
}

std::string CharacterGender::toString() const
{
	auto displayName = CharacterGenderDisplayNames.at(this->getGenderName());
	return displayName;
}
