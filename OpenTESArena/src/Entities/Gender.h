#ifndef GENDER_H
#define GENDER_H

#include <string>

enum class GenderName;

class Gender
{
private:
	GenderName genderName;
public:
	Gender(GenderName genderName);
	~Gender();

	GenderName getGenderName() const;
	std::string toString() const;
};

#endif
