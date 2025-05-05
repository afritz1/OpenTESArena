#include <cstdio>

#include "Color.h"
#include "../Math/Random.h"

const Color Color::Red(255, 0, 0, 255);
const Color Color::Green(0, 255, 0, 255);
const Color Color::Blue(0, 0, 255, 255);
const Color Color::Cyan(0, 255, 255, 255);
const Color Color::Magenta(255, 0, 255, 255);
const Color Color::Yellow(255, 255, 0, 255);
const Color Color::Black(0, 0, 0, 255);
const Color Color::White(255, 255, 255, 255);
const Color Color::Gray(127, 127, 127, 255);
const Color Color::Transparent(0, 0, 0, 0);

Color Color::randomRGBA(Random &random)
{
	return Color(
		static_cast<uint8_t>(random.next(256)),
		static_cast<uint8_t>(random.next(256)),
		static_cast<uint8_t>(random.next(256)),
		static_cast<uint8_t>(random.next(256)));
}

Color Color::randomRGB(Random &random)
{
	return Color(
		static_cast<uint8_t>(random.next(256)),
		static_cast<uint8_t>(random.next(256)),
		static_cast<uint8_t>(random.next(256)));
}

Color Color::fromARGB(uint32_t argb)
{
	return Color(
		static_cast<uint8_t>(argb >> 16),
		static_cast<uint8_t>(argb >> 8),
		static_cast<uint8_t>(argb),
		static_cast<uint8_t>(argb >> 24));
}

Color Color::fromRGBA(uint32_t rgba)
{
	return Color(
		static_cast<uint8_t>(rgba >> 24),
		static_cast<uint8_t>(rgba >> 16),
		static_cast<uint8_t>(rgba >> 8),
		static_cast<uint8_t>(rgba));
}

Color Color::fromRGB(uint32_t rgb)
{
	return Color(
		static_cast<uint8_t>(rgb >> 16),
		static_cast<uint8_t>(rgb >> 8),
		static_cast<uint8_t>(rgb));
}

Color Color::operator+(const Color &other) const
{
	return Color(this->r + other.r, this->g + other.g, this->b + other.b, this->a + other.a);
}

Color Color::operator-(const Color &other) const
{
	return Color(this->r - other.r, this->g - other.g, this->b - other.b, this->a - other.a);
}

bool Color::operator==(const Color &other) const
{
	return (this->r == other.r) && (this->g == other.g) && (this->b == other.b) && (this->a == other.a);
}

bool Color::operator!=(const Color &other) const
{
	return (this->r != other.r) || (this->g != other.g) || (this->b != other.b) || (this->a != other.a);
}

std::string Color::toString() const
{
	char buffer[64];
	std::snprintf(buffer, std::size(buffer), "%d, %d, %d, %d", this->r, this->g, this->b, this->a);
	return std::string(buffer);
}

uint32_t Color::toARGB() const
{
	return static_cast<uint32_t>((this->r << 16) | (this->g << 8) | (this->b) | (this->a << 24));
}

uint32_t Color::toRGBA() const
{
	return static_cast<uint32_t>((this->r << 24) | (this->g << 16) | (this->b << 8) | (this->a));
}

uint32_t Color::toRGB() const
{
	return static_cast<uint32_t>((this->r << 16) | (this->g << 8) | (this->b));
}

Color Color::clamped(uint8_t low, uint8_t high) const
{
	return Color(
		(this->r > high) ? high : ((this->r < low) ? low : this->r),
		(this->g > high) ? high : ((this->g < low) ? low : this->g),
		(this->b > high) ? high : ((this->b < low) ? low : this->b),
		(this->a > high) ? high : ((this->a < low) ? low : this->a));
}

ColorReal ColorReal::randomRGBA(Random &random)
{
	return ColorReal(
		random.nextReal(),
		random.nextReal(),
		random.nextReal(),
		random.nextReal());
}

ColorReal ColorReal::randomRGB(Random &random)
{
	return ColorReal(
		random.nextReal(),
		random.nextReal(),
		random.nextReal(),
		1.0);
}

