#include <unordered_map>

#include "AttributeModifier.h"
#include "AttributeModifierName.h"

// @todo: very old, replace this with ExeData.
const std::unordered_map<AttributeModifierName, std::string> AttributeModifierDisplayNames =
{
	{ AttributeModifierName::MeleeDamage, "Melee Damage" },
	{ AttributeModifierName::MagicDefense, "Magic Defense" },
	{ AttributeModifierName::ToHit, "To Hit" }, 
	{ AttributeModifierName::ToDefense, "To Defense" }, 
	{ AttributeModifierName::HealthPerLevel, "Health Per Level" },
	{ AttributeModifierName::HealModifier, "Heal Modifier" },
	{ AttributeModifierName::Charisma, "Charisma" }
};

const std::string &AttributeModifier::toString(AttributeModifierName modifierName)
{
	const std::string &displayName = AttributeModifierDisplayNames.at(modifierName);
	return displayName;
}
