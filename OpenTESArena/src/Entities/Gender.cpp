#include <unordered_map>

#include "Gender.h"
#include "GenderName.h"

const std::unordered_map<GenderName, std::string> GenderDisplayNames =
{
	{ GenderName::Female, "Female" },
	{ GenderName::Male, "Male" }
};

const std::string &Gender::toString(GenderName genderName)
{
	const std::string &displayName = GenderDisplayNames.at(genderName);
	return displayName;
}
