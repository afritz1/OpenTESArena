#include "ArenaWeatherUtils.h"
#include "WeatherDefinition.h"

#include "components/debug/Debug.h"

void WeatherDefinition::OvercastDefinition::init(bool heavyFog)
{
	this->heavyFog = heavyFog;
}

void WeatherDefinition::RainDefinition::init(bool thunderstorm)
{
	this->thunderstorm = thunderstorm;
}

void WeatherDefinition::SnowDefinition::init(bool overcast, bool heavyFog)
{
	this->overcast = overcast;
	this->heavyFog = heavyFog;
}

WeatherDefinition::WeatherDefinition()
{
	this->type = static_cast<WeatherDefinition::Type>(-1);
}

void WeatherDefinition::initClear()
{
	this->type = WeatherDefinition::Type::Clear;
}

void WeatherDefinition::initOvercast(bool heavyFog)
{
	this->type = WeatherDefinition::Type::Overcast;
	this->overcast.init(heavyFog);
}

void WeatherDefinition::initRain(bool thunderstorm)
{
	this->type = WeatherDefinition::Type::Rain;
	this->rain.init(thunderstorm);
}

void WeatherDefinition::initSnow(bool overcast, bool heavyFog)
{
	this->type = WeatherDefinition::Type::Snow;
	this->snow.init(overcast, heavyFog);
}

void WeatherDefinition::initFromClassic(ArenaTypes::WeatherType weatherType, int currentDay, ArenaRandom &random)
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
		const bool overcast = (weatherType == ArenaTypes::WeatherType::SnowOvercast) ||
			(weatherType == ArenaTypes::WeatherType::SnowOvercast2);
		const bool heavyFog = ArenaWeatherUtils::fogIsHeavy(currentDay);
		this->initSnow(overcast, heavyFog);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(weatherType)));
	}
}

WeatherDefinition::Type WeatherDefinition::getType() const
{
	return this->type;
}

double WeatherDefinition::getFogDistance() const
{
	// Arbitrary fog distances.
	if (this->type == WeatherDefinition::Type::Clear)
	{
		return 100.0;
	}
	else if (this->type == WeatherDefinition::Type::Overcast)
	{
		return this->overcast.heavyFog ? 25.0 : 40.0;
	}
	else if (this->type == WeatherDefinition::Type::Rain)
	{
		return this->rain.thunderstorm ? 40.0 : 50.0;
	}
	else if (this->type == WeatherDefinition::Type::Snow)
	{
		return this->snow.heavyFog ? 20.0 : 35.0;
	}
	else
	{
		DebugUnhandledReturnMsg(double, std::to_string(static_cast<int>(this->type)));
	}
}

const WeatherDefinition::OvercastDefinition &WeatherDefinition::getOvercast() const
{
	DebugAssert(this->type == WeatherDefinition::Type::Overcast);
	return this->overcast;
}

const WeatherDefinition::RainDefinition &WeatherDefinition::getRain() const
{
	DebugAssert(this->type == WeatherDefinition::Type::Rain);
	return this->rain;
}

const WeatherDefinition::SnowDefinition &WeatherDefinition::getSnow() const
{
	DebugAssert(this->type == WeatherDefinition::Type::Snow);
	return this->snow;
}
