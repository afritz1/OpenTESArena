#ifndef COLOR_H
#define COLOR_H

#include <string>

class Random;

class Color
{
private:
	unsigned char r, g, b, a;
public:
	Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
	Color(unsigned char r, unsigned char g, unsigned char b);
	Color();
	~Color();

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
	static Color fromARGB(unsigned int argb);
	static Color fromRGB(unsigned int rgb);

	Color operator +(const Color &c) const;
	Color operator -(const Color &c) const;
	bool operator ==(const Color &c) const;
	bool operator !=(const Color &c) const;

	const unsigned char &getR() const;
	const unsigned char &getG() const;
	const unsigned char &getB() const;
	const unsigned char &getA() const;

	std::string toString() const;
	unsigned int toARGB() const;
	unsigned int toRGB() const;
	Color clamped(unsigned char low, unsigned char high) const;
	Color clamped() const;
};

#endif
