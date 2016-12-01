#include "Light.h"

Light::Light(const Float3d &point, const Float3d &color, double intensity)
	: point(point), color(color)
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

double Light::getIntensity() const
{
	return this->intensity;
}
