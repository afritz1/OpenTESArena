#ifndef ATTRIBUTE_MODIFIER_H
#define ATTRIBUTE_MODIFIER_H

#include <string>

#include "AttributeModifierName.h"

// This class is just for toString purposes. Obtaining the modifier value is a simple
// function, and can be done in the PrimaryAttribute class because it's the same for
// all modifiers.

enum class PrimaryAttributeName;

class AttributeModifier
{
private:
	AttributeModifierName modifierName;
public:
	AttributeModifier(AttributeModifierName modifierName);
	~AttributeModifier();

	const AttributeModifierName &getModifierName() const;
	std::string toString() const;
};

#endif