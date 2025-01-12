#include <algorithm>

#include "LevelDefinition.h"

LevelEntityPlacementDefinition::LevelEntityPlacementDefinition(LevelVoxelEntityDefID id, std::vector<WorldDouble2> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelLockPlacementDefinition::LevelLockPlacementDefinition(LevelVoxelLockDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelTriggerPlacementDefinition::LevelTriggerPlacementDefinition(LevelVoxelTriggerDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelTransitionPlacementDefinition::LevelTransitionPlacementDefinition(LevelVoxelTransitionDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelBuildingNamePlacementDefinition::LevelBuildingNamePlacementDefinition(LevelVoxelBuildingNameID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelDoorPlacementDefinition::LevelDoorPlacementDefinition(LevelVoxelDoorDefID id, std::vector<WorldInt3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

LevelChasmPlacementDefinition::LevelChasmPlacementDefinition(LevelVoxelChasmDefID id, std::vector<WorldInt3>&& positions)
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

const LevelEntityPlacementDefinition &LevelDefinition::getEntityPlacementDef(int index) const
{
	DebugAssertIndex(this->entityPlacementDefs, index);
	return this->entityPlacementDefs[index];
}

int LevelDefinition::getLockPlacementDefCount() const
{
	return static_cast<int>(this->lockPlacementDefs.size());
}

const LevelLockPlacementDefinition &LevelDefinition::getLockPlacementDef(int index) const
{
	DebugAssertIndex(this->lockPlacementDefs, index);
	return this->lockPlacementDefs[index];
}

int LevelDefinition::getTriggerPlacementDefCount() const
{
	return static_cast<int>(this->triggerPlacementDefs.size());
}

const LevelTriggerPlacementDefinition &LevelDefinition::getTriggerPlacementDef(int index) const
{
	DebugAssertIndex(this->triggerPlacementDefs, index);
	return this->triggerPlacementDefs[index];
}

int LevelDefinition::getTransitionPlacementDefCount() const
{
	return static_cast<int>(this->transitionPlacementDefs.size());
}

const LevelTransitionPlacementDefinition &LevelDefinition::getTransitionPlacementDef(int index) const
{
	DebugAssertIndex(this->transitionPlacementDefs, index);
	return this->transitionPlacementDefs[index];
}

int LevelDefinition::getBuildingNamePlacementDefCount() const
{
	return static_cast<int>(this->buildingNamePlacementDefs.size());
}

const LevelBuildingNamePlacementDefinition &LevelDefinition::getBuildingNamePlacementDef(int index) const
{
	DebugAssertIndex(this->buildingNamePlacementDefs, index);
	return this->buildingNamePlacementDefs[index];
}

int LevelDefinition::getDoorPlacementDefCount() const
{
	return static_cast<int>(this->doorPlacementDefs.size());
}

const LevelDoorPlacementDefinition &LevelDefinition::getDoorPlacementDef(int index) const
{
	DebugAssertIndex(this->doorPlacementDefs, index);
	return this->doorPlacementDefs[index];
}

int LevelDefinition::getChasmPlacementDefCount() const
{
	return static_cast<int>(this->chasmPlacementDefs.size());
}

const LevelChasmPlacementDefinition &LevelDefinition::getChasmPlacementDef(int index) const
{
	DebugAssertIndex(this->chasmPlacementDefs, index);
	return this->chasmPlacementDefs[index];
}

void LevelDefinition::addEntity(LevelVoxelEntityDefID id, const WorldDouble2 &position)
{
	const auto iter = std::find_if(this->entityPlacementDefs.begin(), this->entityPlacementDefs.end(),
		[id](const LevelEntityPlacementDefinition &def)
	{
		return def.id == id;
	});

	if (iter != this->entityPlacementDefs.end())
	{
		std::vector<WorldDouble2> &positions = iter->positions;
		positions.emplace_back(position);
	}
	else
	{
		this->entityPlacementDefs.emplace_back(id, std::vector<WorldDouble2> { position });
	}
}

void LevelDefinition::addLock(LevelVoxelLockDefID id, const WorldInt3 &position)
{
	const auto iter = std::find_if(this->lockPlacementDefs.begin(), this->lockPlacementDefs.end(),
		[id](const LevelLockPlacementDefinition &def)
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
		[id](const LevelTriggerPlacementDefinition &def)
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
		[id](const LevelTransitionPlacementDefinition &def)
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
	const auto iter = std::find_if(this->buildingNamePlacementDefs.begin(), this->buildingNamePlacementDefs.end(),
		[id](const LevelBuildingNamePlacementDefinition &def)
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
	const auto iter = std::find_if(this->doorPlacementDefs.begin(), this->doorPlacementDefs.end(),
		[id](const LevelDoorPlacementDefinition &def)
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
	const auto iter = std::find_if(this->chasmPlacementDefs.begin(), this->chasmPlacementDefs.end(),
		[id](const LevelChasmPlacementDefinition &def)
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
