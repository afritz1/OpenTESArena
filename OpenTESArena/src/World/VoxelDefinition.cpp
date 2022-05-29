#include "MapType.h"
#include "VoxelDefinition.h"
#include "VoxelFacing2D.h"

#include "components/debug/Debug.h"

void VoxelDefinition::WallData::init(TextureAsset &&sideTextureAsset,
	TextureAsset &&floorTextureAsset, TextureAsset &&ceilingTextureAsset)
{
	this->sideTextureAsset = std::move(sideTextureAsset);
	this->floorTextureAsset = std::move(floorTextureAsset);
	this->ceilingTextureAsset = std::move(ceilingTextureAsset);
}

void VoxelDefinition::FloorData::init(TextureAsset &&textureAsset, bool isWildWallColored)
{
	this->textureAsset = std::move(textureAsset);
	this->isWildWallColored = isWildWallColored;
}

void VoxelDefinition::CeilingData::init(TextureAsset &&textureAsset)
{
	this->textureAsset = std::move(textureAsset);
}

void VoxelDefinition::RaisedData::init(TextureAsset &&sideTextureAsset,
	TextureAsset &&floorTextureAsset, TextureAsset &&ceilingTextureAsset,
	double yOffset, double ySize, double vTop, double vBottom)
{
	this->sideTextureAsset = std::move(sideTextureAsset);
	this->floorTextureAsset = std::move(floorTextureAsset);
	this->ceilingTextureAsset = std::move(ceilingTextureAsset);
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->vTop = vTop;
	this->vBottom = vBottom;
}

void VoxelDefinition::DiagonalData::init(TextureAsset &&textureAsset, bool type1)
{
	this->textureAsset = std::move(textureAsset);
	this->type1 = type1;
}

void VoxelDefinition::TransparentWallData::init(TextureAsset &&textureAsset, bool collider)
{
	this->textureAsset = std::move(textureAsset);
	this->collider = collider;
}

void VoxelDefinition::EdgeData::init(TextureAsset &&textureAsset, double yOffset, bool collider,
	bool flipped, VoxelFacing2D facing)
{
	this->textureAsset = std::move(textureAsset);
	this->yOffset = yOffset;
	this->collider = collider;
	this->flipped = flipped;
	this->facing = facing;
}

void VoxelDefinition::ChasmData::init(TextureAsset &&textureAsset, ArenaTypes::ChasmType type)
{
	this->textureAsset = std::move(textureAsset);
	this->type = type;
}

bool VoxelDefinition::ChasmData::matches(const ChasmData &other) const
{
	return (this->textureAsset == other.textureAsset) && (this->type == other.type);
}

void VoxelDefinition::DoorData::init(TextureAsset &&textureAsset, ArenaTypes::DoorType type)
{
	this->textureAsset = std::move(textureAsset);
	this->type = type;
}

VoxelDefinition::VoxelDefinition()
{
	// Default to empty.
	this->type = ArenaTypes::VoxelType::None;
}

VoxelDefinition VoxelDefinition::makeWall(TextureAsset &&sideTextureAsset,
	TextureAsset &&floorTextureAsset, TextureAsset &&ceilingTextureAsset)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Wall;
	voxelDef.wall.init(std::move(sideTextureAsset), std::move(floorTextureAsset), 
		std::move(ceilingTextureAsset));
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeFloor(TextureAsset &&textureAsset, bool isWildWallColored)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Floor;
	voxelDef.floor.init(std::move(textureAsset), isWildWallColored);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeCeiling(TextureAsset &&textureAsset)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Ceiling;
	voxelDef.ceiling.init(std::move(textureAsset));
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeRaised(TextureAsset &&sideTextureAsset,
	TextureAsset &&floorTextureAsset, TextureAsset &&ceilingTextureAsset,
	double yOffset, double ySize, double vTop, double vBottom)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Raised;
	voxelDef.raised.init(std::move(sideTextureAsset), std::move(floorTextureAsset), 
		std::move(ceilingTextureAsset), yOffset, ySize, vTop, vBottom);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeDiagonal(TextureAsset &&textureAsset, bool type1)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Diagonal;
	voxelDef.diagonal.init(std::move(textureAsset), type1);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeTransparentWall(TextureAsset &&textureAsset, bool collider)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::TransparentWall;
	voxelDef.transparentWall.init(std::move(textureAsset), collider);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeEdge(TextureAsset &&textureAsset, double yOffset, bool collider,
	bool flipped, VoxelFacing2D facing)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Edge;
	voxelDef.edge.init(std::move(textureAsset), yOffset, collider, flipped, facing);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeChasm(TextureAsset &&textureAsset, ArenaTypes::ChasmType type)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Chasm;
	voxelDef.chasm.init(std::move(textureAsset), type);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeDoor(TextureAsset &&textureAsset, ArenaTypes::DoorType type)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Door;
	voxelDef.door.init(std::move(textureAsset), type);
	return voxelDef;
}

