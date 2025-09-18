#ifndef COLOR_H
#define COLOR_H

#include <cstdint>
#include <string>

#include "Endian.h"

class Random;

struct Color
{
	uint8_t r, g, b, a;

	constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	constexpr Color(uint8_t r, uint8_t g, uint8_t b)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = 255;
	}
	
	constexpr Color()
	{
		this->r = 0;
		this->g = 0;
		this->b = 0;
		this->a = 255;
	}

	static Color randomRGBA(Random &random);

	static constexpr Color fromRGBA(uint32_t rgba)
	{
		const uint8_t r = static_cast<uint8_t>(rgba >> Endian::RGBA_RedShift);
		const uint8_t g = static_cast<uint8_t>(rgba >> Endian::RGBA_GreenShift);
		const uint8_t b = static_cast<uint8_t>(rgba >> Endian::RGBA_BlueShift);
		const uint8_t a = static_cast<uint8_t>(rgba >> Endian::RGBA_AlphaShift);
		return Color(r, g, b, a);
	}

	Color operator+(const Color &other) const;
	Color operator-(const Color &other) const;
	bool operator==(const Color &other) const;
	bool operator!=(const Color &other) const;
	std::string toString() const;

	constexpr uint32_t toRGBA() const
	{
		return static_cast<uint32_t>(
			(this->r << Endian::RGBA_RedShift) |
			(this->g << Endian::RGBA_GreenShift) |
			(this->b << Endian::RGBA_BlueShift) |
			(this->a << Endian::RGBA_AlphaShift));
	}

	constexpr Color clamped(uint8_t low, uint8_t high) const
	{
		return Color(
			(this->r > high) ? high : ((this->r < low) ? low : this->r),
			(this->g > high) ? high : ((this->g < low) ? low : this->g),
			(this->b > high) ? high : ((this->b < low) ? low : this->b),
			(this->a > high) ? high : ((this->a < low) ? low : this->a));
	}
};

struct ColorReal
{
	double r, g, b, a;

	constexpr ColorReal(double r, double g, double b, double a)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	constexpr ColorReal(double r, double g, double b)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = 1.0;
	}

	constexpr ColorReal()
	{
		this->r = 0.0;
		this->g = 0.0;
		this->b = 0.0;
		this->a = 1.0;
	}

	static ColorReal randomRGBA(Random &random);
	static ColorReal fromRGBA(uint32_t rgba);

	ColorReal operator+(const ColorReal &other) const;
	ColorReal operator-(const ColorReal &other) const;
	ColorReal operator*(double value) const;
	bool operator==(const ColorReal &other) const;
	bool operator!=(const ColorReal &other) const;

	std::string toString() const;
	uint32_t toRGBA() const;
	ColorReal clamped(double low, double high) const;
};

namespace Colors
{
	constexpr Color Red(255, 0, 0);
	constexpr uint32_t RedRGBA = Red.toRGBA();

	constexpr Color Green(0, 255, 0);
	constexpr uint32_t GreenRGBA = Green.toRGBA();
	
	constexpr Color Blue(0, 0, 255);
	constexpr uint32_t BlueRGBA = Blue.toRGBA();

	constexpr Color Cyan(0, 255, 255);
	constexpr uint32_t CyanRGBA = Cyan.toRGBA();

	constexpr Color Magenta(255, 0, 255);
	constexpr uint32_t MagentaRGBA = Magenta.toRGBA();

	constexpr Color Yellow(255, 255, 0);
	constexpr uint32_t YellowRGBA = Yellow.toRGBA();

	constexpr Color Black(0, 0, 0);
	constexpr uint32_t BlackRGBA = Black.toRGBA();

	constexpr Color White(255, 255, 255);
	constexpr uint32_t WhiteRGBA = White.toRGBA();

	constexpr Color Gray(128, 128, 128);
	constexpr uint32_t GrayRGBA = Gray.toRGBA();

	constexpr Color Transparent(0, 0, 0, 0);
	constexpr uint32_t TransparentRGBA = Transparent.toRGBA();
}

#endif
