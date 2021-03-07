#include <algorithm>

#include "Chunk.h"

#include "components/debug/Debug.h"

void Chunk::init(const ChunkInt2 &coord, int height)
{
	// Set all voxels to air and unused.
	this->voxels.init(Chunk::WIDTH, height, Chunk::DEPTH);
	this->voxels.fill(Chunk::AIR_VOXEL_ID);

	this->voxelDefs.fill(VoxelDefinition());
	this->activeVoxelDefs.fill(false);

	// Let the first voxel definition (air) be usable immediately. All default voxel IDs can safely
	// point to it.
	this->activeVoxelDefs.front() = true;

	this->coord = coord;
}

const ChunkInt2 &Chunk::getCoord() const
{
	return this->coord;
}

int Chunk::getHeight() const
{
	return this->voxels.getHeight();
}

Chunk::VoxelID Chunk::getVoxel(SNInt x, int y, WEInt z) const
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

VoxelInstance *Chunk::tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type)
{
	for (int i = 0; i < static_cast<int>(this->voxelInsts.size()); i++)
	{
		VoxelInstance &inst = this->voxelInsts[i];
		if ((inst.getX() == voxel.x) && (inst.getY() == voxel.y) && (inst.getZ() == voxel.z) &&
			(inst.getType() == type))
		{
			return &inst;
		}
	}

	return nullptr;
}

const VoxelInstance *Chunk::tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type) const
{
	for (int i = 0; i < static_cast<int>(this->voxelInsts.size()); i++)
	{
		const VoxelInstance &inst = this->voxelInsts[i];
		if ((inst.getX() == voxel.x) && (inst.getY() == voxel.y) && (inst.getZ() == voxel.z) &&
			(inst.getType() == type))
		{
			return &inst;
		}
	}

	return nullptr;
}

const TransitionDefinition *Chunk::tryGetTransition(const VoxelInt3 &voxel) const
{
	const auto iter = this->transitionDefIndices.find(voxel);
	if (iter != this->transitionDefIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->transitionDefs, index);
		return &this->transitionDefs[index];
	}
	else
	{
		return nullptr;
	}
}

const TriggerDefinition *Chunk::tryGetTrigger(const VoxelInt3 &voxel) const
{
	const auto iter = this->triggerDefIndices.find(voxel);
	if (iter != this->triggerDefIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->triggerDefs, index);
		return &this->triggerDefs[index];
	}
	else
	{
		return nullptr;
	}
}

const LockDefinition *Chunk::tryGetLock(const VoxelInt3 &voxel) const
{
	const auto iter = this->lockDefIndices.find(voxel);
	if (iter != this->lockDefIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->lockDefs, index);
		return &this->lockDefs[index];
	}
	else
	{
		return nullptr;
	}
}

const std::string *Chunk::tryGetBuildingName(const VoxelInt3 &voxel) const
{
	const auto iter = this->buildingNameIndices.find(voxel);
	if (iter != this->buildingNameIndices.end())
	{
		const int index = iter->second;
		DebugAssertIndex(this->buildingNames, index);
		return &this->buildingNames[index];
	}
	else
	{
		return nullptr;
	}
}

void Chunk::setVoxel(SNInt x, int y, WEInt z, VoxelID value)
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

void Chunk::addVoxelInst(VoxelInstance &&voxelInst)
{
	this->voxelInsts.emplace_back(std::move(voxelInst));
}

Chunk::TransitionID Chunk::addTransition(TransitionDefinition &&transition)
{
	const TransitionID id = static_cast<int>(this->transitionDefs.size());
	this->transitionDefs.emplace_back(std::move(transition));
	return id;
}

Chunk::TriggerID Chunk::addTrigger(TriggerDefinition &&trigger)
{
	const TriggerID id = static_cast<int>(this->triggerDefs.size());
	this->triggerDefs.emplace_back(std::move(trigger));
	return id;
}

Chunk::LockID Chunk::addLock(LockDefinition &&lock)
{
	const LockID id = static_cast<int>(this->lockDefs.size());
	this->lockDefs.emplace_back(std::move(lock));
	return id;
}

