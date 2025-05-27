#include "MusicLibrary.h"
#include "MusicUtils.h"
#include "../Time/ArenaClockUtils.h"

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

const MusicDefinition *MusicUtils::getExteriorMusicDefinition(const WeatherDefinition &weatherDef, const Clock &clock, Random &random)
{
	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = nullptr;

	if (!ArenaClockUtils::nightMusicIsActive(clock))
	{
		musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicType::Weather, random,
			[&weatherDef](const MusicDefinition &def)
		{
			DebugAssert(def.type == MusicType::Weather);
			const WeatherMusicDefinition &weatherMusicDef = def.weather;
			return weatherMusicDef.weatherDef == weatherDef;
		});
	}
	else
	{
		musicDef = musicLibrary.getRandomMusicDefinition(MusicType::Night, random);
	}

	return musicDef;
}

const MusicDefinition *MusicUtils::getRandomDungeonMusicDefinition(Random &random)
{
	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(MusicType::Interior, random,
		[](const MusicDefinition &def)
	{
		DebugAssert(def.type == MusicType::Interior);
		const InteriorMusicDefinition &interiorMusicDef = def.interior;
		return interiorMusicDef.type == InteriorMusicType::Dungeon;
	});

	return musicDef;
}

const MusicDefinition *MusicUtils::getMainQuestCinematicGoodMusicDefinition(Random &random)
{
	const MusicLibrary &musicLibrary = MusicLibrary::getInstance();
	const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
		MusicType::Cinematic, random, [](const MusicDefinition &def)
	{
		DebugAssert(def.type == MusicType::Cinematic);
		const CinematicMusicDefinition &cinematicMusicDef = def.cinematic;
		return cinematicMusicDef.type == CinematicMusicType::DreamGood;
	});

	return musicDef;
}
