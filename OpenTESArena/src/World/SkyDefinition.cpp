#include "SkyDefinition.h"

#include "components/debug/Debug.h"

void SkyDefinition::init(Buffer<Color> &&skyColors)
{
	this->skyColors = std::move(skyColors);
}

int SkyDefinition::getSkyColorCount() const
{
	return this->skyColors.getCount();
}

const Color &SkyDefinition::getSkyColor(int index)
{
	return this->skyColors.get(index);
}

int SkyDefinition::getObjectCount() const
{
	return static_cast<int>(this->objects.size());
}

const SkyObjectDefinition &SkyDefinition::getObject(int index) const
{
	DebugAssertIndex(this->objects, index);
	return this->objects[index];
}

int SkyDefinition::addObject(SkyObjectDefinition &&def)
{
	this->objects.emplace_back(std::move(def));
	return static_cast<int>(this->objects.size()) - 1;
}
