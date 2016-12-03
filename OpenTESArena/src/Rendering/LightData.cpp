#include "LightData.h"

LightData::LightData(const Light &light, const OwnerReference &ownerRef)
	: light(light), ownerRef(ownerRef) { }

LightData::~LightData()
{

}

const Light &LightData::getLight() const
{
	return this->light;
}

const OwnerReference &LightData::getOwnerRef() const
{
	return this->ownerRef;
}
