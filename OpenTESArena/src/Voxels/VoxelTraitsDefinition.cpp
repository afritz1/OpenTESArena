#include <cstring>

#include "VoxelTraitsDefinition.h"

VoxelTraitsDefinition::VoxelTraitsDefinition()
{
	// Default to air.
	std::memset(this, 0, sizeof(*this));
	this->type = ArenaVoxelType::None;
}

void VoxelTraitsDefinition::initGeneral(ArenaVoxelType type)
{
	this->type = type;
}

void VoxelTraitsDefinition::initFloor(bool isWildWallColored)
{
	this->initGeneral(ArenaVoxelType::Floor);
	this->floor.isWildWallColored = isWildWallColored;
}

void VoxelTraitsDefinition::initTransparentWall(bool collider)
{
	this->initGeneral(ArenaVoxelType::TransparentWall);
	this->transparentWall.collider = collider;
}

void VoxelTraitsDefinition::initEdge(VoxelFacing2D facing, bool collider)
{
	this->initGeneral(ArenaVoxelType::Edge);
	this->edge.facing = facing;
	this->edge.collider = collider;
}

void VoxelTraitsDefinition::initChasm(ArenaChasmType chasmType)
{
	this->initGeneral(ArenaVoxelType::Chasm);
	this->chasm.type = chasmType;
}

bool VoxelTraitsDefinition::hasCollision() const
{
	switch (this->type)
	{
	case ArenaVoxelType::None:
		return false;
	case ArenaVoxelType::TransparentWall:
		return this->transparentWall.collider;
	case ArenaVoxelType::Edge:
		return this->edge.collider;
	default:
		return true;
	}
}
