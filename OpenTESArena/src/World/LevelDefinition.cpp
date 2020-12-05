#include <algorithm>

#include "LevelDefinition.h"

LevelDefinition::EntityPlacementDef::EntityPlacementDef(EntityDefID id, std::vector<LevelDouble3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::LockPlacementDef::LockPlacementDef(LockDefID id, std::vector<LevelInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::TriggerPlacementDef::TriggerPlacementDef(TriggerDefID id, std::vector<LevelInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::TransitionPlacementDef::TransitionPlacementDef(TransitionDefID id,
	std::vector<LevelInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::BuildingNamePlacementDef::BuildingNamePlacementDef(BuildingNameID id,
	std::vector<LevelInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

void LevelDefinition::init(SNInt width, int height, WEInt depth)
{
	this->voxels.init(width, height, depth);
	this->voxels.fill(0);
}

SNInt LevelDefinition::getWidth() const
{
	return this->voxels.getWidth();
}

int LevelDefinition::getHeight() const
{
	return this->voxels.getHeight();
}

WEInt LevelDefinition::getDepth() const
{
	return this->voxels.getDepth();
}

LevelDefinition::VoxelDefID LevelDefinition::getVoxel(SNInt x, int y, WEInt z) const
{
	return this->voxels.get(x, y, z);
}

void LevelDefinition::setVoxel(SNInt x, int y, WEInt z, VoxelDefID voxel)
{
	this->voxels.set(x, y, z, voxel);
}

int LevelDefinition::getEntityPlacementDefCount() const
{
	return static_cast<int>(this->entityPlacementDefs.size());
}

const LevelDefinition::EntityPlacementDef &LevelDefinition::getEntityPlacementDef(int index) const
{
	DebugAssertIndex(this->entityPlacementDefs, index);
	return this->entityPlacementDefs[index];
}

int LevelDefinition::getLockPlacementDefCount() const
{
	return static_cast<int>(this->lockPlacementDefs.size());
}

const LevelDefinition::LockPlacementDef &LevelDefinition::getLockPlacementDef(int index) const
{
	DebugAssertIndex(this->lockPlacementDefs, index);
	return this->lockPlacementDefs[index];
}

int LevelDefinition::getTriggerPlacementDefCount() const
{
	return static_cast<int>(this->triggerPlacementDefs.size());
}

const LevelDefinition::TriggerPlacementDef &LevelDefinition::getTriggerPlacementDef(int index) const
{
	DebugAssertIndex(this->triggerPlacementDefs, index);
	return this->triggerPlacementDefs[index];
}

int LevelDefinition::getTransitionPlacementDefCount() const
{
	return static_cast<int>(this->transitionPlacementDefs.size());
}

const LevelDefinition::TransitionPlacementDef &LevelDefinition::getTransitionPlacementDef(int index) const
{
	DebugAssertIndex(this->transitionPlacementDefs, index);
	return this->transitionPlacementDefs[index];
}

int LevelDefinition::getBuildingNamePlacementDefCount() const
{
	return static_cast<int>(this->buildingNamePlacementDefs.size());
}

const LevelDefinition::BuildingNamePlacementDef &LevelDefinition::getBuildingNamePlacementDef(int index) const
{
	DebugAssertIndex(this->buildingNamePlacementDefs, index);
	return this->buildingNamePlacementDefs[index];
}

void LevelDefinition::addEntity(EntityDefID id, const LevelDouble3 &position)
{
	const auto iter = std::find_if(this->entityPlacementDefs.begin(), this->entityPlacementDefs.end(),
		[id](const EntityPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->entityPlacementDefs.end())
	{
		std::vector<LevelDouble3> &positions = iter->positions;
		positions.push_back(position);
	}
	else
	{
		this->entityPlacementDefs.emplace_back(id, std::vector<LevelDouble3> { position });
	}
}

void LevelDefinition::addLock(LockDefID id, const LevelInt3 &position)
{
	const auto iter = std::find_if(this->lockPlacementDefs.begin(), this->lockPlacementDefs.end(),
		[id](const LockPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->lockPlacementDefs.end())
	{
		std::vector<LevelInt3> &positions = iter->positions;
		positions.push_back(position);
	}
	else
	{
		this->lockPlacementDefs.emplace_back(id, std::vector<LevelInt3> { position });
	}
}

void LevelDefinition::addTrigger(TriggerDefID id, const LevelInt3 &position)
{
	const auto iter = std::find_if(this->triggerPlacementDefs.begin(), this->triggerPlacementDefs.end(),
		[id](const TriggerPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->triggerPlacementDefs.end())
	{
		std::vector<LevelInt3> &positions = iter->positions;
		positions.push_back(position);
	}
	else
	{
		this->triggerPlacementDefs.emplace_back(id, std::vector<LevelInt3> { position });
	}
}

void LevelDefinition::addTransition(TransitionDefID id, const LevelInt3 &position)
{
	const auto iter = std::find_if(this->transitionPlacementDefs.begin(), this->transitionPlacementDefs.end(),
		[id](const TransitionPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->transitionPlacementDefs.end())
	{
		std::vector<LevelInt3> &positions = iter->positions;
		positions.push_back(position);
	}
	else
	{
		this->transitionPlacementDefs.emplace_back(id, std::vector<LevelInt3> { position });
	}
}

void LevelDefinition::addBuildingName(BuildingNameID id, const LevelInt3 &position)
{
	const auto iter = std::find_if(this->buildingNamePlacementDefs.begin(),
		this->buildingNamePlacementDefs.end(), [id](const BuildingNamePlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->buildingNamePlacementDefs.end())
	{
		std::vector<LevelInt3> &positions = iter->positions;
		positions.push_back(position);
	}
	else
	{
		this->buildingNamePlacementDefs.emplace_back(id, std::vector<LevelInt3> { position });
	}
}
