#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace Constants
{
	constexpr double Epsilon = 1.0e-5;
	constexpr double Pi = 3.1415926535897932;
	constexpr double HalfPi = Pi / 2.0;
	constexpr double TwoPi = Pi * 2.0;
	constexpr double JustBelowOne = 1.0 - Epsilon;
	constexpr double Sqrt2 = 1.414213562373095;
	constexpr double HalfSqrt2 = Sqrt2 / 2.0;
}

namespace ConstantsF
{
	constexpr float Epsilon = 1.0e-5f;
	constexpr float Pi = 3.14159265f;
	constexpr float HalfPi = Pi / 2.0f;
	constexpr float TwoPi = Pi * 2.0f;
	constexpr float JustBelowOne = 1.0f - Epsilon;
	constexpr float Sqrt2 = 1.41421356f;
	constexpr float HalfSqrt2 = Sqrt2 / 2.0f;
}

#endif
