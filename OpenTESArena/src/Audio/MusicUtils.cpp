#include "MusicUtils.h"

#include "components/debug/Debug.h"

InteriorMusicType MusicUtils::getInteriorMusicType(ArenaTypes::InteriorType interiorType)
{
	if ((interiorType == ArenaTypes::InteriorType::Crypt) ||
		(interiorType == ArenaTypes::InteriorType::Dungeon) ||
		(interiorType == ArenaTypes::InteriorType::Tower))
	{
		return InteriorMusicType::Dungeon;
	}
	else if (interiorType == ArenaTypes::InteriorType::Equipment)
	{
		return InteriorMusicType::Equipment;
	}
	else if ((interiorType == ArenaTypes::InteriorType::House) ||
		(interiorType == ArenaTypes::InteriorType::Noble))
	{
		return InteriorMusicType::House;
	}
	else if (interiorType == ArenaTypes::InteriorType::MagesGuild)
	{
		return InteriorMusicType::MagesGuild;
	}
	else if (interiorType == ArenaTypes::InteriorType::Palace)
	{
		return InteriorMusicType::Palace;
	}
	else if (interiorType == ArenaTypes::InteriorType::Tavern)
	{
		return InteriorMusicType::Tavern;
	}
	else if (interiorType == ArenaTypes::InteriorType::Temple)
	{
		return InteriorMusicType::Temple;
	}
	else
	{
		DebugUnhandledReturnMsg(InteriorMusicType,
			std::to_string(static_cast<int>(interiorType)));
	}
}
