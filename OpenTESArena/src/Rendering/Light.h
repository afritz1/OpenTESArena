#ifndef LIGHT_H
#define LIGHT_H

#include "../Math/Float3.h"

// Helper class for managing lights in OpenCL device memory.

// The owner reference is managed elsewhere in the CLProgram. This class is just 
// for the light's data.

class Light
{
private:
	Float3d point, color;
	double intensity;
public:
	Light(const Float3d &point, const Float3d &color, double intensity);
	~Light();

	const Float3d &getPoint() const;
	const Float3d &getColor() const;
	double getIntensity() const;
};

#endif
