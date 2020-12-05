#include "InteriorType.h"
#include "InteriorUtils.h"

#include "components/debug/Debug.h"

std::optional<InteriorType> InteriorUtils::menuTypeToInteriorType(ArenaTypes::MenuType menuType)
{
	switch (menuType)
	{
		case ArenaTypes::MenuType::None:
		case ArenaTypes::MenuType::CityGates:
			return std::nullopt;
		case ArenaTypes::MenuType::Crypt:
			return InteriorType::Crypt;
		case ArenaTypes::MenuType::Dungeon:
			return InteriorType::Dungeon;
		case ArenaTypes::MenuType::Equipment:
			return InteriorType::Equipment;
		case ArenaTypes::MenuType::House:
			return InteriorType::House;
		case ArenaTypes::MenuType::MagesGuild:
			return InteriorType::MagesGuild;
		case ArenaTypes::MenuType::Noble:
			return InteriorType::Noble;
		case ArenaTypes::MenuType::Palace:
			return InteriorType::Palace;
		case ArenaTypes::MenuType::Tavern:
			return InteriorType::Tavern;
		case ArenaTypes::MenuType::Temple:
			return InteriorType::Temple;
		case ArenaTypes::MenuType::Tower:
			return InteriorType::Tower;
		default:
			DebugUnhandledReturnMsg(std::optional<InteriorType>, std::to_string(static_cast<int>(menuType)));
	};
}

bool InteriorUtils::isPrefabInterior(InteriorType interiorType)
{
	return (interiorType == InteriorType::Crypt) || (interiorType == InteriorType::Equipment) ||
		(interiorType == InteriorType::House) || (interiorType == InteriorType::MagesGuild) ||
		(interiorType == InteriorType::Noble) || (interiorType == InteriorType::Palace) ||
		(interiorType == InteriorType::Tavern) || (interiorType == InteriorType::Temple) ||
		(interiorType == InteriorType::Tower);
}

bool InteriorUtils::isProceduralInterior(InteriorType interiorType)
{
	return interiorType == InteriorType::Dungeon;
}
