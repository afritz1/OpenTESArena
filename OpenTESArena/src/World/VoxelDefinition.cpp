#include "MapType.h"
#include "VoxelDefinition.h"
#include "VoxelFacing2D.h"

#include "components/debug/Debug.h"

void VoxelDefinition::WallData::init(TextureAssetReference &&sideTextureAssetRef,
	TextureAssetReference &&floorTextureAssetRef, TextureAssetReference &&ceilingTextureAssetRef)
{
	this->sideTextureAssetRef = std::move(sideTextureAssetRef);
	this->floorTextureAssetRef = std::move(floorTextureAssetRef);
	this->ceilingTextureAssetRef = std::move(ceilingTextureAssetRef);
}

void VoxelDefinition::FloorData::init(TextureAssetReference &&textureAssetRef, bool isWildWallColored)
{
	this->textureAssetRef = std::move(textureAssetRef);
	this->isWildWallColored = isWildWallColored;
}

void VoxelDefinition::CeilingData::init(TextureAssetReference &&textureAssetRef)
{
	this->textureAssetRef = std::move(textureAssetRef);
}

void VoxelDefinition::RaisedData::init(TextureAssetReference &&sideTextureAssetRef,
	TextureAssetReference &&floorTextureAssetRef, TextureAssetReference &&ceilingTextureAssetRef,
	double yOffset, double ySize, double vTop, double vBottom)
{
	this->sideTextureAssetRef = std::move(sideTextureAssetRef);
	this->floorTextureAssetRef = std::move(floorTextureAssetRef);
	this->ceilingTextureAssetRef = std::move(ceilingTextureAssetRef);
	this->yOffset = yOffset;
	this->ySize = ySize;
	this->vTop = vTop;
	this->vBottom = vBottom;
}

void VoxelDefinition::DiagonalData::init(TextureAssetReference &&textureAssetRef, bool type1)
{
	this->textureAssetRef = std::move(textureAssetRef);
	this->type1 = type1;
}

void VoxelDefinition::TransparentWallData::init(TextureAssetReference &&textureAssetRef, bool collider)
{
	this->textureAssetRef = std::move(textureAssetRef);
	this->collider = collider;
}

void VoxelDefinition::EdgeData::init(TextureAssetReference &&textureAssetRef, double yOffset, bool collider,
	bool flipped, VoxelFacing2D facing)
{
	this->textureAssetRef = std::move(textureAssetRef);
	this->yOffset = yOffset;
	this->collider = collider;
	this->flipped = flipped;
	this->facing = facing;
}

void VoxelDefinition::ChasmData::init(TextureAssetReference &&textureAssetRef, ArenaTypes::ChasmType type)
{
	this->textureAssetRef = std::move(textureAssetRef);
	this->type = type;
}

bool VoxelDefinition::ChasmData::matches(const ChasmData &other) const
{
	return (this->textureAssetRef == other.textureAssetRef) && (this->type == other.type);
}

void VoxelDefinition::DoorData::init(TextureAssetReference &&textureAssetRef, ArenaTypes::DoorType type)
{
	this->textureAssetRef = std::move(textureAssetRef);
	this->type = type;
}

VoxelDefinition::VoxelDefinition()
{
	// Default to empty.
	this->type = ArenaTypes::VoxelType::None;
}

