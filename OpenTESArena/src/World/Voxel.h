#ifndef VOXEL_H
#define VOXEL_H

#include <string>
#include <vector>

#include "VoxelType.h"

// Voxels in Arena are about twice as big as those in Minecraft, and are either
// a 1x1x1 cube or a 1x1.5x1 cuboid. These "tall voxels" are found in the Mages'
// Guild, temples, palaces, and certain dungeons.

// Instead of storing the boolean in every voxel, it should be stored in the chunk
// since it will be constant for all voxels in that chunk. Theoretically, it could
// be moved up even higher to the chunk manager, since it's on a "World" basis
// if interior locations are considered their own world.

class Triangle;

enum class VoxelMaterialType;

class Voxel
{
private:
	VoxelType voxelType;
public:
	Voxel(VoxelType voxelType);
	Voxel();
	~Voxel();

	const VoxelType &getVoxelType() const;
	VoxelMaterialType getVoxelMaterialType() const;
	std::string typeToString() const;
	std::string materialToString() const;

	// This method replaces the VoxelTemplate class. Assume that all voxel types
	// are composed of only convex shapes (like cubes). Objects like doors and
	// flats are not part of the voxel types even though they are static objects
	// in voxels. They should be managed separately. If this method is a bottleneck,
	// the return type can be changed to a const vector&.
	std::vector<Triangle> getGeometry() const;
};

#endif
