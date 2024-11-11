#ifndef METALLIC_H
#define METALLIC_H

#include "Metal.h"

enum class MetalType;

// Inherit this class if the item type has any kind of metal associated with it.
class Metallic
{
private:
	Metal metal;
public:
	Metallic(MetalType metalType);

	const Metal &getMetal() const;
};

#endif
