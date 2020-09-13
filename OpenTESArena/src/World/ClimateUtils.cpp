#include "ClimateType.h"
#include "ClimateUtils.h"

#include "components/debug/Debug.h"

int ClimateUtils::getClimateTypeCount()
{
	return 3;
}

ClimateType ClimateUtils::getClimateType(int index)
{
	DebugAssert((index >= 0) && (index < ClimateUtils::getClimateTypeCount()));
	return static_cast<ClimateType>(index);
}
