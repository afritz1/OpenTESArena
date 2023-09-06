#include <algorithm>

#include "DoorUtils.h"
#include "../Rendering/ArenaRenderUtils.h"

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
