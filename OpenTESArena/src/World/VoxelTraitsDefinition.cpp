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

void VoxelTraitsDefinition::initTransparentWall(bool collider)
{
	this->initGeneral(ArenaTypes::VoxelType::TransparentWall);
	this->transparentWall.collider = collider;
}

void VoxelTraitsDefinition::initEdge(bool collider)
{
	this->initGeneral(ArenaTypes::VoxelType::Edge);
	this->edge.collider = collider;
}

void VoxelTraitsDefinition::initChasm(ArenaTypes::ChasmType chasmType)
{
	this->initGeneral(ArenaTypes::VoxelType::Chasm);
	this->chasm.type = chasmType;
}
