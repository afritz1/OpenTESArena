#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cmath>

// Math constants.
namespace Constants
{
	constexpr double Epsilon = 1.0e-5;
	constexpr double Pi = 3.1415926535897932;
	constexpr double DegToRad = Pi / 180.0;
	constexpr double RadToDeg = 180.0 / Pi;
	const double JustBelowOne = std::nextafter(1.0, 0.0);
}

#endif
