#include <cassert>

#include "Metallic.h"

#include "Metal.h"
#include "MetalType.h"

Metallic::Metallic(MetalType metalType)
{
	this->metal = std::unique_ptr<Metal>(new Metal(metalType));
}

Metallic::~Metallic()
{

}

const Metal &Metallic::getMetal() const
{
	assert(this->metal.get() != nullptr);

	return *this->metal;
}
