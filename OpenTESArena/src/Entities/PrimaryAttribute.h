#ifndef PRIMARY_ATTRIBUTE_H
#define PRIMARY_ATTRIBUTE_H

#include <string>
#include <vector>

#include "PrimaryAttributeName.h"

// The AttributeModifier class is just for the modifier display names. The modifier 
// value can be obtained in this class. AttributeModifiers are in PDF page 22 of the
// manual.

class AttributeModifier;

enum class AttributeModifierName;

class PrimaryAttribute
{
private:
	static const int MIN_VALUE;
	static const int MAX_VALUE;

	PrimaryAttributeName attributeName;
	int baseValue; // Based on allocated points.
public:
	PrimaryAttribute(PrimaryAttributeName attributeName, int baseValue);
	~PrimaryAttribute();

	const int &get() const;
	const PrimaryAttributeName &getAttributeName() const;
	std::vector<AttributeModifierName> getModifierNames() const;
	int getModifier() const;
	std::string toString() const;

	// Perhaps there would be a "getCalculatedValue(EquippedItems..., StatusEffects...)", 
	// which would cap between the min and max value behind the scenes here.

	void set(int value);
};

#endif
