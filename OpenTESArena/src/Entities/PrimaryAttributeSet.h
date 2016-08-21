#ifndef PRIMARY_ATTRIBUTE_SET_H
#define PRIMARY_ATTRIBUTE_SET_H

#include <map>

// This class has a concrete collection of the eight distinct primary attributes.

// It's cleaner to use a mapping of an enumeration to an attribute than having eight
// different private members. 

// I don't think it's important at this point to have custom attributes for modding 
// because the original eight are essential to the game mechanics.

class PrimaryAttribute;

enum class PrimaryAttributeName;

class PrimaryAttributeSet
{
private:
	std::map<PrimaryAttributeName, PrimaryAttribute> primaryAttributes;
public:
	// Initializes all attributes to zero.
	PrimaryAttributeSet();
	~PrimaryAttributeSet();

	const PrimaryAttribute &get(PrimaryAttributeName attributeName) const;
	void set(PrimaryAttributeName attributeName, int value);
};

#endif
