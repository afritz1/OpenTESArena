#include "MusicDefinition.h"

#include "components/debug/Debug.h"

void MusicDefinition::CinematicMusicDefinition::init(CinematicMusicDefinition::Type type)
{
	this->type = type;
}

void MusicDefinition::InteriorMusicDefinition::init(InteriorMusicDefinition::Type type)
{
	this->type = type;
}

void MusicDefinition::JingleMusicDefinition::init(ArenaTypes::CityType cityType, ClimateType climateType)
{
	this->cityType = cityType;
	this->climateType = climateType;
}

void MusicDefinition::WeatherMusicDefinition::init(WeatherType type)
{
	this->type = type;
}

void MusicDefinition::init(std::string &&filename, Type type)
{
	this->filename = std::move(filename);
	this->type = type;
}

void MusicDefinition::initCharacterCreation(std::string &&filename)
{
	this->init(std::move(filename), Type::CharacterCreation);
}

void MusicDefinition::initCinematic(std::string &&filename, CinematicMusicDefinition::Type type)
{
	this->init(std::move(filename), Type::Cinematic);
	this->cinematic.init(type);
}

void MusicDefinition::initDungeon(std::string &&filename)
{
	this->init(std::move(filename), Type::Dungeon);
}

void MusicDefinition::initInterior(std::string &&filename, InteriorMusicDefinition::Type type)
{
	this->init(std::move(filename), Type::Interior);
	this->interior.init(type);
}

void MusicDefinition::initJingle(std::string &&filename, ArenaTypes::CityType cityType,
	ClimateType climateType)
{
	this->init(std::move(filename), Type::Jingle);
	this->jingle.init(cityType, climateType);
}

void MusicDefinition::initMainMenu(std::string &&filename)
{
	this->init(std::move(filename), Type::MainMenu);
}

void MusicDefinition::initNight(std::string &&filename)
{
	this->init(std::move(filename), Type::Night);
}

void MusicDefinition::initSwimming(std::string &&filename)
{
	this->init(std::move(filename), Type::Swimming);
}

void MusicDefinition::initWeather(std::string &&filename, WeatherType type)
{
	this->init(std::move(filename), Type::Weather);
	this->weather.init(type);
}

const std::string &MusicDefinition::getFilename() const
{
	return this->filename;
}

MusicDefinition::Type MusicDefinition::getType() const
{
	return this->type;
}

const MusicDefinition::CinematicMusicDefinition &MusicDefinition::getCinematicMusicDefinition() const
{
	DebugAssert(this->type == Type::Cinematic);
	return this->cinematic;
}

const MusicDefinition::InteriorMusicDefinition &MusicDefinition::getInteriorMusicDefinition() const
{
	DebugAssert(this->type == Type::Interior);
	return this->interior;
}

const MusicDefinition::JingleMusicDefinition &MusicDefinition::getJingleMusicDefinition() const
{
	DebugAssert(this->type == Type::Jingle);
	return this->jingle;
}

const MusicDefinition::WeatherMusicDefinition &MusicDefinition::getWeatherMusicDefinition() const
{
	DebugAssert(this->type == Type::Weather);
	return this->weather;
}
