#include <cmath>

#include "Constants.h"
#include "MathUtils.h"

double MathUtils::verticalFovToZoom(double fovY)
{
	return 1.0 / std::tan((fovY * 0.5) * Constants::DegToRad);
}
