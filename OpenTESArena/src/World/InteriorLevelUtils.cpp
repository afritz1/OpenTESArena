#include "InteriorLevelUtils.h"

int InteriorLevelUtils::packLevelChangeVoxel(WEInt x, SNInt y)
{
	return (10 * y) + x;
}

void InteriorLevelUtils::unpackLevelChangeVoxel(int voxel, WEInt *outX, SNInt *outY)
{
	*outX = voxel % 10;
	*outY = voxel / 10;
}

int InteriorLevelUtils::offsetLevelChangeVoxel(int coord)
{
	return 10 + (coord * InteriorLevelUtils::DUNGEON_CHUNK_DIM);
}

uint16_t InteriorLevelUtils::convertLevelChangeVoxel(uint8_t voxel)
{
	return (voxel << 8) | voxel;
}
