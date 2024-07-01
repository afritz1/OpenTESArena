#include <algorithm>

#include "DoorUtils.h"
#include "VoxelChunk.h"
#include "../Rendering/ArenaRenderUtils.h"

double DoorUtils::getAnimPercentOrZero(SNInt x, int y, WEInt z, const VoxelChunk &voxelChunk)
{
	double animPercent = 0.0;
	int animInstIndex;
	if (voxelChunk.tryGetDoorAnimInstIndex(x, y, z, &animInstIndex))
	{
		BufferView<const VoxelDoorAnimationInstance> animInsts = voxelChunk.getDoorAnimInsts();
		const VoxelDoorAnimationInstance &animInst = animInsts[animInstIndex];
		animPercent = animInst.percentOpen;
	}

	return animPercent;
}

Radians DoorUtils::getSwingingRotationRadians(Radians baseRadians, double animPercent)
{
	return baseRadians + (-(Constants::HalfPi - Constants::Epsilon) * animPercent);
}

double DoorUtils::getAnimatedTexCoordPercent(double animPercent)
{
	return std::clamp((1.0 - ArenaRenderUtils::DOOR_MIN_VISIBLE) * animPercent, 0.0, 1.0);
}

double DoorUtils::getAnimatedScaleAmount(double texCoordPercent)
{
	return std::clamp(1.0 - texCoordPercent, 0.0, 1.0);
}
