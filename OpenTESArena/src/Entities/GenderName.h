#ifndef GENDER_NAME_H
#define GENDER_NAME_H

// I prefer using a strongly typed enum here instead of "isMale" or something.
// It allows trivial mappings for getting gender-specific information, like
// standing height or primary attribute changes.
enum class GenderName
{
	Female,
	Male
};

#endif
