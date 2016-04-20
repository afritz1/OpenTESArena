#ifndef CHARACTER_GENDER_NAME_H
#define CHARACTER_GENDER_NAME_H

// I prefer using a strongly typed enum here instead of "isMale" or something.
// It allows trivial std::map tables for getting gender-specific information,
// like standing height or primary attribute changes.
enum class CharacterGenderName
{
	Female,
	Male
};

#endif
