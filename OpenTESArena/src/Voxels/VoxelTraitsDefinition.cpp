#include <cstring>

#include "VoxelTraitsDefinition.h"

VoxelTraitsDefinition::VoxelTraitsDefinition()
{
	// Default to air.
	std::memset(this, 0, sizeof(*this));
	this->type = ArenaTypes::VoxelType::None;
}

void VoxelTraitsDefinition::initGeneral(ArenaTypes::VoxelType type)
{
	this->type = type;
}

void VoxelTraitsDefinition::initFloor(bool isWildWallColored)
{
	this->initGeneral(ArenaTypes::VoxelType::Floor);
	this->floor.isWildWallColored = isWildWallColored;
}

void VoxelTraitsDefinition::initRaised(double yOffset, double ySize)
{
	this->initGeneral(ArenaTypes::VoxelType::Raised);
	this->raised.yOffset = yOffset;
	this->raised.ySize = ySize;
}

void VoxelTraitsDefinition::initTransparentWall(bool collider)
{
	this->initGeneral(ArenaTypes::VoxelType::TransparentWall);
	this->transparentWall.collider = collider;
}

void VoxelTraitsDefinition::initEdge(VoxelFacing2D facing, bool collider)
{
	this->initGeneral(ArenaTypes::VoxelType::Edge);
	this->edge.facing = facing;
	this->edge.collider = collider;
}

void VoxelTraitsDefinition::initChasm(ArenaTypes::ChasmType chasmType)
{
	this->initGeneral(ArenaTypes::VoxelType::Chasm);
	this->chasm.type = chasmType;
}

bool VoxelTraitsDefinition::hasCollision() const
{
	switch (this->type)
	{
	case ArenaTypes::VoxelType::None:
		return false;
	case ArenaTypes::VoxelType::TransparentWall:
		return this->transparentWall.collider;
	case ArenaTypes::VoxelType::Edge:
		return this->edge.collider;
	default:
		return true;
	}
}
