#include "ArenaClimateUtils.h"

#include "components/debug/Debug.h"

int ArenaClimateUtils::getClimateTypeCount()
{
	return 3;
}

ArenaTypes::ClimateType ArenaClimateUtils::getClimateType(int index)
{
	DebugAssert((index >= 0) && (index < ArenaClimateUtils::getClimateTypeCount()));
	return static_cast<ArenaTypes::ClimateType>(index);
}
