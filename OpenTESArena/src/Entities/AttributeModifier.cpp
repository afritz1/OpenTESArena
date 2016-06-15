#include <cassert>
#include <map>

#include "AttributeModifier.h"

#include "AttributeModifierName.h"

const std::map<AttributeModifierName, std::string> AttributeModifierDisplayNames =
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
	return displayName;
}
