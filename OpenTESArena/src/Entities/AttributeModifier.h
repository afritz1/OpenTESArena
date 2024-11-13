#ifndef ATTRIBUTE_MODIFIER_H
#define ATTRIBUTE_MODIFIER_H

#include <string>

enum class AttributeModifierName;

// This namespace is just for toString() purposes. Obtaining the modifier value is a 
// simple function, and can be done in the PrimaryAttribute class because it's the same 
// for all modifiers.
namespace AttributeModifier
{
	const std::string &toString(AttributeModifierName modifierName);
}

#endif
