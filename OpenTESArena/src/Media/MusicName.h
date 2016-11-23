#ifndef MUSIC_NAME_H
#define MUSIC_NAME_H

// A unique identifier for each music file.

// There are two copies of each file: one in .XMI format and one in .XFM format.
// The .XMI version should be preferred, but .XFM must be used with DUNGEON1
// since DUNGEON1.XMI and DUNGEON5.XMI are duplicates, while DUNGEON1.XFM and 
// DUNGEON5.XFM are different. This was likely a mistake during Arena's development.

enum class MusicName
{
	ArabCityEnter,
	ArabTownEnter,
	ArabVillageEnter,
	CityEnter,
	Combat,
	Credits,
	Dungeon1,
	Dungeon2,
	Dungeon3,
	Dungeon4,
	Dungeon5,
	Equipment,
	Evil,
	EvilIntro,
	Magic,
	Night,
	Overcast,
	OverSnow,
	Palace,
	PercIntro,
	Raining,
	Sheet,
	Sneaking,
	Snowing,
	Square,
	SunnyDay,
	Swimming,
	Tavern,
	Temple,
	TownEnter,
	VillageEnter,
	Vision,
	WinGame
};

#endif
