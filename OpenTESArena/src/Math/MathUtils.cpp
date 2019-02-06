#include <cassert>
#include <cmath>

#include "Constants.h"
#include "MathUtils.h"

double MathUtils::verticalFovToZoom(double fovY)
{
	return 1.0 / std::tan((fovY * 0.5) * Constants::DegToRad);
}

double MathUtils::verticalFovToHorizontalFov(double fovY, double aspectRatio)
{
	assert(fovY > 0.0);
	assert(fovY < 180.0);
	assert(aspectRatio > 0.0);

	const double halfDim = aspectRatio * std::tan((fovY * 0.50) * Constants::DegToRad);
	return (2.0 * std::atan(halfDim)) * Constants::RadToDeg;
}
