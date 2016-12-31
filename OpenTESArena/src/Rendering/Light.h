#ifndef LIGHT_H
#define LIGHT_H

#include <vector>

#include "../Math/Float3.h"

// Helper class for managing lights in OpenCL device memory.

// The owner reference is managed elsewhere in the CLProgram. This class is just 
// for the light's data.

class Int3;

class Light
{
private:
	Float3d point, color;
	double intensity;

	// Gets the axis-aligned bounding box for the light.
	std::pair<Float3f, Float3f> getAABB() const;
public:
	Light(const Float3d &point, const Float3d &color, double intensity);
	~Light();

	const Float3d &getPoint() const;
	const Float3d &getColor() const;
	double getIntensity() const;

	// Returns a vector of voxel coordinates for all voxels that the light reaches,
	// with the option to only get voxels within the world bounds.
	std::vector<Int3> getTouchedVoxels(int worldWidth, int worldHeight, int worldDepth) const;
	std::vector<Int3> getTouchedVoxels() const;
};

#endif
