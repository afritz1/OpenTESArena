#ifndef ATTRIBUTE_MODIFIER_NAME_H
#define ATTRIBUTE_MODIFIER_NAME_H

// A unique identifier for each [-5, 5] attribute modifier.
// The new design will use tooltips on the attributes instead of showing them 
// directly with the other attributes.
// Carry weight is not included because it's not a [-5, 5] value.
enum class AttributeModifierName
{
	// Strength
	MeleeDamage, 

	// Willpower
	MagicDefense, 

	// Agility
	ToHit, 
	ToDefense,

	// Endurance
	HealthPerLevel, 
	HealModifier,

	// Personality
	Charisma
};

#endif
