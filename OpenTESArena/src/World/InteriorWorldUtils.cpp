#include "InteriorWorldUtils.h"
#include "../Math/Random.h"

int InteriorWorldUtils::generateDungeonLevelCount(bool isArtifactDungeon, ArenaRandom &random)
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