ColorReal ColorReal::fromARGB(uint32_t argb)
{
	const uint8_t r = static_cast<uint8_t>(argb >> 16);
	const uint8_t g = static_cast<uint8_t>(argb >> 8);
	const uint8_t b = static_cast<uint8_t>(argb);
	const uint8_t a = static_cast<uint8_t>(argb >> 24);
	return ColorReal(
		static_cast<double>(r) / 255.0,
		static_cast<double>(g) / 255.0,
		static_cast<double>(b) / 255.0,
		static_cast<double>(a) / 255.0);
}

ColorReal ColorReal::fromRGBA(uint32_t rgba)
{
	const uint8_t r = static_cast<uint8_t>(rgba >> 24);
	const uint8_t g = static_cast<uint8_t>(rgba >> 16);
	const uint8_t b = static_cast<uint8_t>(rgba >> 8);
	const uint8_t a = static_cast<uint8_t>(rgba);
	return ColorReal(
		static_cast<double>(r) / 255.0,
		static_cast<double>(g) / 255.0,
		static_cast<double>(b) / 255.0,
		static_cast<double>(a) / 255.0);
}

ColorReal ColorReal::fromRGB(uint32_t rgb)
{
	const uint8_t r = static_cast<uint8_t>(rgb >> 16);
	const uint8_t g = static_cast<uint8_t>(rgb >> 8);
	const uint8_t b = static_cast<uint8_t>(rgb);
	return ColorReal(
		static_cast<double>(r) / 255.0,
		static_cast<double>(g) / 255.0,
		static_cast<double>(b) / 255.0,
		1.0);
}

ColorReal ColorReal::operator+(const ColorReal &other) const
{
	return ColorReal(
		this->r + other.r,
		this->g + other.g,
		this->b + other.b,
		this->a + other.a);
}

ColorReal ColorReal::operator-(const ColorReal &other) const
{
	return ColorReal(
		this->r - other.r,
		this->g - other.g,
		this->b - other.b,
		this->a - other.a);
}

ColorReal ColorReal::operator*(double value) const
{
	return ColorReal(
		this->r * value,
		this->g * value,
		this->b * value,
		this->a * value);
}

bool ColorReal::operator==(const ColorReal &other) const
{
	return (this->r == other.r) && (this->g == other.g) && (this->b == other.b) && (this->a == other.a);
}

bool ColorReal::operator!=(const ColorReal &other) const
{
	return (this->r != other.r) || (this->g != other.g) || (this->b != other.b) || (this->a != other.a);
}

std::string ColorReal::toString() const
{
	char buffer[64];
	std::snprintf(buffer, std::size(buffer), "(%.2f, %.2f, %.2f, %.2f)", this->r, this->g, this->b, this->a);
	return std::string(buffer);
}

uint32_t ColorReal::toARGB() const
{
	const uint8_t r = static_cast<uint8_t>(this->r * 255.0);
	const uint8_t g = static_cast<uint8_t>(this->g * 255.0);
	const uint8_t b = static_cast<uint8_t>(this->b * 255.0);
	const uint8_t a = static_cast<uint8_t>(this->a * 255.0);
	return static_cast<uint32_t>((r << 16) | (g << 8) | (b) | (a << 24));
}

uint32_t ColorReal::toRGBA() const
{
	const uint8_t r = static_cast<uint8_t>(this->r * 255.0);
	const uint8_t g = static_cast<uint8_t>(this->g * 255.0);
	const uint8_t b = static_cast<uint8_t>(this->b * 255.0);
	const uint8_t a = static_cast<uint8_t>(this->a * 255.0);
	return static_cast<uint32_t>((r << 24) | (g << 16) | (b << 8) | (a));
}

uint32_t ColorReal::toRGB() const
{
	const uint8_t r = static_cast<uint8_t>(this->r * 255.0);
	const uint8_t g = static_cast<uint8_t>(this->g * 255.0);
	const uint8_t b = static_cast<uint8_t>(this->b * 255.0);
	const uint8_t a = 255;
	return static_cast<uint32_t>((r << 16) | (g << 8) | (b) | (a << 24));
}

ColorReal ColorReal::clamped(double low, double high) const
{
	return ColorReal(
		(this->r > high) ? high : ((this->r < low) ? low : this->r),
		(this->g > high) ? high : ((this->g < low) ? low : this->g),
		(this->b > high) ? high : ((this->b < low) ? low : this->b),
		(this->a > high) ? high : ((this->a < low) ? low : this->a));
}
