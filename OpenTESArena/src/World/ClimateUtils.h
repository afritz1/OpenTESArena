#ifndef CLIMATE_UTILS_H
#define CLIMATE_UTILS_H

#include "../Assets/ArenaTypes.h"

namespace ClimateUtils
{
	int getClimateTypeCount();
	ArenaTypes::ClimateType getClimateType(int index);
}

#endif
