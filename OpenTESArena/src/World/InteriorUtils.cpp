#include "InteriorType.h"
#include "InteriorUtils.h"

#include "components/debug/Debug.h"

std::optional<InteriorType> InteriorUtils::menuTypeToInteriorType(VoxelDefinition::WallData::MenuType menuType)
{
	switch (menuType)
	{
		case VoxelDefinition::WallData::MenuType::None:
		case VoxelDefinition::WallData::MenuType::CityGates:
			return std::nullopt;
		case VoxelDefinition::WallData::MenuType::Crypt:
			return InteriorType::Crypt;
		case VoxelDefinition::WallData::MenuType::Dungeon:
			return InteriorType::Dungeon;
		case VoxelDefinition::WallData::MenuType::Equipment:
			return InteriorType::Equipment;
		case VoxelDefinition::WallData::MenuType::House:
			return InteriorType::House;
		case VoxelDefinition::WallData::MenuType::MagesGuild:
			return InteriorType::MagesGuild;
		case VoxelDefinition::WallData::MenuType::Noble:
			return InteriorType::Noble;
		case VoxelDefinition::WallData::MenuType::Palace:
			return InteriorType::Palace;
		case VoxelDefinition::WallData::MenuType::Tavern:
			return InteriorType::Tavern;
		case VoxelDefinition::WallData::MenuType::Temple:
			return InteriorType::Temple;
		case VoxelDefinition::WallData::MenuType::Tower:
			return InteriorType::Tower;
		default:
			DebugUnhandledReturnMsg(std::optional<InteriorType>, std::to_string(static_cast<int>(menuType)));
	};
}
