#include "ArenaInteriorUtils.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"

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

std::optional<ArenaInteriorType> ArenaInteriorUtils::menuTypeToInteriorType(ArenaMenuType menuType)
{
	switch (menuType)
	{
	case ArenaMenuType::None:
	case ArenaMenuType::CityGates:
		return std::nullopt;
	case ArenaMenuType::Crypt:
		return ArenaInteriorType::Crypt;
	case ArenaMenuType::Dungeon:
		return ArenaInteriorType::Dungeon;
	case ArenaMenuType::Equipment:
		return ArenaInteriorType::Equipment;
	case ArenaMenuType::House:
		return ArenaInteriorType::House;
	case ArenaMenuType::MagesGuild:
		return ArenaInteriorType::MagesGuild;
	case ArenaMenuType::Noble:
		return ArenaInteriorType::Noble;
	case ArenaMenuType::Palace:
		return ArenaInteriorType::Palace;
	case ArenaMenuType::Tavern:
		return ArenaInteriorType::Tavern;
	case ArenaMenuType::Temple:
		return ArenaInteriorType::Temple;
	case ArenaMenuType::Tower:
		return ArenaInteriorType::Tower;
	default:
		DebugUnhandledReturnMsg(std::optional<ArenaInteriorType>, std::to_string(static_cast<int>(menuType)));
	};
}

bool ArenaInteriorUtils::isPrefabInterior(ArenaInteriorType interiorType)
{
	return (interiorType == ArenaInteriorType::Crypt) ||
		(interiorType == ArenaInteriorType::Equipment) ||
		(interiorType == ArenaInteriorType::House) ||
		(interiorType == ArenaInteriorType::MagesGuild) ||
		(interiorType == ArenaInteriorType::Noble) ||
		(interiorType == ArenaInteriorType::Palace) ||
		(interiorType == ArenaInteriorType::Tavern) ||
		(interiorType == ArenaInteriorType::Temple) ||
		(interiorType == ArenaInteriorType::Tower);
}

bool ArenaInteriorUtils::isProceduralInterior(ArenaInteriorType interiorType)
{
	return interiorType == ArenaInteriorType::Dungeon;
}
