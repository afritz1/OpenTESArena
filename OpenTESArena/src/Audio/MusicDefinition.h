#ifndef MUSIC_DEFINITION_H
#define MUSIC_DEFINITION_H

#include <string>

#include "../Assets/ArenaTypes.h"
#include "../Weather/WeatherDefinition.h"

enum class CinematicMusicType
{
	Intro,
	DreamGood,
	DreamBad,
	Ending
};

struct CinematicMusicDefinition
{
	CinematicMusicType type;

	void init(CinematicMusicType type);
};

enum class InteriorMusicType
{
	Dungeon,
	Equipment,
	House,
	MagesGuild,
	Palace,
	Tavern,
	Temple
};

struct InteriorMusicDefinition
{
	InteriorMusicType type;

	void init(InteriorMusicType type);
};

struct JingleMusicDefinition
{
	ArenaTypes::CityType cityType;
	ArenaTypes::ClimateType climateType;

	void init(ArenaTypes::CityType cityType, ArenaTypes::ClimateType climateType);
};

struct WeatherMusicDefinition
{
	WeatherDefinition weatherDef;

	void init(const WeatherDefinition &weatherDef);
};

enum class MusicType
{
	CharacterCreation,
	Cinematic,
	Interior,
	Jingle,
	MainMenu,
	Night,
	Swimming,
	Weather
};

struct MusicDefinition
{
	std::string filename;
	MusicType type;

	// Only one is active at a time, given by 'type' (no union because of WeatherMusicDefinition).
	CinematicMusicDefinition cinematic;
	InteriorMusicDefinition interior;
	JingleMusicDefinition jingle;
	WeatherMusicDefinition weather;

	void initCharacterCreation(const std::string &filename);
	void initCinematic(const std::string &filename, CinematicMusicType type);
	void initInterior(const std::string &filename, InteriorMusicType type);
	void initJingle(const std::string &filename, ArenaTypes::CityType cityType, ArenaTypes::ClimateType climateType);
	void initMainMenu(const std::string &filename);
	void initNight(const std::string &filename);
	void initSwimming(const std::string &filename);
	void initWeather(const std::string &filename, const WeatherDefinition &weatherDef);
};

#endif
