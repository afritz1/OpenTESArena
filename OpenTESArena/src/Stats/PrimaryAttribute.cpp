#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>

#include "PrimaryAttribute.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

static_assert(sizeof(PrimaryAttributes) == (sizeof(PrimaryAttribute) * PrimaryAttributes::COUNT));
static_assert(offsetof(PrimaryAttributes, strength) == 0);
static_assert(offsetof(PrimaryAttributes, luck) == (sizeof(PrimaryAttribute) * (PrimaryAttributes::COUNT - 1)));

PrimaryAttribute::PrimaryAttribute()
{
	this->clear();
}

void PrimaryAttribute::init(const char *name, int maxValue)
{
	DebugAssert(maxValue >= 0);
	std::snprintf(std::begin(this->name), std::size(this->name), "%s", name);
	this->maxValue = maxValue;
}

void PrimaryAttribute::clear()
{
	std::fill(std::begin(this->name), std::end(this->name), '\0');
	this->maxValue = 0;
}

void PrimaryAttributes::init(CharacterRaceDefinitionID raceDefID, bool isMale, const ExeData &exeData)
{
	const CharacterRaceDefinition &raceDef = CharacterRaceLibrary::getInstance().getDefinition(raceDefID);
	const Span<const std::string> attributeNames = exeData.entities.attributeNames;

	Span<PrimaryAttribute> attributes = this->getView();
	for (int i = 0; i < COUNT; i++)
	{
		const std::string &attributeName = attributeNames[i];
		const int raceAttributesIndex = (raceDef.originalRaceIndex * 2) + (isMale ? 0 : 1); // Alternates male/female
		const Span<const uint8_t> raceAttributes = exeData.entities.raceAttributes[raceAttributesIndex];
		const uint8_t attributeValue = raceAttributes[i]; // 0..255
		const int attributeValueCorrected = static_cast<int>(std::round(static_cast<double>(attributeValue) / (255.0 / 100.0))); // @todo use ArenaStatUtils::scale256To100() here?
		
		PrimaryAttribute &attribute = attributes[i];
		attribute.init(attributeName.c_str(), attributeValueCorrected);
	}
}

Span<PrimaryAttribute> PrimaryAttributes::getView()
{
	return Span<PrimaryAttribute>(&this->strength, COUNT);
}

Span<const PrimaryAttribute> PrimaryAttributes::getView() const
{
	return Span<const PrimaryAttribute>(&this->strength, COUNT);
}

void PrimaryAttributes::clear()
{
	for (PrimaryAttribute &attribute : this->getView())
	{
		attribute.clear();
	}
}

DerivedAttributes::DerivedAttributes()
{
	this->clear();
}

void DerivedAttributes::clear()
{
	this->bonusDamage = 0;
	this->maxKilos = 0;
	this->magicDef = 0;
	this->bonusToHit = 0;
	this->bonusToDefend = 0;
	this->bonusToHealth = 0;
	this->healMod = 0;
	this->bonusToCharisma = 0;
}

bool DerivedAttributes::isModifier(int index)
{
	return index != 1;
}

Span<const int> DerivedAttributes::getView() const
{
	return Span<const int>(&this->bonusDamage, COUNT);
}
