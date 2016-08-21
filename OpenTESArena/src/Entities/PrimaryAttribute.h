#ifndef PRIMARY_ATTRIBUTE_H
#define PRIMARY_ATTRIBUTE_H

#include <cstdint>
#include <string>
#include <vector>

// The AttributeModifier class is just for the modifier display names. The modifier 
// value can be obtained in this class. AttributeModifiers are in PDF page 22 of the
// manual.

class AttributeModifier;

enum class AttributeModifierName;
enum class PrimaryAttributeName;

class PrimaryAttribute
{
private:
	static const int32_t MIN_VALUE;
	static const int32_t MAX_VALUE;

	PrimaryAttributeName attributeName;
	int32_t baseValue; // Based on allocated points.
public:
	PrimaryAttribute(PrimaryAttributeName attributeName, int32_t baseValue);
	~PrimaryAttribute();

	int32_t get() const;
	PrimaryAttributeName getAttributeName() const;
	std::vector<AttributeModifierName> getModifierNames() const;
	int32_t getModifier() const;
	std::string toString() const;

	// Perhaps there would be a "getCalculatedValue(EquippedItems..., StatusEffects...)", 
	// which would cap between the min and max value behind the scenes here.

	void set(int32_t value);
};

#endif
