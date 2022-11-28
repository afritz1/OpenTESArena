#ifndef COLOR_H
#define COLOR_H

#include <cstdint>
#include <string>

class Random;

class Color
{
public:
	Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	Color(uint8_t r, uint8_t g, uint8_t b);
	Color();

	uint8_t r, g, b, a;

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

	Color operator+(const Color &c) const;
	Color operator-(const Color &c) const;
	bool operator==(const Color &c) const;
	bool operator!=(const Color &c) const;

	std::string toString() const;
	uint32_t toARGB() const;
	uint32_t toRGBA() const;
	uint32_t toRGB() const;
	Color clamped(uint8_t low, uint8_t high) const;
	Color clamped() const;
};

#endif
