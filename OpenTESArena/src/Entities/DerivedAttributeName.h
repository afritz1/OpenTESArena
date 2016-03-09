#ifndef DERIVED_ATTRIBUTE_NAME_H
#define DERIVED_ATTRIBUTE_NAME_H

// A unique identifier for each kind of derived attribute. 

// These are distinct from modifier types, like extra melee damage, etc., because 
// these are considered "pools", not just functions of primary attributes.
enum class DerivedAttributeName
{
	Health,
	SpellPoints,
	Stamina
};

#endif