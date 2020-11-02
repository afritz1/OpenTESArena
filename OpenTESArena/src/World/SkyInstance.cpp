#include "SkyDefinition.h"
#include "SkyInstance.h"

#include "components/debug/Debug.h"

int SkyInstance::getObjectCount() const
{
	return static_cast<int>(this->objects.size());
}

const SkyObjectInstance &SkyInstance::getObject(int index) const
{
	DebugAssertIndex(this->objects, index);
	return this->objects[index];
}

void SkyInstance::addObject(SkyObjectInstance &&inst)
{
	this->objects.emplace_back(std::move(inst));
}

void SkyInstance::update(double dt, double latitude, double daytimePercent,
	const SkyDefinition &skyDefinition)
{
	const int objectCount = this->getObjectCount();
	for (int i = 0; i < objectCount; i++)
	{
		SkyObjectInstance &objectInst = this->objects[i];
		const SkyObjectDefinition &objectDef = skyDefinition.getObject(objectInst.getDefIndex());
		objectInst.update(dt, objectDef);
	}
}