bool VoxelDefinition::allowsChasmFace() const
{
	return (this->type != ArenaTypes::VoxelType::None) && (this->type != ArenaTypes::VoxelType::Chasm);
}

int VoxelDefinition::getTextureAssetCount() const
{
	if (this->type == ArenaTypes::VoxelType::None)
	{
		return 0;
	}
	else if (this->type == ArenaTypes::VoxelType::Wall)
	{
		return 3;
	}
	else if (this->type == ArenaTypes::VoxelType::Floor)
	{
		return 1;
	}
	else if (this->type == ArenaTypes::VoxelType::Ceiling)
	{
		return 1;
	}
	else if (this->type == ArenaTypes::VoxelType::Raised)
	{
		return 3;
	}
	else if (this->type == ArenaTypes::VoxelType::Diagonal)
	{
		return 1;
	}
	else if (this->type == ArenaTypes::VoxelType::TransparentWall)
	{
		return 1;
	}
	else if (this->type == ArenaTypes::VoxelType::Edge)
	{
		return 1;
	}
	else if (this->type == ArenaTypes::VoxelType::Chasm)
	{
		return 1;
	}
	else if (this->type == ArenaTypes::VoxelType::Door)
	{
		return 1;
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(this->type)));
	}
}

const TextureAsset &VoxelDefinition::getTextureAsset(int index) const
{
	if (this->type == ArenaTypes::VoxelType::None)
	{
		throw DebugException("Can't get texture asset reference from None voxel definition.");
	}
	else if (this->type == ArenaTypes::VoxelType::Wall)
	{
		const std::array<const TextureAsset*, 3> ptrs =
		{
			&this->wall.sideTextureAsset,
			&this->wall.floorTextureAsset,
			&this->wall.ceilingTextureAsset
		};

		DebugAssertIndex(ptrs, index);
		return *ptrs[index];
	}
	else if (this->type == ArenaTypes::VoxelType::Floor)
	{
		DebugAssert(index == 0);
		return this->floor.textureAsset;
	}
	else if (this->type == ArenaTypes::VoxelType::Ceiling)
	{
		DebugAssert(index == 0);
		return this->ceiling.textureAsset;
	}
	else if (this->type == ArenaTypes::VoxelType::Raised)
	{
		const std::array<const TextureAsset*, 3> ptrs =
		{
			&this->raised.sideTextureAsset,
			&this->raised.floorTextureAsset,
			&this->raised.ceilingTextureAsset
		};

		DebugAssertIndex(ptrs, index);
		return *ptrs[index];
	}
	else if (this->type == ArenaTypes::VoxelType::Diagonal)
	{
		DebugAssert(index == 0);
		return this->diagonal.textureAsset;
	}
	else if (this->type == ArenaTypes::VoxelType::TransparentWall)
	{
		DebugAssert(index == 0);
		return this->transparentWall.textureAsset;
	}
	else if (this->type == ArenaTypes::VoxelType::Edge)
	{
		DebugAssert(index == 0);
		return this->edge.textureAsset;
	}
	else if (this->type == ArenaTypes::VoxelType::Chasm)
	{
		DebugAssert(index == 0);
		return this->chasm.textureAsset;
	}
	else if (this->type == ArenaTypes::VoxelType::Door)
	{
		DebugAssert(index == 0);
		return this->door.textureAsset;
	}
	else
	{
		throw DebugException("Unrecognized voxel type \"" + std::to_string(static_cast<int>(this->type)) + "\".");
	}
}
