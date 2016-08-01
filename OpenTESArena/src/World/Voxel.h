#ifndef VOXEL_H
#define VOXEL_H

#include <string>
#include <vector>

// Voxels in Arena are about twice as big as those in Minecraft, and are either
// a 1x1x1 cube or a 1x1.5x1 cuboid. These "tall voxels" are found in the Mages'
// Guild, temples, palaces, and certain dungeons.

// Associate transition voxels with a cardinal direction, so there is a direction
// that the player is faced when they are sent through.

// Instead of storing the boolean in every voxel, it should be stored in the chunk
// since it will be constant for all voxels in that chunk. Theoretically, it could
// be moved up even higher to the chunk manager, since it's on a "World" basis
// if interior locations are considered their own world.

class Rect3D;

enum class VoxelMaterialType;
enum class VoxelType;

class Voxel
{
private:
	VoxelType voxelType;
public:
	Voxel(VoxelType voxelType);
	Voxel();
	~Voxel();

	VoxelType getVoxelType() const;
	VoxelMaterialType getVoxelMaterialType() const;
	std::string typeToString() const;
	std::string materialToString() const;

	// Assume that all voxel types are composed of only convex shapes (like cubes). 
	// Objects like doors and flats are not part of the voxel types even though they 
	// are static objects in voxels. They should be managed separately. If this 
	// method is a bottleneck, the return type can be changed to a const vector&.
	std::vector<Rect3D> getGeometry() const;
};

#endif
