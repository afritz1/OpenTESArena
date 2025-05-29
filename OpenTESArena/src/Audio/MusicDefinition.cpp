#include "MusicDefinition.h"

#include "components/debug/Debug.h"

void CinematicMusicDefinition::init(CinematicMusicType type)
{
	this->type = type;
}

void InteriorMusicDefinition::init(InteriorMusicType type)
{
	this->type = type;
}

void JingleMusicDefinition::init(ArenaCityType cityType, ArenaClimateType climateType)
{
	this->cityType = cityType;
	this->climateType = climateType;
}

void WeatherMusicDefinition::init(const WeatherDefinition &weatherDef)
{
	this->weatherDef = weatherDef;
}

void MusicDefinition::initCharacterCreation(const std::string &filename)
{
	this->filename = filename;
	this->type = MusicType::CharacterCreation;
}

void MusicDefinition::initCinematic(const std::string &filename, CinematicMusicType type)
{
	this->filename = filename;
	this->type = MusicType::Cinematic;
	this->cinematic.init(type);
}

void MusicDefinition::initInterior(const std::string &filename, InteriorMusicType type)
{
	this->filename = filename;
	this->type = MusicType::Interior;
	this->interior.init(type);
}

void MusicDefinition::initJingle(const std::string &filename, ArenaCityType cityType, ArenaClimateType climateType)
{
	this->filename = filename;
	this->type = MusicType::Jingle;
	this->jingle.init(cityType, climateType);
}

void MusicDefinition::initMainMenu(const std::string &filename)
{
	this->filename = filename;
	this->type = MusicType::MainMenu;
}

void MusicDefinition::initNight(const std::string &filename)
{
	this->filename = filename;
	this->type = MusicType::Night;
}

void MusicDefinition::initSwimming(const std::string &filename)
{
	this->filename = filename;
	this->type = MusicType::Swimming;
}

void MusicDefinition::initWeather(const std::string &filename, const WeatherDefinition &weatherDef)
{
	this->filename = filename;
	this->type = MusicType::Weather;
	this->weather.init(weatherDef);
}
