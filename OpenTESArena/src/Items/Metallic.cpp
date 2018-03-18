#include "Metal.h"
#include "Metallic.h"
#include "MetalType.h"

Metallic::Metallic(MetalType metalType)
	: metal(metalType) { }

const Metal &Metallic::getMetal() const
{
	return this->metal;
}
