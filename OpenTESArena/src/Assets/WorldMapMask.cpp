#include <cmath>

#include "WorldMapMask.h"

WorldMapMask::WorldMapMask(std::vector<uint8_t> &&mask, const Rect &rect)
	: mask(std::move(mask)), rect(rect) { }

WorldMapMask::WorldMapMask()
{
	// This exists so MiscAssets can store a std::array of them.
}

WorldMapMask::~WorldMapMask()
{

}

int WorldMapMask::getAdjustedWidth(int width)
{
	return static_cast<int>(std::ceil(static_cast<double>(width) / 8.0));
}

const Rect &WorldMapMask::getRect() const
{
	return this->rect;
}

bool WorldMapMask::get(int x, int y) const
{
	const int relativeX = x - this->rect.getLeft();
	const int relativeY = y - this->rect.getTop();
	const int byteIndex = ((relativeX / 8) + 
		(relativeY * WorldMapMask::getAdjustedWidth(this->rect.getWidth())));
	const uint8_t maskByte = this->mask.at(byteIndex);
	const int bitIndex = 7 - (relativeX % 8);
	return (maskByte & (1 << bitIndex)) != 0;
}
