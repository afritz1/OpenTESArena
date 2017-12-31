#ifndef METALLIC_H
#define METALLIC_H

#include "Metal.h"

// Inherit this class if the item type has any kind of metal associated with it.

enum class MetalType;

class Metallic
{
private:
	Metal metal;
public:
	Metallic(MetalType metalType);
	~Metallic();

	const Metal &getMetal() const;
};

#endif
