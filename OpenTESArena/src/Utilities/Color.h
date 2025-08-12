#ifndef COLOR_H
#define COLOR_H

#include <cstdint>
#include <string>

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
	static Color randomRGB(Random &random);

	static constexpr Color fromARGB(uint32_t argb)
	{
		return Color(
			static_cast<uint8_t>(argb >> 16),
			static_cast<uint8_t>(argb >> 8),
			static_cast<uint8_t>(argb),
			static_cast<uint8_t>(argb >> 24));
	}

	static constexpr Color fromRGBA(uint32_t rgba)
	{
		return Color(
			static_cast<uint8_t>(rgba >> 24),
			static_cast<uint8_t>(rgba >> 16),
			static_cast<uint8_t>(rgba >> 8),
			static_cast<uint8_t>(rgba));
	}

	static constexpr Color fromRGB(uint32_t rgb)
	{
		return Color(
			static_cast<uint8_t>(rgb >> 16),
			static_cast<uint8_t>(rgb >> 8),
			static_cast<uint8_t>(rgb));
	}

	Color operator+(const Color &other) const;
	Color operator-(const Color &other) const;
	bool operator==(const Color &other) const;
	bool operator!=(const Color &other) const;
	std::string toString() const;
	
	constexpr uint32_t toARGB() const
	{
		return static_cast<uint32_t>((this->r << 16) | (this->g << 8) | (this->b) | (this->a << 24));
	}

	constexpr uint32_t toBGRA() const
	{
		return static_cast<uint32_t>((this->r << 8) | (this->g << 16) | (this->b << 24) | this->a);
	}

	constexpr uint32_t toRGBA() const
	{
		return static_cast<uint32_t>((this->r << 24) | (this->g << 16) | (this->b << 8) | (this->a));
	}

	constexpr uint32_t toRGB() const
	{
		return static_cast<uint32_t>((this->r << 16) | (this->g << 8) | (this->b));
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
	static ColorReal randomRGB(Random &random);
	static ColorReal fromARGB(uint32_t argb);
	static ColorReal fromRGBA(uint32_t rgba);
	static ColorReal fromRGB(uint32_t rgb);

	ColorReal operator+(const ColorReal &other) const;
	ColorReal operator-(const ColorReal &other) const;
	ColorReal operator*(double value) const;
	bool operator==(const ColorReal &other) const;
	bool operator!=(const ColorReal &other) const;

	std::string toString() const;
	uint32_t toARGB() const;
	uint32_t toRGBA() const;
	uint32_t toRGB() const;
	ColorReal clamped(double low, double high) const;
};

namespace Colors
{
	constexpr Color Red(255, 0, 0);
	constexpr uint32_t RedARGB = Red.toARGB();
	constexpr uint32_t RedRGBA = Red.toRGBA();
	constexpr uint32_t RedRGB = Red.toRGB();

	constexpr Color Green(0, 255, 0);
	constexpr uint32_t GreenARGB = Green.toARGB();
	constexpr uint32_t GreenRGBA = Green.toRGBA();
	constexpr uint32_t GreenRGB = Green.toRGB();
	
	constexpr Color Blue(0, 0, 255);
	constexpr uint32_t BlueARGB = Blue.toARGB();
	constexpr uint32_t BlueRGBA = Blue.toRGBA();
	constexpr uint32_t BlueRGB = Blue.toRGB();

	constexpr Color Cyan(0, 255, 255);
	constexpr uint32_t CyanARGB = Cyan.toARGB();
	constexpr uint32_t CyanRGBA = Cyan.toRGBA();
	constexpr uint32_t CyanRGB = Cyan.toRGB();

	constexpr Color Magenta(255, 0, 255);
	constexpr uint32_t MagentaARGB = Magenta.toARGB();
	constexpr uint32_t MagentaRGBA = Magenta.toRGBA();
	constexpr uint32_t MagentaRGB = Magenta.toRGB();

	constexpr Color Yellow(255, 255, 0);
	constexpr uint32_t YellowARGB = Yellow.toARGB();
	constexpr uint32_t YellowRGBA = Yellow.toRGBA();
	constexpr uint32_t YellowRGB = Yellow.toRGB();

	constexpr Color Black(0, 0, 0);
	constexpr uint32_t BlackARGB = Black.toARGB();
	constexpr uint32_t BlackRGBA = Black.toRGBA();
	constexpr uint32_t BlackRGB = Black.toRGB();

	constexpr Color White(255, 255, 255);
	constexpr uint32_t WhiteARGB = White.toARGB();
	constexpr uint32_t WhiteRGBA = White.toRGBA();
	constexpr uint32_t WhiteRGB = White.toRGB();

	constexpr Color Gray(128, 128, 128);
	constexpr uint32_t GrayARGB = Gray.toARGB();
	constexpr uint32_t GrayRGBA = Gray.toRGBA();
	constexpr uint32_t GrayRGB = Gray.toRGB();

	constexpr Color Transparent(0, 0, 0, 0);
	constexpr uint32_t TransparentARGB = Transparent.toARGB();
	constexpr uint32_t TransparentRGBA = Transparent.toRGBA();
	constexpr uint32_t TransparentRGB = Transparent.toRGB();
}

#endif
