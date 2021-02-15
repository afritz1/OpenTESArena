#include "LevelDefinition.h"
#include "LevelInfoDefinition.h"
#include "LevelUtils.h"
#include "VoxelUtils.h"

const TransitionDefinition *LevelUtils::tryGetTransition(const LevelInt3 &voxel, const LevelDefinition &levelDef,
	const LevelInfoDefinition &levelInfoDef)
{
	for (int i = 0; i < levelDef.getTransitionPlacementDefCount(); i++)
	{
		const LevelDefinition::TransitionPlacementDef &placementDef = levelDef.getTransitionPlacementDef(i);
		for (const LevelInt3 &placementPos : placementDef.positions)
		{
			if (placementPos == voxel)
			{
				return &levelInfoDef.getTransitionDef(placementDef.id);
			}
		}
	}

	return nullptr;
}
