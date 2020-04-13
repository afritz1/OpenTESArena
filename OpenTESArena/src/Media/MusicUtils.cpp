#include <array>

#include "MusicFile.h"
#include "MusicName.h"
#include "MusicUtils.h"
#include "../Math/Random.h"

#include "components/debug/Debug.h"

MusicName MusicUtils::getExteriorMusicName(WeatherType weatherType)
{
	return MusicFile::fromWeather(weatherType);
}

MusicName MusicUtils::getDungeonMusicName(Random &random)
{
	const std::array<MusicName, 5> DungeonMusics =
	{
		MusicName::Dungeon1,
		MusicName::Dungeon2,
		MusicName::Dungeon3,
		MusicName::Dungeon4,
		MusicName::Dungeon5
	};

	const int index = random.next(static_cast<int>(DungeonMusics.size()));
	DebugAssertIndex(DungeonMusics, index);
	return DungeonMusics[index];
}

MusicName MusicUtils::getInteriorMusicName(const std::string &mifName, Random &random)
{
	// Check against all of the non-dungeon interiors first.
	const bool isEquipmentStore = mifName.find("EQUIP") != std::string::npos;
	const bool isHouse = (mifName.find("BS") != std::string::npos) ||
		(mifName.find("NOBLE") != std::string::npos);
	const bool isMagesGuild = mifName.find("MAGE") != std::string::npos;
	const bool isPalace = (mifName.find("PALACE") != std::string::npos) ||
		(mifName.find("TOWNPAL") != std::string::npos) ||
		(mifName.find("VILPAL") != std::string::npos);
	const bool isTavern = mifName.find("TAVERN") != std::string::npos;
	const bool isTemple = mifName.find("TEMPLE") != std::string::npos;

	if (isEquipmentStore)
	{
		return MusicName::Equipment;
	}
	else if (isHouse)
	{
		return MusicName::Sneaking;
	}
	else if (isMagesGuild)
	{
		return MusicName::Magic;
	}
	else if (isPalace)
	{
		return MusicName::Palace;
	}
	else if (isTavern)
	{
		const std::array<MusicName, 2> TavernMusics =
		{
			MusicName::Square,
			MusicName::Tavern
		};

		const int index = random.next(static_cast<int>(TavernMusics.size()));
		DebugAssertIndex(TavernMusics, index);
		return TavernMusics[index];
	}
	else if (isTemple)
	{
		return MusicName::Temple;
	}
	else
	{
		// Dungeon.
		return MusicUtils::getDungeonMusicName(random);
	}
}
