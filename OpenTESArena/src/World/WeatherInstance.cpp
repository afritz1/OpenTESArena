#include <algorithm>
#include <limits>

#include "ArenaWeatherUtils.h"
#include "WeatherDefinition.h"
#include "WeatherInstance.h"
#include "../Assets/ArenaSoundName.h"
#include "../Audio/AudioManager.h"
#include "../Game/ArenaClockUtils.h"
#include "../Game/Clock.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Rendering/ArenaRenderUtils.h"

#include "components/debug/Debug.h"

namespace
{
	bool IsDuringThunderstorm(const Clock &clock)
	{
		// Starts in the evening, ends in the morning.
		const double seconds = clock.getPreciseTotalSeconds();
		const double startSeconds = ArenaClockUtils::ThunderstormStart.getPreciseTotalSeconds();
		const double endSeconds = ArenaClockUtils::ThunderstormEnd.getPreciseTotalSeconds();
		return (seconds >= startSeconds) || (seconds < endSeconds);
	}

	double MakeSecondsUntilNextLightning(Random &random)
	{
		return ArenaWeatherUtils::THUNDERSTORM_SKY_FLASH_SECONDS + (random.nextReal() * 5.0);
	}

	Radians MakeLightningBoltAngle(Random &random)
	{
		return random.nextReal() * Constants::TwoPi;
	}

	bool MakeSnowflakeDirection(Random &random)
	{
		return (random.next() % 2) != 0;
	}
}

void WeatherInstance::Particle::init(double xPercent, double yPercent)
{
	this->xPercent = xPercent;
	this->yPercent = yPercent;
}

void WeatherInstance::RainInstance::Thunderstorm::init(Buffer<uint8_t> &&flashColors,
	Buffer<TextureBuilderIdGroup> &&lightningBoltTextureBuilderIDs, bool active, Random &random)
{
	this->flashColors = std::move(flashColors);
	this->lightningBoltTextureBuilderIDs = std::move(lightningBoltTextureBuilderIDs);
	this->secondsSincePrevLightning = std::numeric_limits<double>::infinity();
	this->secondsUntilNextLightning = MakeSecondsUntilNextLightning(random);
	this->lightningBoltAngle = 0.0;
	this->lightningBoltGroupIndex = -1;
	this->active = active;
}

