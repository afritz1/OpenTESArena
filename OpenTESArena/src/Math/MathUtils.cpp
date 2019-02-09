#include <cassert>
#include <cmath>

#include "Constants.h"
#include "MathUtils.h"

double MathUtils::getYAngleRadians(double x, double y, double z)
{
	// Get the length of the direction vector's projection onto the XZ plane.
	const double xzProjection = std::sqrt((x * x) + (z * z));

	if (y > 0.0)
	{
		// Above the horizon.
		return std::acos(xzProjection);
	}
	else if (y < 0.0)
	{
		// Below the horizon.
		return -std::acos(xzProjection);
	}
	else
	{
		// At the horizon.
		return 0.0;
	}
}

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
