#include <algorithm>

#include "LevelInfoDefinition.h"
#include "../Assets/INFFile.h"

#include "components/debug/Debug.h"

void LevelInfoDefinition::init(const INFFile &inf)
{
	DebugNotImplemented();
}

bool LevelInfoDefinition::tryGetVoxelDef(LevelDefinition::VoxelDefID id,
	const VoxelDefinition **outDef) const
{
	const auto iter = std::find_if(this->voxelDefs.begin(), this->voxelDefs.end(),
		[id](const auto &pair)
	{
		return pair.first == id;
	});

	if (iter != this->voxelDefs.end())
	{
		*outDef = &iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

bool LevelInfoDefinition::tryGetEntityDef(LevelDefinition::EntityDefID id,
	const EntityDefinition **outDef) const
{
	const auto iter = std::find_if(this->entityDefs.begin(), this->entityDefs.end(),
		[id](const auto &pair)
	{
		return pair.first == id;
	});

	if (iter != this->entityDefs.end())
	{
		*outDef = &iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

bool LevelInfoDefinition::tryGetLockDef(LevelDefinition::LockDefID id,
	const LockDefinition **outDef) const
{
	const auto iter = std::find_if(this->lockDefs.begin(), this->lockDefs.end(),
		[id](const auto &pair)
	{
		return pair.first == id;
	});

	if (iter != this->lockDefs.end())
	{
		*outDef = &iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

bool LevelInfoDefinition::tryGetTriggerDef(LevelDefinition::TriggerDefID id,
	const TriggerDefinition **outDef) const
{
	const auto iter = std::find_if(this->triggerDefs.begin(), this->triggerDefs.end(),
		[id](const auto &pair)
	{
		return pair.first == id;
	});

	if (iter != this->triggerDefs.end())
	{
		*outDef = &iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

double LevelInfoDefinition::getCeilingScale() const
{
	return this->ceilingScale;
}
