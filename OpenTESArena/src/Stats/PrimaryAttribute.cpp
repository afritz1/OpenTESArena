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

void PrimaryAttributes::init(int raceID, bool isMale, const ExeData &exeData)
{
	BufferView<PrimaryAttribute> attributes = this->getAttributes();
	for (int i = 0; i < COUNT; i++)
	{
		DebugAssertIndex(exeData.entities.attributeNames, i);
		const std::string &attributeName = exeData.entities.attributeNames[i];
		const int raceAttributesIndex = (raceID * 2) + (isMale ? 0 : 1); // Alternates male/female
		DebugAssertIndex(exeData.entities.raceAttributes, raceAttributesIndex);
		const BufferView<const uint8_t> raceAttributesView = exeData.entities.raceAttributes[raceAttributesIndex];
		const uint8_t attributeValue = raceAttributesView[i]; // 0..255
		const int attributeValueCorrected = static_cast<int>(std::round(static_cast<double>(attributeValue) / (255.0 / 100.0)));

		PrimaryAttribute &attribute = attributes[i];
		attribute.init(attributeName.c_str(), attributeValueCorrected);
	}
}

BufferView<PrimaryAttribute> PrimaryAttributes::getAttributes()
{
	return BufferView<PrimaryAttribute>(&this->strength, COUNT);
}

BufferView<const PrimaryAttribute> PrimaryAttributes::getAttributes() const
{
	return BufferView<const PrimaryAttribute>(&this->strength, COUNT);
}

void PrimaryAttributes::clear()
{
	for (PrimaryAttribute &attribute : this->getAttributes())
	{
		attribute.clear();
	}
}