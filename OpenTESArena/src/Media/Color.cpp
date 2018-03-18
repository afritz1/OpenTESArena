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

Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color::Color(uint8_t r, uint8_t g, uint8_t b)
	: Color(r, g, b, 255) { }

Color::Color()
	: Color(0, 0, 0) { }

Color Color::randomRGBA(Random &random)
{
	uint8_t r = static_cast<uint8_t>(random.next(256));
	uint8_t g = static_cast<uint8_t>(random.next(256));
	uint8_t b = static_cast<uint8_t>(random.next(256));
	uint8_t a = static_cast<uint8_t>(random.next(256));
	return Color(r, g, b, a);
}

Color Color::randomRGB(Random &random)
{
	uint8_t r = static_cast<uint8_t>(random.next(256));
	uint8_t g = static_cast<uint8_t>(random.next(256));
	uint8_t b = static_cast<uint8_t>(random.next(256));
	return Color(r, g, b);
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

std::string Color::toString() const
{
	return std::string("[r=") +
		std::to_string(this->r) + std::string(", g=") +
		std::to_string(this->g) + std::string(", b=") +
		std::to_string(this->b) + std::string(", a=") +
		std::to_string(this->a) + std::string("]");
}

uint32_t Color::toARGB() const
{
	return static_cast<uint32_t>(
		(this->r << 16) | (this->g << 8) | (this->b) | (this->a << 24));
}

uint32_t Color::toRGBA() const
{
	return static_cast<uint32_t>(
		(this->r << 24) | (this->g << 16) | (this->b << 8) | (this->a));
}

uint32_t Color::toRGB() const
{
	return static_cast<uint32_t>(
		(this->r << 16) | (this->g << 8) | (this->b));
}

Color Color::operator+(const Color &c) const
{
	return Color(this->r + c.r, this->g + c.g, this->b + c.b, this->a + c.a);
}

Color Color::operator-(const Color &c) const
{
	return Color(this->r - c.r, this->g - c.g, this->b - c.b, this->a - c.a);
}

bool Color::operator==(const Color &c) const
{
	return (this->r == c.r) && (this->g == c.g) && (this->b == c.b) &&
		(this->a == c.a);
}

bool Color::operator!=(const Color &c) const
{
	return (this->r != c.r) || (this->g != c.g) || (this->b != c.b) ||
		(this->a != c.a);
}

Color Color::clamped(uint8_t low, uint8_t high) const
{
	return Color(
		(this->r > high) ? high : ((this->r < low) ? low : this->r),
		(this->g > high) ? high : ((this->g < low) ? low : this->g),
		(this->b > high) ? high : ((this->b < low) ? low : this->b),
		(this->a > high) ? high : ((this->a < low) ? low : this->a));
}

Color Color::clamped() const
{
	const uint8_t low = 0;
	const uint8_t high = 255;
	return this->clamped(low, high);
}
