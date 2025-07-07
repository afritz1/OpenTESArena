#include <algorithm>
#include <limits>

#include "RenderLightUtils.h"

#include "components/debug/Debug.h"

namespace
{
	constexpr RenderLightID NO_LIGHT_ID = -1;
	constexpr double NO_DISTANCE_SQR = std::numeric_limits<double>::infinity();
}
