#include <cassert>
#include <map>

#include "AttributeModifier.h"

const auto AttributeModifierDisplayNames = std::map<AttributeModifierName, std::string>
{
	{ AttributeModifierName::MeleeDamage, "Melee Damage" },
	{ AttributeModifierName::MagicDefense, "Magic Defense" },
	{ AttributeModifierName::ToHit, "To Hit" }, 
	{ AttributeModifierName::ToDefense, "To Defense" }, 
	{ AttributeModifierName::HealthPerLevel, "Health Per Level" },
	{ AttributeModifierName::HealModifier, "Heal Modifier" },
	{ AttributeModifierName::Charisma, "Charisma" }
};

AttributeModifier::AttributeModifier(AttributeModifierName modifierName)
{
	this->modifierName = modifierName;
}

AttributeModifier::~AttributeModifier()
{

}

AttributeModifierName AttributeModifier::getModifierName() const
{
	return this->modifierName;
}

std::string AttributeModifier::toString() const
{
	auto displayName = AttributeModifierDisplayNames.at(this->getModifierName());
	assert(displayName.size() > 0);
	return displayName;
}
