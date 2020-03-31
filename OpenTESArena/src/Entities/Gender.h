#ifndef GENDER_H
#define GENDER_H

#include <string>

// Namespace for converting a gender name to a string.

enum class GenderName;

namespace Gender
{
	const std::string &toString(GenderName genderName);
}

#endif
