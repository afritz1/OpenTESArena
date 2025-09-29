#include "ArenaWeatherUtils.h"
#include "WeatherDefinition.h"

#include "components/debug/Debug.h"

void WeatherOvercastDefinition::init(bool heavyFog)
{
	this->heavyFog = heavyFog;
}

void WeatherRainDefinition::init(bool thunderstorm)
{
	this->thunderstorm = thunderstorm;
}

void WeatherSnowDefinition::init(bool overcast, bool heavyFog)
{
	this->overcast = overcast;
	this->heavyFog = heavyFog;
}

WeatherDefinition::WeatherDefinition()
{
	this->type = static_cast<WeatherType>(-1);
}

bool WeatherDefinition::operator==(const WeatherDefinition &other) const
{
	if (this->type != other.type)
	{
		return false;
	}

	if (this->type == WeatherType::Clear)
	{
		return true;
	}
	else if (this->type == WeatherType::Overcast)
	{
		const WeatherOvercastDefinition &aOvercast = this->overcast;
		const WeatherOvercastDefinition &bOvercast = other.overcast;
		return aOvercast.heavyFog == bOvercast.heavyFog;
	}
	else if (this->type == WeatherType::Rain)
	{
		const WeatherRainDefinition &aRain = this->rain;
		const WeatherRainDefinition &bRain = other.rain;
		return aRain.thunderstorm == bRain.thunderstorm;
	}
	else if (this->type == WeatherType::Snow)
	{
		const WeatherSnowDefinition &aSnow = this->snow;
		const WeatherSnowDefinition &bSnow = other.snow;
		return (aSnow.overcast == bSnow.overcast) && (aSnow.heavyFog == bSnow.heavyFog);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(type)));
	}
}

void WeatherDefinition::initClear()
{
	this->type = WeatherType::Clear;
}

void WeatherDefinition::initOvercast(bool heavyFog)
{
	this->type = WeatherType::Overcast;
	this->overcast.init(heavyFog);
}

void WeatherDefinition::initRain(bool thunderstorm)
{
	this->type = WeatherType::Rain;
	this->rain.init(thunderstorm);
}

void WeatherDefinition::initSnow(bool overcast, bool heavyFog)
{
	this->type = WeatherType::Snow;
	this->snow.init(overcast, heavyFog);
}

void WeatherDefinition::initFromClassic(ArenaWeatherType weatherType, int currentDay, Random &random)
{
	if (ArenaWeatherUtils::isClear(weatherType))
	{
		this->initClear();
	}
	else if (ArenaWeatherUtils::isOvercast(weatherType))
	{
		const bool heavyFog = ArenaWeatherUtils::fogIsHeavy(currentDay);
		this->initOvercast(heavyFog);
	}
	else if (ArenaWeatherUtils::isRain(weatherType))
	{
		const bool thunderstorm = ArenaWeatherUtils::rainIsThunderstorm(random);
		this->initRain(thunderstorm);
	}
	else if (ArenaWeatherUtils::isSnow(weatherType))
	{
		const bool overcast = (weatherType == ArenaWeatherType::SnowOvercast) || (weatherType == ArenaWeatherType::SnowOvercast2);
		const bool heavyFog = ArenaWeatherUtils::fogIsHeavy(currentDay);
		this->initSnow(overcast, heavyFog);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(weatherType)));
	}
}
