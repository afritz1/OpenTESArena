#ifndef CLIMATE_UTILS_H
#define CLIMATE_UTILS_H

enum class ClimateType;

namespace ClimateUtils
{
	int getClimateTypeCount();
	ClimateType getClimateType(int index);
}

#endif