std::optional<double> WeatherInstance::RainInstance::Thunderstorm::getFlashPercent() const
{
	const double percent = this->secondsSincePrevLightning / ArenaWeatherUtils::THUNDERSTORM_SKY_FLASH_SECONDS;
	if ((percent >= 0.0) && (percent < 1.0))
	{
		return percent;
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<double> WeatherInstance::RainInstance::Thunderstorm::getLightningBoltPercent() const
{
	const double percent = this->secondsSincePrevLightning / ArenaWeatherUtils::THUNDERSTORM_BOLT_SECONDS;
	if ((percent >= 0.0) && (percent < 1.0))
	{
		return percent;
	}
	else
	{
		return std::nullopt;
	}
}

void WeatherInstance::RainInstance::Thunderstorm::update(double dt, const Clock &clock,
	Random &random, AudioManager &audioManager)
{
	this->active = IsDuringThunderstorm(clock);

	if (this->active)
	{
		this->secondsSincePrevLightning += dt;
		this->secondsUntilNextLightning -= dt;
		if (this->secondsUntilNextLightning <= 0.0)
		{
			this->secondsSincePrevLightning = 0.0;
			this->secondsUntilNextLightning = MakeSecondsUntilNextLightning(random);
			this->lightningBoltAngle = MakeLightningBoltAngle(random);
			this->lightningBoltGroupIndex = random.next(this->lightningBoltTextureBuilderIDs.getCount());

			const std::string &soundFilename = ArenaSoundName::Thunder;
			audioManager.playSound(soundFilename);
		}
	}
}

void WeatherInstance::RainInstance::init(bool isThunderstorm, const Clock &clock,
	Buffer<uint8_t> &&flashColors, Random &random, TextureManager &textureManager)
{
	this->particles.init(ArenaWeatherUtils::RAINDROP_TOTAL_COUNT);
	for (int i = 0; i < this->particles.getCount(); i++)
	{
		Particle &particle = this->particles.get(i);
		particle.init(random.nextReal(), random.nextReal());
	}

	if (isThunderstorm)
	{
		this->thunderstorm = std::make_optional<Thunderstorm>();

		Buffer<TextureBuilderIdGroup> lightningBoltTextureBuilderIDs =
			ArenaWeatherUtils::makeLightningBoltTextureBuilderIDs(textureManager);
		this->thunderstorm->init(std::move(flashColors), std::move(lightningBoltTextureBuilderIDs),
			IsDuringThunderstorm(clock), random);
	}
	else
	{
		this->thunderstorm = std::nullopt;
	}
}

void WeatherInstance::RainInstance::update(double dt, const Clock &clock, double aspectRatio,
	Random &random, AudioManager &audioManager)
{
	auto animateRaindropRange = [this, dt, aspectRatio, &random](int startIndex, int endIndex,
		double velocityPercentX, double velocityPercentY)
	{
		for (int i = startIndex; i < endIndex; i++)
		{
			Particle &particle = this->particles.get(i);
			const bool canBeRestarted = (particle.xPercent < 0.0) || (particle.yPercent >= 1.0);
			if (canBeRestarted)
			{
				// Pick a screen edge to spawn at. This involves the aspect ratio so drops are properly distributed.
				const double topEdgeLength = aspectRatio;
				constexpr double rightEdgeLength = 1.0;
				const double topEdgePercent = topEdgeLength / (topEdgeLength + rightEdgeLength);
				if (random.nextReal() <= topEdgePercent)
				{
					// Top edge.
					particle.xPercent = random.nextReal();
					particle.yPercent = 0.0;
				}
				else
				{
					// Right edge.
					particle.xPercent = 1.0;
					particle.yPercent = random.nextReal();
				}
			}
			else
			{
				// The particle's horizontal movement is aspect-ratio-dependent.
				const double aspectRatioMultiplierX = ArenaRenderUtils::ASPECT_RATIO / aspectRatio;
				const double deltaPercentX = (velocityPercentX * aspectRatioMultiplierX) * dt;
				const double deltaPercentY = velocityPercentY * dt;
				particle.xPercent += deltaPercentX;
				particle.yPercent += deltaPercentY;
			}
		}
	};

	constexpr int fastStartIndex = 0;
	constexpr int fastEndIndex = ArenaWeatherUtils::RAINDROP_FAST_COUNT;
	constexpr int mediumStartIndex = fastEndIndex;
	constexpr int mediumEndIndex = mediumStartIndex + ArenaWeatherUtils::RAINDROP_MEDIUM_COUNT;
	constexpr int slowStartIndex = mediumEndIndex;
	constexpr int slowEndIndex = slowStartIndex + ArenaWeatherUtils::RAINDROP_SLOW_COUNT;

	constexpr double arenaScreenWidthReal = static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
	constexpr double arenaScreenHeightReal = static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT);
	constexpr int arenaFramesPerSecond = ArenaRenderUtils::FRAMES_PER_SECOND;

	constexpr double fastVelocityPercentX = static_cast<double>(
		ArenaWeatherUtils::RAINDROP_FAST_PIXELS_PER_FRAME_X * arenaFramesPerSecond) / arenaScreenWidthReal;
	constexpr double fastVelocityPercentY = static_cast<double>(
		ArenaWeatherUtils::RAINDROP_FAST_PIXELS_PER_FRAME_Y * arenaFramesPerSecond) / arenaScreenHeightReal;
	constexpr double mediumVelocityPercentX = static_cast<double>(
		ArenaWeatherUtils::RAINDROP_MEDIUM_PIXELS_PER_FRAME_X * arenaFramesPerSecond) / arenaScreenWidthReal;
	constexpr double mediumVelocityPercentY = static_cast<double>(
		ArenaWeatherUtils::RAINDROP_MEDIUM_PIXELS_PER_FRAME_Y * arenaFramesPerSecond) / arenaScreenHeightReal;
	constexpr double slowVelocityPercentX = static_cast<double>(
		ArenaWeatherUtils::RAINDROP_SLOW_PIXELS_PER_FRAME_X * arenaFramesPerSecond) / arenaScreenWidthReal;
	constexpr double slowVelocityPercentY = static_cast<double>(
		ArenaWeatherUtils::RAINDROP_SLOW_PIXELS_PER_FRAME_Y * arenaFramesPerSecond) / arenaScreenHeightReal;

	animateRaindropRange(fastStartIndex, fastEndIndex, fastVelocityPercentX, fastVelocityPercentY);
	animateRaindropRange(mediumStartIndex, mediumEndIndex, mediumVelocityPercentX, mediumVelocityPercentY);
	animateRaindropRange(slowStartIndex, slowEndIndex, slowVelocityPercentX, slowVelocityPercentY);

	if (this->thunderstorm.has_value())
	{
		this->thunderstorm->update(dt, clock, random, audioManager);
	}
}

void WeatherInstance::SnowInstance::init(Random &random)
{
	this->particles.init(ArenaWeatherUtils::SNOWFLAKE_TOTAL_COUNT);
	for (int i = 0; i < this->particles.getCount(); i++)
	{
		Particle &particle = this->particles.get(i);
		particle.init(random.nextReal(), random.nextReal());
	}

	this->directions.init(this->particles.getCount());
	for (int i = 0; i < this->directions.getCount(); i++)
	{
		this->directions.set(i, MakeSnowflakeDirection(random));
	}

	this->lastDirectionChangeSeconds.init(this->particles.getCount());
	this->lastDirectionChangeSeconds.fill(0.0);
}

void WeatherInstance::SnowInstance::update(double dt, double aspectRatio, Random &random)
{
	auto animateSnowflakeRange = [this, dt, aspectRatio, &random](int startIndex, int endIndex,
		double velocityPercentX, double velocityPercentY)
	{
		for (int i = startIndex; i < endIndex; i++)
		{
			Particle &particle = this->particles.get(i);
			const bool canBeRestarted = particle.yPercent >= 1.0;
			if (canBeRestarted)
			{
				// Pick somewhere on the top edge to spawn.
				particle.xPercent = random.nextReal();
				particle.yPercent = 0.0;

				this->directions.set(i, MakeSnowflakeDirection(random));
			}
			else
			{
				double &secondsSinceDirectionChange = this->lastDirectionChangeSeconds.get(i);
				secondsSinceDirectionChange += dt;

				// The snowflake gets a chance to change direction a few times a second.
				if (secondsSinceDirectionChange >= ArenaWeatherUtils::SNOWFLAKE_MIN_SECONDS_BEFORE_DIRECTION_CHANGE)
				{
					secondsSinceDirectionChange = std::fmod(
						secondsSinceDirectionChange, ArenaWeatherUtils::SNOWFLAKE_MIN_SECONDS_BEFORE_DIRECTION_CHANGE);

					if (ArenaWeatherUtils::shouldSnowflakeChangeDirection(random))
					{
						this->directions.set(i, !this->directions.get(i));
					}
				}

				const double directionX = this->directions.get(i) ? 1.0 : -1.0;

				// The particle's horizontal movement is aspect-ratio-dependent.
				const double aspectRatioMultiplierX = ArenaRenderUtils::ASPECT_RATIO / aspectRatio;

				// This seems to make snowflakes move at a closer speed to the original game.
				constexpr double velocityCorrectionX = 0.50;

				const double deltaPercentX = (velocityPercentX * directionX * aspectRatioMultiplierX * velocityCorrectionX) * dt;
				const double deltaPercentY = velocityPercentY * dt;
				particle.xPercent += deltaPercentX;
				particle.yPercent += deltaPercentY;
			}
		}
	};

	constexpr int fastStartIndex = 0;
	constexpr int fastEndIndex = ArenaWeatherUtils::SNOWFLAKE_FAST_COUNT;
	constexpr int mediumStartIndex = fastEndIndex;
	constexpr int mediumEndIndex = mediumStartIndex + ArenaWeatherUtils::SNOWFLAKE_MEDIUM_COUNT;
	constexpr int slowStartIndex = mediumEndIndex;
	constexpr int slowEndIndex = slowStartIndex + ArenaWeatherUtils::SNOWFLAKE_SLOW_COUNT;

	constexpr double arenaScreenWidthReal = static_cast<double>(ArenaRenderUtils::SCREEN_WIDTH);
	constexpr double arenaScreenHeightReal = static_cast<double>(ArenaRenderUtils::SCREEN_HEIGHT);
	constexpr int arenaFramesPerSecond = ArenaRenderUtils::FRAMES_PER_SECOND;

	constexpr double fastVelocityPercentX = static_cast<double>(
		ArenaWeatherUtils::SNOWFLAKE_PIXELS_PER_FRAME_X * arenaFramesPerSecond) / arenaScreenWidthReal;
	constexpr double fastVelocityPercentY = static_cast<double>(
		ArenaWeatherUtils::SNOWFLAKE_FAST_PIXELS_PER_FRAME_Y * arenaFramesPerSecond) / arenaScreenHeightReal;
	constexpr double mediumVelocityPercentX = static_cast<double>(
		ArenaWeatherUtils::SNOWFLAKE_PIXELS_PER_FRAME_X * arenaFramesPerSecond) / arenaScreenWidthReal;
	constexpr double mediumVelocityPercentY = static_cast<double>(
		ArenaWeatherUtils::SNOWFLAKE_MEDIUM_PIXELS_PER_FRAME_Y * arenaFramesPerSecond) / arenaScreenHeightReal;
	constexpr double slowVelocityPercentX = static_cast<double>(
		ArenaWeatherUtils::SNOWFLAKE_PIXELS_PER_FRAME_X * arenaFramesPerSecond) / arenaScreenWidthReal;
	constexpr double slowVelocityPercentY = static_cast<double>(
		ArenaWeatherUtils::SNOWFLAKE_SLOW_PIXELS_PER_FRAME_Y * arenaFramesPerSecond) / arenaScreenHeightReal;

	animateSnowflakeRange(fastStartIndex, fastEndIndex, fastVelocityPercentX, fastVelocityPercentY);
	animateSnowflakeRange(mediumStartIndex, mediumEndIndex, mediumVelocityPercentX, mediumVelocityPercentY);
	animateSnowflakeRange(slowStartIndex, slowEndIndex, slowVelocityPercentX, slowVelocityPercentY);
}

WeatherInstance::WeatherInstance()
{
	this->type = static_cast<WeatherInstance::Type>(-1);
}

void WeatherInstance::init(const WeatherDefinition &weatherDef, const Clock &clock,
	const ExeData &exeData, Random &random, TextureManager &textureManager)
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
		Buffer<uint8_t> thunderstormColors = ArenaWeatherUtils::makeThunderstormColors(exeData);
		this->rain.init(rainDef.thunderstorm, clock, std::move(thunderstormColors), random, textureManager);
	}
	else if (weatherDefType == WeatherDefinition::Type::Snow)
	{
		this->type = WeatherInstance::Type::Snow;
		this->snow.init(random);
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

const WeatherInstance::SnowInstance &WeatherInstance::getSnow() const
{
	DebugAssert(this->type == WeatherInstance::Type::Snow);
	return this->snow;
}

void WeatherInstance::update(double dt, const Clock &clock, double aspectRatio,
	Random &random, AudioManager &audioManager)
{
	if (this->type == WeatherInstance::Type::None)
	{
		// Do nothing.
	}
	else if (this->type == WeatherInstance::Type::Rain)
	{
		this->rain.update(dt, clock, aspectRatio, random, audioManager);
	}
	else if (this->type == WeatherInstance::Type::Snow)
	{
		this->snow.update(dt, aspectRatio, random);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(this->type)));
	}
}
