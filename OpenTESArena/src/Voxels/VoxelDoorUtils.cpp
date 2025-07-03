#include <algorithm>

#include "VoxelChunk.h"
#include "VoxelDoorUtils.h"
#include "../Rendering/ArenaRenderUtils.h"

double VoxelDoorUtils::getAnimPercentOrZero(SNInt x, int y, WEInt z, const VoxelChunk &voxelChunk)
{
	double animPercent = 0.0;
	int animInstIndex;
	if (voxelChunk.tryGetDoorAnimInstIndex(x, y, z, &animInstIndex))
	{
		const VoxelDoorAnimationInstance &animInst = voxelChunk.doorAnimInsts[animInstIndex];
		animPercent = animInst.percentOpen;
	}

	return animPercent;
}

Radians VoxelDoorUtils::getSwingingRotationRadians(Radians baseRadians, double animPercent)
{
	return baseRadians + (-(Constants::HalfPi - Constants::Epsilon) * animPercent);
}

double VoxelDoorUtils::getAnimatedTexCoordPercent(double animPercent)
{
	return std::clamp((1.0 - ArenaRenderUtils::DOOR_MIN_VISIBLE) * animPercent, 0.0, 1.0);
}

double VoxelDoorUtils::getAnimatedScaleAmount(double texCoordPercent)
{
	return std::clamp(1.0 - texCoordPercent, 0.0, 1.0);
}
