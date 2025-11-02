#include <cstdio>

#include "Color.h"
#include "../Math/Random.h"

Color Color::randomRGBA(Random &random)
{
	return Color(
		static_cast<uint8_t>(random.next(256)),
		static_cast<uint8_t>(random.next(256)),
		static_cast<uint8_t>(random.next(256)),
		static_cast<uint8_t>(random.next(256)));
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

ColorReal ColorReal::randomRGBA(Random &random)
{
	return ColorReal(
		random.nextReal(),
		random.nextReal(),
		random.nextReal(),
		random.nextReal());
}

ColorReal ColorReal::fromRGBA(uint32_t rgba)
{
	const uint8_t r = static_cast<uint8_t>(rgba >> Endian::RGBA_RedShift);
	const uint8_t g = static_cast<uint8_t>(rgba >> Endian::RGBA_GreenShift);
	const uint8_t b = static_cast<uint8_t>(rgba >> Endian::RGBA_BlueShift);
	const uint8_t a = static_cast<uint8_t>(rgba >> Endian::RGBA_AlphaShift);
	return ColorReal(
		static_cast<double>(r) / 255.0,
		static_cast<double>(g) / 255.0,
		static_cast<double>(b) / 255.0,
		static_cast<double>(a) / 255.0);
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

uint32_t ColorReal::toRGBA() const
{
	const ColorReal clampedColor = this->clamped(0.0, 1.0);
	const uint8_t r = static_cast<uint8_t>(clampedColor.r * 255.0);
	const uint8_t g = static_cast<uint8_t>(clampedColor.g * 255.0);
	const uint8_t b = static_cast<uint8_t>(clampedColor.b * 255.0);
	const uint8_t a = static_cast<uint8_t>(clampedColor.a * 255.0);
	return static_cast<uint32_t>(
		(r << Endian::RGBA_RedShift) |
		(g << Endian::RGBA_GreenShift) |
		(b << Endian::RGBA_BlueShift) |
		(a << Endian::RGBA_AlphaShift));
}

ColorReal ColorReal::clamped(double low, double high) const
{
	return ColorReal(
		(this->r > high) ? high : ((this->r < low) ? low : this->r),
		(this->g > high) ? high : ((this->g < low) ? low : this->g),
		(this->b > high) ? high : ((this->b < low) ? low : this->b),
		(this->a > high) ? high : ((this->a < low) ? low : this->a));
}
