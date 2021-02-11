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

Buffer<TextureAssetReference> VoxelDefinition::getTextureAssetReferences() const
{
	Buffer<TextureAssetReference> buffer;

	if (this->type == ArenaTypes::VoxelType::None)
	{
		// Do nothing.
	}
	else if (this->type == ArenaTypes::VoxelType::Wall)
	{
		buffer.init(3);
		buffer.set(0, this->wall.sideTextureAssetRef);
		buffer.set(1, this->wall.floorTextureAssetRef);
		buffer.set(2, this->wall.ceilingTextureAssetRef);
	}
	else if (this->type == ArenaTypes::VoxelType::Floor)
	{
		buffer.init(1);
		buffer.set(0, this->floor.textureAssetRef);
	}
	else if (this->type == ArenaTypes::VoxelType::Ceiling)
	{
		buffer.init(1);
		buffer.set(0, this->ceiling.textureAssetRef);
	}
	else if (this->type == ArenaTypes::VoxelType::Raised)
	{
		buffer.init(3);
		buffer.set(0, this->raised.sideTextureAssetRef);
		buffer.set(1, this->raised.floorTextureAssetRef);
		buffer.set(2, this->raised.ceilingTextureAssetRef);
	}
	else if (this->type == ArenaTypes::VoxelType::Diagonal)
	{
		buffer.init(1);
		buffer.set(0, this->diagonal.textureAssetRef);
	}
	else if (this->type == ArenaTypes::VoxelType::TransparentWall)
	{
		buffer.init(1);
		buffer.set(0, this->transparentWall.textureAssetRef);
	}
	else if (this->type == ArenaTypes::VoxelType::Edge)
	{
		buffer.init(1);
		buffer.set(0, this->edge.textureAssetRef);
	}
	else if (this->type == ArenaTypes::VoxelType::Chasm)
	{
		buffer.init(1);
		buffer.set(0, this->chasm.textureAssetRef);
	}
	else if (this->type == ArenaTypes::VoxelType::Door)
	{
		buffer.init(1);
		buffer.set(0, this->door.textureAssetRef);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(this->type)));
	}

	return buffer;
}
