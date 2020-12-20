#include <algorithm>

#include "Chunk.h"

#include "components/debug/Debug.h"

void Chunk::init(const ChunkInt2 &coord, int height)
{
	// Set all voxels to air and unused.
	this->voxels.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->voxels.fill(0);

	this->voxelDefs.fill(VoxelDefinition());
	this->activeVoxelDefs.fill(false);

	// Let the first voxel data (air) be usable immediately. All default voxel IDs can safely point to it.
	this->activeVoxelDefs.front() = true;

	this->coord = coord;
}

const ChunkInt2 &Chunk::getCoord() const
{
	return this->coord;
}

constexpr int Chunk::getWidth() const
{
	return Chunk::WIDTH;
}

int Chunk::getHeight() const
{
	return this->voxels.getHeight();
}

constexpr int Chunk::getDepth() const
{
	return Chunk::DEPTH;
}

Chunk::VoxelID Chunk::get(int x, int y, int z) const
{
	return this->voxels.get(x, y, z);
}

int Chunk::getVoxelDefCount() const
{
	return static_cast<int>(std::count(this->activeVoxelDefs.begin(),
		this->activeVoxelDefs.end(), true));
}

const VoxelDefinition &Chunk::getVoxelDef(VoxelID id) const
{
	DebugAssert(id < this->voxelDefs.size());
	DebugAssert(this->activeVoxelDefs[id]);
	return this->voxelDefs[id];
}

int Chunk::getVoxelInstCount() const
{
	return static_cast<int>(this->voxelInsts.size());
}

VoxelInstance &Chunk::getVoxelInst(int index)
{
	DebugAssertIndex(this->voxelInsts, index);
	return this->voxelInsts[index];
}

const VoxelInstance &Chunk::getVoxelInst(int index) const
{
	DebugAssertIndex(this->voxelInsts, index);
	return this->voxelInsts[index];
}

void Chunk::setCoord(const ChunkInt2 &coord)
{
	this->coord = coord;
}

void Chunk::set(int x, int y, int z, VoxelID value)
{
	this->voxels.set(x, y, z, value);
}

bool Chunk::tryAddVoxelDef(VoxelDefinition &&voxelDef, Chunk::VoxelID *outID)
{
	// Find a place to add the voxel data.
	const auto iter = std::find(this->activeVoxelDefs.begin(), this->activeVoxelDefs.end(), false);

	// If this is ever true, we need more bits per voxel.
	if (iter == this->activeVoxelDefs.end())
	{
		return false;
	}

	const VoxelID id = static_cast<VoxelID>(std::distance(this->activeVoxelDefs.begin(), iter));
	this->voxelDefs[id] = std::move(voxelDef);
	this->activeVoxelDefs[id] = true;
	*outID = id;
	return true;
}

void Chunk::removeVoxelDef(VoxelID id)
{
	DebugAssert(id < this->voxelDefs.size());
	this->voxelDefs[id] = VoxelDefinition();
	this->activeVoxelDefs[id] = false;
}

void Chunk::clear()
{
	this->voxels.clear();
	this->voxelDefs.fill(VoxelDefinition());
	this->activeVoxelDefs.fill(false);
	this->voxelInsts.clear();
	this->coord = ChunkInt2();
}

void Chunk::update(double dt)
{
	for (int i = static_cast<int>(this->voxelInsts.size()) - 1; i >= 0; i--)
	{
		VoxelInstance &voxelInst = this->voxelInsts[i];
		voxelInst.update(dt);

		const VoxelInstance::Type voxelInstType = voxelInst.getType();
		if (voxelInstType == VoxelInstance::Type::Chasm)
		{
			// Do nothing.
		}
		else if (voxelInstType == VoxelInstance::Type::OpenDoor)
		{
			// @todo: check door state. See LevelData::DoorState::update() for reference.
			DebugNotImplemented();
		}
		else if (voxelInstType == VoxelInstance::Type::Fading)
		{
			// @todo: delete faded voxels. See LevelData::updateFadingVoxels() for reference.
			DebugNotImplemented();
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(voxelInstType)));
		}
	}
}