Chunk::BuildingNameID Chunk::addBuildingName(std::string &&buildingName)
{
	const BuildingNameID id = static_cast<int>(this->buildingNames.size());
	this->buildingNames.emplace_back(std::move(buildingName));
	return id;
}

void Chunk::addTransitionPosition(Chunk::TransitionID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->transitionDefIndices.find(voxel) == this->transitionDefIndices.end());
	this->transitionDefIndices.emplace(voxel, id);
}

void Chunk::addTriggerPosition(Chunk::TriggerID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->triggerDefIndices.find(voxel) == this->triggerDefIndices.end());
	this->triggerDefIndices.emplace(voxel, id);
}

void Chunk::addLockPosition(Chunk::LockID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->lockDefIndices.find(voxel) == this->lockDefIndices.end());
	this->lockDefIndices.emplace(voxel, id);
}

void Chunk::addBuildingNamePosition(Chunk::BuildingNameID id, const VoxelInt3 &voxel)
{
	DebugAssert(this->buildingNameIndices.find(voxel) == this->buildingNameIndices.end());
	this->buildingNameIndices.emplace(voxel, id);
}

void Chunk::removeVoxelDef(VoxelID id)
{
	DebugAssert(id < this->voxelDefs.size());
	this->voxelDefs[id] = VoxelDefinition();
	this->activeVoxelDefs[id] = false;
}

void Chunk::removeVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type)
{
	for (int i = 0; i < static_cast<int>(this->voxelInsts.size()); i++)
	{
		const VoxelInstance &voxelInst = this->voxelInsts[i];
		if ((voxelInst.getX() == voxel.x) && (voxelInst.getY() == voxel.y) &&
			(voxelInst.getZ() == voxel.z) && (voxelInst.getType() == type))
		{
			this->voxelInsts.erase(this->voxelInsts.begin() + i);
			break;
		}
	}
}

void Chunk::clear()
{
	this->voxels.clear();
	this->voxelDefs.fill(VoxelDefinition());
	this->activeVoxelDefs.fill(false);
	this->voxelInsts.clear();
	this->transitionDefs.clear();
	this->triggerDefs.clear();
	this->lockDefs.clear();
	this->buildingNames.clear();
	this->transitionDefIndices.clear();
	this->triggerDefIndices.clear();
	this->lockDefIndices.clear();
	this->buildingNameIndices.clear();
	this->coord = ChunkInt2();
}

void Chunk::update(double dt)
{
	for (int i = static_cast<int>(this->voxelInsts.size()) - 1; i >= 0; i--)
	{
		VoxelInstance &voxelInst = this->voxelInsts[i];
		voxelInst.update(dt);

		// @todo: handle doors that just closed
		// - delete voxel instance and conditionally play onClosed sound at center of voxel.
		DebugNotImplemented();

		// @todo: handle doors far enough from the player to close.
		// - ArenaLevelUtils::DOOR_CLOSE_DISTANCE
		// - if player is far enough, then set to closing and conditionally play onClosing sound at center of voxel.
		DebugNotImplemented();

		// See if the voxel instance can be removed because it no longer has interesting state.
		if (!voxelInst.hasRelevantState())
		{
			const VoxelInstance::Type voxelInstType = voxelInst.getType();
			
			// Do the voxel instance's "on destroy" action (if any).
			if (voxelInstType == VoxelInstance::Type::Fading)
			{
				// Convert the faded voxel to air or a chasm depending on the Y coordinate.
				const int voxelY = voxelInst.getY();
				if (voxelY == 0)
				{
					// Chasm voxel.
					// @todo: may need to store vector<Int3> of all faded voxels here so their adjacent
					// chasms can be updated properly in the next for loop. I say "may" because voxel
					// instances might be separate enough now from the old context-sensitive voxel
					// definitions. Need to re-evaluate it.
					DebugNotImplemented();
				}
				else
				{
					// Air voxel.
					this->setVoxel(voxelInst.getX(), voxelY, voxelInst.getZ(), Chunk::AIR_VOXEL_ID);
				}
			}

			this->voxelInsts.erase(this->voxelInsts.begin() + i);
		}
	}
}
