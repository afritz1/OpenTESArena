#ifndef MUSIC_TYPE_H
#define MUSIC_TYPE_H

// A unique identifier for each type of music.

// Prefer using a MusicType over a MusicName, because a MusicType translates to
// a list of MusicNames for more randomization during gameplay.

enum class MusicType
{
	ArabCityEnter,
	ArabTownEnter,
	ArabVillageEnter,
	CityEnter,
	Credits,
	Dungeon,
	Equipment,
	Evil,
	EvilIntro,
	Magic,
	Night,
	Overcast,
	Palace,
	PercIntro,
	Raining,
	Sheet,
	Sneaking,
	Snowing,
	Sunny,
	Swimming,
	Tavern,
	Temple,
	TownEnter,
	VillageEnter,
	Vision,
	WinGame
};

#endif