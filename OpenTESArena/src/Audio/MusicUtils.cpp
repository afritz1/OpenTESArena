#include "MusicUtils.h"

#include "components/debug/Debug.h"

MusicDefinition::InteriorMusicDefinition::Type MusicUtils::getInteriorMusicType(ArenaTypes::InteriorType interiorType)
{
	if ((interiorType == ArenaTypes::InteriorType::Crypt) ||
		(interiorType == ArenaTypes::InteriorType::Dungeon) ||
		(interiorType == ArenaTypes::InteriorType::Tower))
	{
		return MusicDefinition::InteriorMusicDefinition::Type::Dungeon;
	}
	else if (interiorType == ArenaTypes::InteriorType::Equipment)
	{
		return MusicDefinition::InteriorMusicDefinition::Type::Equipment;
	}
	else if ((interiorType == ArenaTypes::InteriorType::House) ||
		(interiorType == ArenaTypes::InteriorType::Noble))
	{
		return MusicDefinition::InteriorMusicDefinition::Type::House;
	}
	else if (interiorType == ArenaTypes::InteriorType::MagesGuild)
	{
		return MusicDefinition::InteriorMusicDefinition::Type::MagesGuild;
	}
	else if (interiorType == ArenaTypes::InteriorType::Palace)
	{
		return MusicDefinition::InteriorMusicDefinition::Type::Palace;
	}
	else if (interiorType == ArenaTypes::InteriorType::Tavern)
	{
		return MusicDefinition::InteriorMusicDefinition::Type::Tavern;
	}
	else if (interiorType == ArenaTypes::InteriorType::Temple)
	{
		return MusicDefinition::InteriorMusicDefinition::Type::Temple;
	}
	else
	{
		DebugUnhandledReturnMsg(MusicDefinition::InteriorMusicDefinition::Type,
			std::to_string(static_cast<int>(interiorType)));
	}
}
