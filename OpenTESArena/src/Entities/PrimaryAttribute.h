#ifndef PRIMARY_ATTRIBUTE_H
#define PRIMARY_ATTRIBUTE_H

#include <string>
#include <vector>

#include "../Math/Random.h"

class AttributeModifier;

enum class AttributeModifierName;
enum class PrimaryAttributeName;

// The AttributeModifier class is just for the modifier display names. The modifier 
// value can be obtained in this class. AttributeModifiers are in PDF page 22 of the
// manual.
class PrimaryAttribute
{
private:
	static const int MIN_VALUE;
	static const int MAX_VALUE;

	PrimaryAttributeName attributeName;
	int baseValue; // Based on allocated points.
public:
	PrimaryAttribute(PrimaryAttributeName attributeName, int baseValue);
	PrimaryAttribute(PrimaryAttributeName attributeName, int raceID, bool male, Random &random); // Rolls new value based on race & gender.

	int get() const;
	PrimaryAttributeName getAttributeName() const;
	std::vector<AttributeModifierName> getModifierNames() const;
	int getModifier() const;
	std::string toString() const;

	// Perhaps there would be a "getCalculatedValue(EquippedItems..., StatusEffects...)", 
	// which would cap between the min and max value behind the scenes here.

	void set(int value);
};

#endif
