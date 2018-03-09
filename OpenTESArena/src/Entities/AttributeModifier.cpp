#include <unordered_map>

#include "AttributeModifier.h"
#include "AttributeModifierName.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<AttributeModifierName>
	{
		size_t operator()(const AttributeModifierName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

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
