#ifndef LIGHT_DATA_H
#define LIGHT_DATA_H

//#include "ArrayReference.h"
#include "Light.h"

// 1/7/2017
// Currently cutting out owner references to simplify the lighting code for now.
// I hit too much of a roadblock with my cluttered design, and the OpenCL code just 
// needs refactoring in general. Once the rendering code is more robust, then light
// owners can be added.

// Helper class for managing a light and its owner reference in OpenCL device memory.
// This class should only be used for lights that actually have owners.

class LightData
{
private:
	Light light;
	//OwnerReference ownerRef;
public:
	LightData(const Light &light/*, const OwnerReference &ownerRef*/);
	~LightData();

	const Light &getLight() const;
	//const OwnerReference &getOwnerRef() const;
};

#endif
