#include "Color.h"

#include "../Math/Random.h"

const Color Color::Red = Color(255, 0, 0, 255);
const Color Color::Green = Color(0, 255, 0, 255);
const Color Color::Blue = Color(0, 0, 255, 255);
const Color Color::Cyan = Color(0, 255, 255, 255);
const Color Color::Magenta = Color(255, 0, 255, 255);
const Color Color::Yellow = Color(255, 255, 0, 255);
const Color Color::Black = Color(0, 0, 0, 255);
const Color Color::White = Color(255, 255, 255, 255);
const Color Color::Gray = Color(127, 127, 127, 255);
const Color Color::Transparent = Color(0, 0, 0, 0);

Color::Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color::Color(unsigned char r, unsigned char g, unsigned char b)
	: Color(r, g, b, 255) { }

Color::Color()
	: Color(0, 0, 0) { }

Color::~Color()
{

}

Color Color::randomRGBA(Random &random)
{
	unsigned char r = static_cast<unsigned char>(random.next(256));
	unsigned char g = static_cast<unsigned char>(random.next(256));
	unsigned char b = static_cast<unsigned char>(random.next(256));
	unsigned char a = static_cast<unsigned char>(random.next(256));
	return Color(r, g, b, a);
}

Color Color::randomRGB(Random &random)
{
	unsigned char r = static_cast<unsigned char>(random.next(256));
	unsigned char g = static_cast<unsigned char>(random.next(256));
	unsigned char b = static_cast<unsigned char>(random.next(256));
	return Color(r, g, b);
}

Color Color::fromARGB(unsigned int argb)
{
	return Color(
		static_cast<unsigned char>(argb >> 16),
		static_cast<unsigned char>(argb >> 8),
		static_cast<unsigned char>(argb),
		static_cast<unsigned char>(argb >> 24));
}

Color Color::fromRGB(unsigned int rgb)
{
	return Color(
		static_cast<unsigned char>(rgb >> 16),
		static_cast<unsigned char>(rgb >> 8),
		static_cast<unsigned char>(rgb));
}

unsigned char Color::getR() const
{
	return this->r;
}

unsigned char Color::getG() const
{
	return this->g;
}

unsigned char Color::getB() const
{
	return this->b;
}

unsigned char Color::getA() const
{
	return this->a;
}

std::string Color::toString() const
{
	return std::string("[r=") +
		std::to_string(this->r) + std::string(", g=") +
		std::to_string(this->g) + std::string(", b=") +
		std::to_string(this->b) + std::string(", a=") +
		std::to_string(this->a) + std::string("]");
}

unsigned int Color::toARGB() const
{
	return static_cast<unsigned int>(
		(this->r << 16) | (this->g << 8) | (this->b) | (this->a << 24));
}

unsigned int Color::toRGB() const
{
	return static_cast<unsigned int>(
		(this->r << 16) | (this->g << 8) | (this->b));
}

Color Color::operator +(const Color &c) const
{
	return Color(this->r + c.r, this->g + c.g, this->b + c.b, this->a + c.a);
}

Color Color::operator -(const Color &c) const
{
	return Color(this->r - c.r, this->g - c.g, this->b - c.b, this->a - c.a);
}

bool Color::operator ==(const Color &c) const
{
	return (this->a == c.a) && (this->r == c.r) && (this->g == c.g) &&
		(this->b == c.b);
}

bool Color::operator !=(const Color &c) const
{
	return !((*this) == c);
}

Color Color::clamped(unsigned char low, unsigned char high) const
{
	return Color(
		(this->r > high) ? high : ((this->r < low) ? low : this->r),
		(this->g > high) ? high : ((this->g < low) ? low : this->g),
		(this->b > high) ? high : ((this->b < low) ? low : this->b),
		(this->a > high) ? high : ((this->a < low) ? low : this->a));
}

Color Color::clamped() const
{
	const unsigned char low = 0;
	const unsigned char high = 255;
	return this->clamped(low, high);
}
