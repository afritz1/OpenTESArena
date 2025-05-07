#include <cmath>

#include "WorldMapMask.h"

#include "components/debug/Debug.h"

WorldMapMask::WorldMapMask(std::vector<uint8_t> &&mask, const Rect &rect)
	: mask(std::move(mask)), rect(rect) { }

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
		(relativeY * WorldMapMask::getAdjustedWidth(this->rect.width)));

	DebugAssertIndex(this->mask, byteIndex);
	const uint8_t maskByte = this->mask[byteIndex];
	const int bitIndex = 7 - (relativeX % 8);
	return (maskByte & (1 << bitIndex)) != 0;
}
