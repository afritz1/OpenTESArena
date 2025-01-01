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

	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Cyan;
	static const Color Magenta;
	static const Color Yellow;
	static const Color Black;
	static const Color White;
	static const Color Gray;
	static const Color Transparent;

	static Color randomRGBA(Random &random);
	static Color randomRGB(Random &random);
	static Color fromARGB(uint32_t argb);
	static Color fromRGBA(uint32_t rgba);
	static Color fromRGB(uint32_t rgb);

	Color operator+(const Color &other) const;
	Color operator-(const Color &other) const;
	bool operator==(const Color &other) const;
	bool operator!=(const Color &other) const;

	std::string toString() const;
	uint32_t toARGB() const;
	uint32_t toRGBA() const;
	uint32_t toRGB() const;
	Color clamped(uint8_t low, uint8_t high) const;
};

#endif