VoxelDefinition VoxelDefinition::makeWall(TextureAssetReference &&sideTextureAssetRef,
	TextureAssetReference &&floorTextureAssetRef, TextureAssetReference &&ceilingTextureAssetRef)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Wall;
	voxelDef.wall.init(std::move(sideTextureAssetRef), std::move(floorTextureAssetRef), 
		std::move(ceilingTextureAssetRef));
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeFloor(TextureAssetReference &&textureAssetRef, bool isWildWallColored)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Floor;
	voxelDef.floor.init(std::move(textureAssetRef), isWildWallColored);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeCeiling(TextureAssetReference &&textureAssetRef)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Ceiling;
	voxelDef.ceiling.init(std::move(textureAssetRef));
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeRaised(TextureAssetReference &&sideTextureAssetRef,
	TextureAssetReference &&floorTextureAssetRef, TextureAssetReference &&ceilingTextureAssetRef,
	double yOffset, double ySize, double vTop, double vBottom)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Raised;
	voxelDef.raised.init(std::move(sideTextureAssetRef), std::move(floorTextureAssetRef), 
		std::move(ceilingTextureAssetRef), yOffset, ySize, vTop, vBottom);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeDiagonal(TextureAssetReference &&textureAssetRef, bool type1)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Diagonal;
	voxelDef.diagonal.init(std::move(textureAssetRef), type1);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeTransparentWall(TextureAssetReference &&textureAssetRef, bool collider)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::TransparentWall;
	voxelDef.transparentWall.init(std::move(textureAssetRef), collider);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeEdge(TextureAssetReference &&textureAssetRef, double yOffset, bool collider,
	bool flipped, VoxelFacing2D facing)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Edge;
	voxelDef.edge.init(std::move(textureAssetRef), yOffset, collider, flipped, facing);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeChasm(TextureAssetReference &&textureAssetRef, ArenaTypes::ChasmType type)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Chasm;
	voxelDef.chasm.init(std::move(textureAssetRef), type);
	return voxelDef;
}

VoxelDefinition VoxelDefinition::makeDoor(TextureAssetReference &&textureAssetRef, ArenaTypes::DoorType type)
{
	VoxelDefinition voxelDef;
	voxelDef.type = ArenaTypes::VoxelType::Door;
	voxelDef.door.init(std::move(textureAssetRef), type);
	return voxelDef;
}

bool VoxelDefinition::allowsChasmFace() const
{
	return (this->type != ArenaTypes::VoxelType::None) && (this->type != ArenaTypes::VoxelType::Chasm);
}

int VoxelDefinition::getTextureAssetReferenceCount() const
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

const TextureAssetReference &VoxelDefinition::getTextureAssetReference(int index) const
{
	if (this->type == ArenaTypes::VoxelType::None)
	{
		throw DebugException("Can't get texture asset reference from None voxel definition.");
	}
	else if (this->type == ArenaTypes::VoxelType::Wall)
	{
		const std::array<const TextureAssetReference*, 3> ptrs =
		{
			&this->wall.sideTextureAssetRef,
			&this->wall.floorTextureAssetRef,
			&this->wall.ceilingTextureAssetRef
		};

		DebugAssertIndex(ptrs, index);
		return *ptrs[index];
	}
	else if (this->type == ArenaTypes::VoxelType::Floor)
	{
		DebugAssert(index == 0);
		return this->floor.textureAssetRef;
	}
	else if (this->type == ArenaTypes::VoxelType::Ceiling)
	{
		DebugAssert(index == 0);
		return this->ceiling.textureAssetRef;
	}
	else if (this->type == ArenaTypes::VoxelType::Raised)
	{
		const std::array<const TextureAssetReference*, 3> ptrs =
		{
			&this->raised.sideTextureAssetRef,
			&this->raised.floorTextureAssetRef,
			&this->raised.ceilingTextureAssetRef
		};

		DebugAssertIndex(ptrs, index);
		return *ptrs[index];
	}
	else if (this->type == ArenaTypes::VoxelType::Diagonal)
	{
		DebugAssert(index == 0);
		return this->diagonal.textureAssetRef;
	}
	else if (this->type == ArenaTypes::VoxelType::TransparentWall)
	{
		DebugAssert(index == 0);
		return this->transparentWall.textureAssetRef;
	}
	else if (this->type == ArenaTypes::VoxelType::Edge)
	{
		DebugAssert(index == 0);
		return this->edge.textureAssetRef;
	}
	else if (this->type == ArenaTypes::VoxelType::Chasm)
	{
		DebugAssert(index == 0);
		return this->chasm.textureAssetRef;
	}
	else if (this->type == ArenaTypes::VoxelType::Door)
	{
		DebugAssert(index == 0);
		return this->door.textureAssetRef;
	}
	else
	{
		throw DebugException("Unrecognized voxel type \"" + std::to_string(static_cast<int>(this->type)) + "\".");
	}
}
