#ifndef MATH_UTILS_H
#define MATH_UTILS_H

namespace MathUtils
{
	// Converts vertical field of view to camera zoom (where 90 degrees = 1.0 zoom).
	double verticalFovToZoom(double fovY);
}

#endif
