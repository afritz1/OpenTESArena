#include <algorithm>
#include <limits>

#include "ArenaWeatherUtils.h"
#include "WeatherDefinition.h"
#include "WeatherInstance.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

namespace
{
	double MakeSecondsUntilNextLightning(Random &random)
	{
		return ArenaWeatherUtils::THUNDERSTORM_FLASH_SECONDS + (random.nextReal() * 5.0);
	}

	Radians MakeLightningBoltAngle(Random &random)
	{
		return random.nextReal() * Constants::TwoPi;
	}
}

void WeatherInstance::RainInstance::Raindrop::init(double xPercent, double yPercent)
{
	this->xPercent = xPercent;
	this->yPercent = yPercent;
}

void WeatherInstance::RainInstance::Thunderstorm::init(Random &random)
{
	this->secondsSincePrevLightning = std::numeric_limits<double>::infinity();
	this->secondsUntilNextLightning = MakeSecondsUntilNextLightning(random);
	this->lightningBoltAngle = 0.0;
}

double WeatherInstance::RainInstance::Thunderstorm::getFlashPercent() const
{
	const double percent = this->secondsSincePrevLightning / ArenaWeatherUtils::THUNDERSTORM_FLASH_SECONDS;
	return std::clamp(1.0 - percent, 0.0, 1.0);
}

bool WeatherInstance::RainInstance::Thunderstorm::isLightningBoltVisible() const
{
	return this->secondsSincePrevLightning <= ArenaWeatherUtils::THUNDERSTORM_BOLT_SECONDS;
}

void WeatherInstance::RainInstance::Thunderstorm::update(double dt, Random &random)
{
	this->secondsSincePrevLightning += dt;
	this->secondsUntilNextLightning -= dt;
	if (this->secondsUntilNextLightning <= 0.0)
	{
		this->secondsSincePrevLightning = 0.0;
		this->secondsUntilNextLightning = MakeSecondsUntilNextLightning(random);
		this->lightningBoltAngle = MakeLightningBoltAngle(random);

		// @todo: signal lightning bolt to generate + appear, and audio to play.
	}
}

void WeatherInstance::RainInstance::init(bool isThunderstorm, Random &random)
{
	this->raindrops.init(ArenaWeatherUtils::RAINDROP_COUNT);
	for (int i = 0; i < this->raindrops.getCount(); i++)
	{
		Raindrop &raindrop = this->raindrops.get(i);
		raindrop.init(random.nextReal(), random.nextReal());
	}

	if (isThunderstorm)
	{
		this->thunderstorm = std::make_optional<Thunderstorm>();
		this->thunderstorm->init(random);
	}
	else
	{
		this->thunderstorm = std::nullopt;
	}
}

void WeatherInstance::RainInstance::update(double dt, double aspectRatio, Random &random)
{
	// Animate raindrops.
	for (int i = 0; i < this->raindrops.getCount(); i++)
	{
		Raindrop &raindrop = this->raindrops.get(i);

		const bool canBeRestarted = (raindrop.xPercent < 0.0) || (raindrop.yPercent >= 1.0);
		if (canBeRestarted)
		{
			// Pick a screen edge to spawn at. This involves the aspect ratio so that drops are properly distributed.
			const double topEdgeLength = aspectRatio;
			constexpr double rightEdgeLength = 1.0;
			const double topEdgePercent = topEdgeLength / (topEdgeLength + rightEdgeLength);
			if (random.nextReal() <= topEdgePercent)
			{
				// Top edge.
				raindrop.xPercent = random.nextReal();
				raindrop.yPercent = 0.0;
			}
			else
			{
				// Right edge.
				raindrop.xPercent = 1.0;
				raindrop.yPercent = random.nextReal();
			}
		}
		else
		{
			// Screens per second the raindrop travels on each axis at the original game's aspect ratio.
			constexpr double baseVelocityPercentX = static_cast<double>(ArenaWeatherUtils::RAINDROP_VELOCITY_X) /
				static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
			constexpr double baseVelocityPercentY = static_cast<double>(ArenaWeatherUtils::RAINDROP_VELOCITY_Y) /
				static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT);

			// The particle's horizontal movement is aspect-ratio-dependent.
			const double aspectRatioMultiplierX = ArenaRenderUtils::ASPECT_RATIO / aspectRatio;
			const double deltaPercentX = (baseVelocityPercentX * aspectRatioMultiplierX) * dt;
			const double deltaPercentY = baseVelocityPercentY * dt;
			raindrop.xPercent += deltaPercentX;
			raindrop.yPercent += deltaPercentY;
		}
	}

	if (this->thunderstorm.has_value())
	{
		// Animate thunderstorm.
		this->thunderstorm->update(dt, random);
	}
}

WeatherInstance::WeatherInstance()
{
	this->type = static_cast<WeatherInstance::Type>(-1);
}

void WeatherInstance::init(const WeatherDefinition &weatherDef, Random &random)
{
	const WeatherDefinition::Type weatherDefType = weatherDef.getType();

	if ((weatherDefType == WeatherDefinition::Type::Clear) ||
		(weatherDefType == WeatherDefinition::Type::Overcast))
	{
		this->type = WeatherInstance::Type::None;
	}
	else if (weatherDefType == WeatherDefinition::Type::Rain)
	{
		this->type = WeatherInstance::Type::Rain;

		const WeatherDefinition::RainDefinition &rainDef = weatherDef.getRain();
		this->rain.init(rainDef.thunderstorm, random);
	}
	else if (weatherDefType == WeatherDefinition::Type::Snow)
	{
		this->type = WeatherInstance::Type::Snow;
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(weatherDefType)));
	}
}

WeatherInstance::Type WeatherInstance::getType() const
{
	return this->type;
}

const WeatherInstance::RainInstance &WeatherInstance::getRain() const
{
	DebugAssert(this->type == WeatherInstance::Type::Rain);
	return this->rain;
}

void WeatherInstance::update(double dt, double aspectRatio, Random &random)
{
	if (this->type == WeatherInstance::Type::Rain)
	{
		this->rain.update(dt, aspectRatio, random);
	}
}
