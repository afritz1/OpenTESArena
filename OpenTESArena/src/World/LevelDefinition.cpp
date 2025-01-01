#include <algorithm>

#include "LevelDefinition.h"

LevelDefinition::EntityPlacementDef::EntityPlacementDef(LevelVoxelEntityDefID id, std::vector<WorldDouble3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::LockPlacementDef::LockPlacementDef(LevelVoxelLockDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::TriggerPlacementDef::TriggerPlacementDef(LevelVoxelTriggerDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::TransitionPlacementDef::TransitionPlacementDef(LevelVoxelTransitionDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::BuildingNamePlacementDef::BuildingNamePlacementDef(LevelVoxelBuildingNameID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::DoorPlacementDef::DoorPlacementDef(LevelVoxelDoorDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::ChasmPlacementDef::ChasmPlacementDef(LevelVoxelChasmDefID id, std::vector<WorldInt3>&& positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDefinition::LevelDefinition()
{
	this->floorReplacementShapeDefID = -1;
	this->floorReplacementTextureDefID = -1;
	this->floorReplacementTraitsDefID = -1;
	this->floorReplacementChasmDefID = -1;
}

void LevelDefinition::init(SNInt width, int height, WEInt depth)
{
	this->voxelShapeIDs.init(width, height, depth);
	this->voxelShapeIDs.fill(0);

	this->voxelTextureIDs.init(width, height, depth);
	this->voxelTextureIDs.fill(0);

	this->voxelTraitsIDs.init(width, height, depth);
	this->voxelTraitsIDs.fill(0);
}

SNInt LevelDefinition::getWidth() const
{
	return this->voxelShapeIDs.getWidth();
}

int LevelDefinition::getHeight() const
{
	return this->voxelShapeIDs.getHeight();
}

WEInt LevelDefinition::getDepth() const
{
	return this->voxelShapeIDs.getDepth();
}

LevelVoxelShapeDefID LevelDefinition::getVoxelShapeID(SNInt x, int y, WEInt z) const
{
	return this->voxelShapeIDs.get(x, y, z);
}

LevelVoxelTextureDefID LevelDefinition::getVoxelTextureID(SNInt x, int y, WEInt z) const
{
	return this->voxelTextureIDs.get(x, y, z);
}

LevelVoxelTraitsDefID LevelDefinition::getVoxelTraitsID(SNInt x, int y, WEInt z) const
{
	return this->voxelTraitsIDs.get(x, y, z);
}

void LevelDefinition::setVoxelShapeID(SNInt x, int y, WEInt z, LevelVoxelShapeDefID id)
{
	this->voxelShapeIDs.set(x, y, z, id);
}

void LevelDefinition::setVoxelTextureID(SNInt x, int y, WEInt z, LevelVoxelTextureDefID id)
{
	this->voxelTextureIDs.set(x, y, z, id);
}

void LevelDefinition::setVoxelTraitsID(SNInt x, int y, WEInt z, LevelVoxelTraitsDefID id)
{
	this->voxelTraitsIDs.set(x, y, z, id);
}

LevelVoxelShapeDefID LevelDefinition::getFloorReplacementShapeDefID() const
{
	return this->floorReplacementShapeDefID;
}

LevelVoxelTextureDefID LevelDefinition::getFloorReplacementTextureDefID() const
{
	return this->floorReplacementTextureDefID;
}

LevelVoxelTraitsDefID LevelDefinition::getFloorReplacementTraitsDefID() const
{
	return this->floorReplacementTraitsDefID;
}

LevelVoxelChasmDefID LevelDefinition::getFloorReplacementChasmDefID() const
{
	return this->floorReplacementChasmDefID;
}

void LevelDefinition::setFloorReplacementShapeDefID(LevelVoxelShapeDefID id)
{
	this->floorReplacementShapeDefID = id;
}

void LevelDefinition::setFloorReplacementTextureDefID(LevelVoxelTextureDefID id)
{
	this->floorReplacementTextureDefID = id;
}

void LevelDefinition::setFloorReplacementTraitsDefID(LevelVoxelTraitsDefID id)
{
	this->floorReplacementTraitsDefID = id;
}

void LevelDefinition::setFloorReplacementChasmDefID(LevelVoxelChasmDefID id)
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

void LevelDefinition::addEntity(LevelVoxelEntityDefID id, const WorldDouble3 &position)
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

void LevelDefinition::addLock(LevelVoxelLockDefID id, const WorldInt3 &position)
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

void LevelDefinition::addTrigger(LevelVoxelTriggerDefID id, const WorldInt3 &position)
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

void LevelDefinition::addTransition(LevelVoxelTransitionDefID id, const WorldInt3 &position)
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

void LevelDefinition::addBuildingName(LevelVoxelBuildingNameID id, const WorldInt3 &position)
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

void LevelDefinition::addDoor(LevelVoxelDoorDefID id, const WorldInt3 &position)
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

void LevelDefinition::addChasm(LevelVoxelChasmDefID id, const WorldInt3 &position)
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
