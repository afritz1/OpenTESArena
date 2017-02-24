#include <cassert>
#include <map>

#include "Gender.h"

#include "GenderName.h"

const std::map<GenderName, std::string> GenderDisplayNames =
{
	{ GenderName::Female, "Female" },
	{ GenderName::Male, "Male" }
};

Gender::Gender(GenderName genderName)
{
	this->genderName = genderName;
}

Gender::~Gender()
{

}

GenderName Gender::getGenderName() const
{
	return this->genderName;
}

std::string Gender::toString() const
{
	std::string displayName = GenderDisplayNames.at(this->getGenderName());
	return displayName;
}
