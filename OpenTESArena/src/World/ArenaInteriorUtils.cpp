#include "ArenaInteriorUtils.h"
#include "../Math/Random.h"

int ArenaInteriorUtils::packLevelChangeVoxel(WEInt x, SNInt y)
{
	return (10 * y) + x;
}

void ArenaInteriorUtils::unpackLevelChangeVoxel(int voxel, WEInt *outX, SNInt *outY)
{
	*outX = voxel % 10;
	*outY = voxel / 10;
}

int ArenaInteriorUtils::offsetLevelChangeVoxel(int coord)
{
	return 10 + (coord * ArenaInteriorUtils::DUNGEON_CHUNK_DIM);
}

uint16_t ArenaInteriorUtils::convertLevelChangeVoxel(uint8_t voxel)
{
	return (voxel << 8) | voxel;
}

int ArenaInteriorUtils::generateDungeonLevelCount(bool isArtifactDungeon, ArenaRandom &random)
{
	if (isArtifactDungeon)
	{
		return 4;
	}
	else
	{
		return 1 + (random.next() % 2);
	}
}
