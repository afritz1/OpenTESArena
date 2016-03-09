#ifndef PRIMARY_ATTRIBUTE_SET_H
#define PRIMARY_ATTRIBUTE_SET_H

#include <map>

#include "PrimaryAttributeName.h"

// This class has a concrete collection of the eight distinct primary attributes.

class PrimaryAttribute;

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