#include <cassert>
#include <map>

#include "CharacterGender.h"

const auto CharacterGenderDisplayNames = std::map<CharacterGenderName, std::string>
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

const CharacterGenderName &CharacterGender::getGenderName() const
{
	return this->genderName;
}

std::string CharacterGender::toString() const
{
	auto displayName = CharacterGenderDisplayNames.at(this->getGenderName());
	assert(displayName.size() > 0);
	return displayName;
}
