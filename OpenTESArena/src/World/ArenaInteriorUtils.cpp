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

std::optional<ArenaTypes::InteriorType> ArenaInteriorUtils::menuTypeToInteriorType(ArenaTypes::MenuType menuType)
{
	switch (menuType)
	{
	case ArenaTypes::MenuType::None:
	case ArenaTypes::MenuType::CityGates:
		return std::nullopt;
	case ArenaTypes::MenuType::Crypt:
		return ArenaTypes::InteriorType::Crypt;
	case ArenaTypes::MenuType::Dungeon:
		return ArenaTypes::InteriorType::Dungeon;
	case ArenaTypes::MenuType::Equipment:
		return ArenaTypes::InteriorType::Equipment;
	case ArenaTypes::MenuType::House:
		return ArenaTypes::InteriorType::House;
	case ArenaTypes::MenuType::MagesGuild:
		return ArenaTypes::InteriorType::MagesGuild;
	case ArenaTypes::MenuType::Noble:
		return ArenaTypes::InteriorType::Noble;
	case ArenaTypes::MenuType::Palace:
		return ArenaTypes::InteriorType::Palace;
	case ArenaTypes::MenuType::Tavern:
		return ArenaTypes::InteriorType::Tavern;
	case ArenaTypes::MenuType::Temple:
		return ArenaTypes::InteriorType::Temple;
	case ArenaTypes::MenuType::Tower:
		return ArenaTypes::InteriorType::Tower;
	default:
		DebugUnhandledReturnMsg(std::optional<ArenaTypes::InteriorType>, std::to_string(static_cast<int>(menuType)));
	};
}

bool ArenaInteriorUtils::isPrefabInterior(ArenaTypes::InteriorType interiorType)
{
	return (interiorType == ArenaTypes::InteriorType::Crypt) ||
		(interiorType == ArenaTypes::InteriorType::Equipment) ||
		(interiorType == ArenaTypes::InteriorType::House) ||
		(interiorType == ArenaTypes::InteriorType::MagesGuild) ||
		(interiorType == ArenaTypes::InteriorType::Noble) ||
		(interiorType == ArenaTypes::InteriorType::Palace) ||
		(interiorType == ArenaTypes::InteriorType::Tavern) ||
		(interiorType == ArenaTypes::InteriorType::Temple) ||
		(interiorType == ArenaTypes::InteriorType::Tower);
}

bool ArenaInteriorUtils::isProceduralInterior(ArenaTypes::InteriorType interiorType)
{
	return interiorType == ArenaTypes::InteriorType::Dungeon;
}
