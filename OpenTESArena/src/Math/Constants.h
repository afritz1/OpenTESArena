#ifndef CONSTANTS_H
#define CONSTANTS_H

// Math constants.
namespace Constants
{
	constexpr double Epsilon = 1.0e-5;
	constexpr double Pi = 3.1415926535897932;
	constexpr double HalfPi = Pi / 2.0;
	constexpr double TwoPi = Pi * 2.0;
	constexpr double DegToRad = Pi / 180.0;
	constexpr double RadToDeg = 180.0 / Pi;
	constexpr double JustBelowOne = 1.0 - Epsilon;
	constexpr double Sqrt2 = 1.414213562373095;
	constexpr double HalfSqrt2 = Sqrt2 / 2.0;
}

#endif
