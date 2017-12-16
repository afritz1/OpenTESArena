#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cmath>

// Math constants.
namespace Constants
{
	const double Epsilon = 1.0e-5;
	const double Pi = 3.1415926535897932;
	const double DegToRad = Pi / 180.0;
	const double RadToDeg = 180.0 / Pi;
	const double JustBelowOne = std::nextafter(1.0, 0.0);
}

#endif
