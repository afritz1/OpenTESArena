#ifndef VOXEL_H
#define VOXEL_H

// Intended for gameplay rules. I might use this with geometry, but I'm not sure.

// Voxels in Arena are about twice as big as those in Minecraft, and are either
// a 1x1x1 cube or a 1x1.5x1 cuboid. These "tall voxels" are found in the Mages'
// Guild, temples, palaces, and certain dungeons.

// Associate transition voxels with a cardinal direction, so there is a direction
// that the player is faced when they are sent through.

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
};

#endif
