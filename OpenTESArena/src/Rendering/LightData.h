#ifndef LIGHT_DATA_H
#define LIGHT_DATA_H

#include "ArrayReference.h"
#include "Light.h"

// Helper class for managing a light and its owner reference in OpenCL device memory.
// This class should only be used for lights that actually have owners.

class LightData
{
private:
	Light light;
	OwnerReference ownerRef;
public:
	LightData(const Light &light, const OwnerReference &ownerRef);
	~LightData();

	const Light &getLight() const;
	const OwnerReference &getOwnerRef() const;
};

#endif
