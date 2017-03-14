#ifndef GENDER_H
#define GENDER_H

#include <string>

// Static class for converting a gender name to a string.

enum class GenderName;

class Gender
{
private:
	Gender() = delete;
	~Gender() = delete;
public:
	static const std::string &toString(GenderName genderName);
};

#endif
