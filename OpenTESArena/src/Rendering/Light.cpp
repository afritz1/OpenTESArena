#include "Light.h"

Light::Light(const Float3d &point, const Float3d &color, 
	const OwnerReference &ownerRef, double intensity)
	: point(point), color(color), ownerRef(ownerRef)
{
	this->intensity = intensity;
}

Light::~Light()
{

}

const Float3d &Light::getPoint() const
{
	return this->point;
}

const Float3d &Light::getColor() const
{
	return this->color;
}

const OwnerReference &Light::getOwnerRef() const
{
	return this->ownerRef;
}

double Light::getIntensity() const
{
	return this->intensity;
}
