#include <algorithm>

#include "LevelDefinition.h"

LevelDefinition::EntityPlacementDef::EntityPlacementDef(EntityDefID id, std::vector<WorldDouble3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::LockPlacementDef::LockPlacementDef(LockDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::TriggerPlacementDef::TriggerPlacementDef(TriggerDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::TransitionPlacementDef::TransitionPlacementDef(TransitionDefID id,
	std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::BuildingNamePlacementDef::BuildingNamePlacementDef(BuildingNameID id,
	std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::DoorPlacementDef::DoorPlacementDef(DoorDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::ChasmPlacementDef::ChasmPlacementDef(ChasmDefID id, std::vector<WorldInt3>&& positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::LevelDefinition()
{
	this->floorReplacementMeshDefID = -1;
	this->floorReplacementTextureDefID = -1;
	this->floorReplacementTraitsDefID = -1;
	this->floorReplacementChasmDefID = -1;
}

void LevelDefinition::init(SNInt width, int height, WEInt depth)
{
	this->voxelMeshIDs.init(width, height, depth);
	this->voxelMeshIDs.fill(0);

	this->voxelTextureIDs.init(width, height, depth);
	this->voxelTextureIDs.fill(0);

	this->voxelTraitsIDs.init(width, height, depth);
	this->voxelTraitsIDs.fill(0);
}

SNInt LevelDefinition::getWidth() const
{
	return this->voxelMeshIDs.getWidth();
}

int LevelDefinition::getHeight() const
{
	return this->voxelMeshIDs.getHeight();
}

WEInt LevelDefinition::getDepth() const
{
	return this->voxelMeshIDs.getDepth();
}

LevelDefinition::VoxelMeshDefID LevelDefinition::getVoxelMeshID(SNInt x, int y, WEInt z) const
{
	return this->voxelMeshIDs.get(x, y, z);
}

LevelDefinition::VoxelTextureDefID LevelDefinition::getVoxelTextureID(SNInt x, int y, WEInt z) const
{
	return this->voxelTextureIDs.get(x, y, z);
}

LevelDefinition::VoxelTraitsDefID LevelDefinition::getVoxelTraitsID(SNInt x, int y, WEInt z) const
{
	return this->voxelTraitsIDs.get(x, y, z);
}

void LevelDefinition::setVoxelMeshID(SNInt x, int y, WEInt z, VoxelMeshDefID id)
{
	this->voxelMeshIDs.set(x, y, z, id);
}

void LevelDefinition::setVoxelTextureID(SNInt x, int y, WEInt z, VoxelTextureDefID id)
{
	this->voxelTextureIDs.set(x, y, z, id);
}

void LevelDefinition::setVoxelTraitsID(SNInt x, int y, WEInt z, VoxelTraitsDefID id)
{
	this->voxelTraitsIDs.set(x, y, z, id);
}

LevelDefinition::VoxelMeshDefID LevelDefinition::getFloorReplacementMeshDefID() const
{
	return this->floorReplacementMeshDefID;
}

LevelDefinition::VoxelTextureDefID LevelDefinition::getFloorReplacementTextureDefID() const
{
	return this->floorReplacementTextureDefID;
}

LevelDefinition::VoxelTraitsDefID LevelDefinition::getFloorReplacementTraitsDefID() const
{
	return this->floorReplacementTraitsDefID;
}

LevelDefinition::ChasmDefID LevelDefinition::getFloorReplacementChasmDefID() const
{
	return this->floorReplacementChasmDefID;
}

void LevelDefinition::setFloorReplacementMeshDefID(VoxelMeshDefID id)
{
	this->floorReplacementMeshDefID = id;
}

void LevelDefinition::setFloorReplacementTextureDefID(VoxelTextureDefID id)
{
	this->floorReplacementTextureDefID = id;
}

void LevelDefinition::setFloorReplacementTraitsDefID(VoxelTraitsDefID id)
{
	this->floorReplacementTraitsDefID = id;
}

void LevelDefinition::setFloorReplacementChasmDefID(ChasmDefID id)
{
	this->floorReplacementChasmDefID = id;
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

int LevelDefinition::getDoorPlacementDefCount() const
{
	return static_cast<int>(this->doorPlacementDefs.size());
}

const LevelDefinition::DoorPlacementDef &LevelDefinition::getDoorPlacementDef(int index) const
{
	DebugAssertIndex(this->doorPlacementDefs, index);
	return this->doorPlacementDefs[index];
}

int LevelDefinition::getChasmPlacementDefCount() const
{
	return static_cast<int>(this->chasmPlacementDefs.size());
}

const LevelDefinition::ChasmPlacementDef &LevelDefinition::getChasmPlacementDef(int index) const
{
	DebugAssertIndex(this->chasmPlacementDefs, index);
	return this->chasmPlacementDefs[index];
}

void LevelDefinition::addEntity(EntityDefID id, const WorldDouble3 &position)
{
	const auto iter = std::find_if(this->entityPlacementDefs.begin(), this->entityPlacementDefs.end(),
		[id](const EntityPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->entityPlacementDefs.end())
	{
		std::vector<WorldDouble3> &positions = iter->positions;
		positions.emplace_back(position);
	}
	else
	{
		this->entityPlacementDefs.emplace_back(id, std::vector<WorldDouble3> { position });
	}
}

void LevelDefinition::addLock(LockDefID id, const WorldInt3 &position)
{
	const auto iter = std::find_if(this->lockPlacementDefs.begin(), this->lockPlacementDefs.end(),
		[id](const LockPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->lockPlacementDefs.end())
	{
		std::vector<WorldInt3> &positions = iter->positions;
		positions.emplace_back(position);
	}
	else
	{
		this->lockPlacementDefs.emplace_back(id, std::vector<WorldInt3> { position });
	}
}

void LevelDefinition::addTrigger(TriggerDefID id, const WorldInt3 &position)
{
	const auto iter = std::find_if(this->triggerPlacementDefs.begin(), this->triggerPlacementDefs.end(),
		[id](const TriggerPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->triggerPlacementDefs.end())
	{
		std::vector<WorldInt3> &positions = iter->positions;
		positions.emplace_back(position);
	}
	else
	{
		this->triggerPlacementDefs.emplace_back(id, std::vector<WorldInt3> { position });
	}
}

void LevelDefinition::addTransition(TransitionDefID id, const WorldInt3 &position)
{
	const auto iter = std::find_if(this->transitionPlacementDefs.begin(), this->transitionPlacementDefs.end(),
		[id](const TransitionPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->transitionPlacementDefs.end())
	{
		std::vector<WorldInt3> &positions = iter->positions;
		positions.emplace_back(position);
	}
	else
	{
		this->transitionPlacementDefs.emplace_back(id, std::vector<WorldInt3> { position });
	}
}

void LevelDefinition::addBuildingName(BuildingNameID id, const WorldInt3 &position)
{
	const auto iter = std::find_if(this->buildingNamePlacementDefs.begin(),
		this->buildingNamePlacementDefs.end(), [id](const BuildingNamePlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->buildingNamePlacementDefs.end())
	{
		std::vector<WorldInt3> &positions = iter->positions;
		positions.emplace_back(position);
	}
	else
	{
		this->buildingNamePlacementDefs.emplace_back(id, std::vector<WorldInt3> { position });
	}
}

void LevelDefinition::addDoor(DoorDefID id, const WorldInt3 &position)
{
	const auto iter = std::find_if(this->doorPlacementDefs.begin(),
		this->doorPlacementDefs.end(), [id](const DoorPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->doorPlacementDefs.end())
	{
		std::vector<WorldInt3> &positions = iter->positions;
		positions.emplace_back(position);
	}
	else
	{
		this->doorPlacementDefs.emplace_back(id, std::vector<WorldInt3> { position });
	}
}

void LevelDefinition::addChasm(ChasmDefID id, const WorldInt3 &position)
{
	const auto iter = std::find_if(this->chasmPlacementDefs.begin(),
		this->chasmPlacementDefs.end(), [id](const ChasmPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->chasmPlacementDefs.end())
	{
		std::vector<WorldInt3> &positions = iter->positions;
		positions.emplace_back(position);
	}
	else
	{
		this->chasmPlacementDefs.emplace_back(id, std::vector<WorldInt3> { position });
	}
}
