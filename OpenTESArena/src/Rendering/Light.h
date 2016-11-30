#ifndef LIGHT_H
#define LIGHT_H

#include "ArrayReference.h"
#include "../Math/Float3.h"

// Helper class for managing lights in OpenCL device memory.

class Light
{
private:
	Float3d point, color;
	OwnerReference ownerRef;
	double intensity;
public:
	Light(const Float3d &point, const Float3d &color,
		const OwnerReference &ownerRef, double intensity);
	~Light();

	const Float3d &getPoint() const;
	const Float3d &getColor() const;
	const OwnerReference &getOwnerRef() const;
	double getIntensity() const;
};

#endif
