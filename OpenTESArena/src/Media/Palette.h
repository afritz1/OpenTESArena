#ifndef PALETTE_H
#define PALETTE_H

#include <array>
#include <string>

#include "Color.h"

class Palette
{
private:
	std::array<Color, 256> colors;
public:
	Palette();
	~Palette();

	// Returns whether the given palette name is "built-in" or not.
	static bool isBuiltIn(const std::string &filename);

	std::array<Color, 256> &get();
	const std::array<Color, 256> &get() const;
};

#endif
