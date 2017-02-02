#ifndef LIGHT_H
#define LIGHT_H

#include <vector>

#include "../Math/Vector3.h"

// Helper class for managing lights in renderer memory.

// The owner reference is managed elsewhere in the renderer. This class is just 
// for the light's data.

class Light
{
private:
	Double3 point, color;
	double intensity;

	// Gets the axis-aligned bounding box for the light.
	std::pair<Float3, Float3> getAABB() const;
public:
	Light(const Double3 &point, const Double3 &color, double intensity);
	~Light();

	const Double3 &getPoint() const;
	const Double3 &getColor() const;
	double getIntensity() const;

	// Returns a vector of voxel coordinates for all voxels that the light reaches,
	// with the option to only get voxels within the world bounds.
	std::vector<Int3> getTouchedVoxels(int worldWidth, int worldHeight, int worldDepth) const;
	std::vector<Int3> getTouchedVoxels() const;
};

#endif
