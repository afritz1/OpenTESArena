#ifndef PRIMARY_ATTRIBUTE_NAME_H
#define PRIMARY_ATTRIBUTE_NAME_H

// A unique identifier for each primary attribute.
enum class PrimaryAttributeName
{
	Strength,
	Intelligence,
	Willpower,
	Agility,
	Speed,
	Endurance,
	Personality,
	Luck
};

// An array for iterating over primary attribute names.
static const PrimaryAttributeName PRIMARY_ATTRIBUTE_NAMES[] = {
	PrimaryAttributeName::Strength,
	PrimaryAttributeName::Intelligence,
	PrimaryAttributeName::Willpower,
	PrimaryAttributeName::Agility,
	PrimaryAttributeName::Speed,
	PrimaryAttributeName::Endurance,
	PrimaryAttributeName::Personality,
	PrimaryAttributeName::Luck,
};

#endif
