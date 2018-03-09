#include <unordered_map>

#include "Gender.h"
#include "GenderName.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<GenderName>
	{
		size_t operator()(const GenderName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

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
