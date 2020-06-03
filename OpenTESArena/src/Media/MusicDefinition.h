#ifndef MUSIC_DEFINITION_H
#define MUSIC_DEFINITION_H

#include <string>

#include "../World/LocationDefinition.h"

enum class ClimateType;
enum class WeatherType;

class MusicDefinition
{
public:
	enum class Type
	{
		CharacterCreation,
		Cinematic,
		Dungeon,
		Interior,
		Jingle,
		MainMenu,
		Night,
		Swimming,
		Weather
	};

	struct CinematicMusicDefinition
	{
		enum class Type
		{
			Intro,
			DreamGood,
			DreamBad,
			Ending
		};

		CinematicMusicDefinition::Type type;

		void init(CinematicMusicDefinition::Type type);
	};

	struct InteriorMusicDefinition
	{
		enum class Type
		{
			Equipment,
			House,
			MagesGuild,
			Palace,
			Tavern,
			Temple
		};

		InteriorMusicDefinition::Type type;

		void init(InteriorMusicDefinition::Type type);
	};

	struct JingleMusicDefinition
	{
		LocationDefinition::CityDefinition::Type cityType;
		ClimateType climateType;

		void init(LocationDefinition::CityDefinition::Type cityType, ClimateType climateType);
	};

	struct WeatherMusicDefinition
	{
		WeatherType type;

		void init(WeatherType type);
	};
private:
	std::string filename;
	Type type;

	union
	{
		CinematicMusicDefinition cinematic;
		InteriorMusicDefinition interior;
		JingleMusicDefinition jingle;
		WeatherMusicDefinition weather;
	};

	void init(std::string &&filename, Type type);
public:
	void initCharacterCreation(std::string &&filename);
	void initCinematic(std::string &&filename, CinematicMusicDefinition::Type type);
	void initDungeon(std::string &&filename);
	void initInterior(std::string &&filename, InteriorMusicDefinition::Type type);
	void initJingle(std::string &&filename, LocationDefinition::CityDefinition::Type cityType,
		ClimateType climateType);
	void initMainMenu(std::string &&filename);
	void initNight(std::string &&filename);
	void initSwimming(std::string &&filename);
	void initWeather(std::string &&filename, WeatherType type);

	const std::string &getFilename() const;
	Type getType() const;

	const CinematicMusicDefinition &getCinematicMusicDefinition() const;
	const InteriorMusicDefinition &getInteriorMusicDefinition() const;
	const JingleMusicDefinition &getJingleMusicDefinition() const;
	const WeatherMusicDefinition &getWeatherMusicDefinition() const;
};

#endif
