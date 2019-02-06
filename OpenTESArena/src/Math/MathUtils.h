#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <algorithm>

namespace MathUtils
{
	template <typename T>
	const T &clamp(const T &value, const T &low, const T &high)
	{
		return std::min(std::max(value, low), high);
	}

	// Converts vertical field of view to camera zoom (where 90 degrees = 1.0 zoom).
	double verticalFovToZoom(double fovY);

	// Converts vertical field of view to horizontal field of view.
	double verticalFovToHorizontalFov(double fovY, double aspectRatio);
}

#endif
