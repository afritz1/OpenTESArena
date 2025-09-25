#include <algorithm>
#include <limits>

#include "ArenaWeatherUtils.h"
#include "WeatherDefinition.h"
#include "WeatherInstance.h"
#include "../Assets/ArenaSoundName.h"
#include "../Audio/AudioManager.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/Clock.h"
#include "../Time/ClockLibrary.h"

#include "components/debug/Debug.h"

namespace
{
	bool IsDuringThunderstorm(const Clock &clock)
	{
		const ClockLibrary &clockLibrary = ClockLibrary::getInstance();
		const Clock &thunderstormStartClock = clockLibrary.getClock(ArenaClockUtils::ThunderstormStart);
		const Clock &thunderstormEndClock = clockLibrary.getClock(ArenaClockUtils::ThunderstormEnd);

		// Starts in the evening, ends in the morning.
		const double seconds = clock.getTotalSeconds();
		const double startSeconds = thunderstormStartClock.getTotalSeconds();
		const double endSeconds = thunderstormEndClock.getTotalSeconds();
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

void WeatherParticle::init(double xPercent, double yPercent)
{
	this->xPercent = xPercent;
	this->yPercent = yPercent;
}

void WeatherFogInstance::init(Random &random, TextureManager &textureManager)
{
	// Put the zeroed row in the middle of the texture since fog works a bit differently in this engine.
	/*constexpr int zeroedRow = ArenaRenderUtils::FOG_MATRIX_HEIGHT / 2;
	if (!ArenaRenderUtils::tryMakeFogMatrix(zeroedRow, random, textureManager, &this->fogMatrix))
	{
		DebugLogWarning("Couldn't make fog matrix.");
	}*/
}

void WeatherFogInstance::update(double dt)
{
	// Do nothing for now.
}

void WeatherRainInstance::Thunderstorm::init(Buffer<uint8_t> &&flashColors, bool active, Random &random)
{
	this->flashColors = std::move(flashColors);
	this->secondsSincePrevLightning = std::numeric_limits<double>::infinity();
	this->secondsUntilNextLightning = MakeSecondsUntilNextLightning(random);
	this->lightningBoltAngle = 0.0;
	this->active = active;
}

std::optional<double> WeatherRainInstance::Thunderstorm::getFlashPercent() const
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

std::optional<double> WeatherRainInstance::Thunderstorm::getLightningBoltPercent() const
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

void WeatherRainInstance::Thunderstorm::update(double dt, const Clock &clock,
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

			audioManager.playSound(ArenaSoundName::Thunder);
		}
	}
}

void WeatherRainInstance::init(bool isThunderstorm, const Clock &clock,
	Buffer<uint8_t> &&flashColors, Random &random, TextureManager &textureManager)
{
	this->particles.init(ArenaWeatherUtils::RAINDROP_TOTAL_COUNT);
	for (WeatherParticle &particle : this->particles)
	{
		particle.init(random.nextReal(), random.nextReal());
	}

	if (isThunderstorm)
	{
		this->thunderstorm = std::make_optional<Thunderstorm>();
		this->thunderstorm->init(std::move(flashColors), IsDuringThunderstorm(clock), random);
	}
	else
	{
		this->thunderstorm = std::nullopt;
	}
}

void WeatherRainInstance::update(double dt, const Clock &clock, double aspectRatio,
	Random &random, AudioManager &audioManager)
{
	auto animateRaindropRange = [this, dt, aspectRatio, &random](int startIndex, int endIndex,
		double velocityPercentX, double velocityPercentY)
	{
		for (int i = startIndex; i < endIndex; i++)
		{
			WeatherParticle &particle = this->particles.get(i);
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

	constexpr double arenaScreenWidthReal = ArenaRenderUtils::SCREEN_WIDTH_REAL;
	constexpr double arenaScreenHeightReal = ArenaRenderUtils::SCREEN_HEIGHT_REAL;
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

void WeatherSnowInstance::init(Random &random)
{
	this->particles.init(ArenaWeatherUtils::SNOWFLAKE_TOTAL_COUNT);
	for (WeatherParticle &particle : this->particles)
	{
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

void WeatherSnowInstance::update(double dt, double aspectRatio, Random &random)
{
	auto animateSnowflakeRange = [this, dt, aspectRatio, &random](int startIndex, int endIndex,
		double velocityPercentX, double velocityPercentY)
	{
		for (int i = startIndex; i < endIndex; i++)
		{
			WeatherParticle &particle = this->particles.get(i);
			const bool canBeRestarted = particle.yPercent >= 1.0;
			if (canBeRestarted)
			{
				// Pick somewhere on the top edge to spawn.
				particle.xPercent = random.nextReal();

				// Don't set Y to 0 since it can result in snowflakes stacking up on the same horizontal
				// line if multiple ones cross the bottom of the screen on the same frame.
				particle.yPercent = -(particle.yPercent - 1.0);

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

	constexpr double arenaScreenWidthReal = ArenaRenderUtils::SCREEN_WIDTH_REAL;
	constexpr double arenaScreenHeightReal = ArenaRenderUtils::SCREEN_HEIGHT_REAL;
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
	this->fog = false;
	this->rain = false;
	this->snow = false;
}

void WeatherInstance::init(const WeatherDefinition &weatherDef, const Clock &clock,
	const ExeData &exeData, Random &random, TextureManager &textureManager)
{
	const WeatherType weatherDefType = weatherDef.type;

	if (weatherDefType == WeatherType::Clear)
	{
		this->fog = false;
		this->rain = false;
		this->snow = false;
	}
	else if (weatherDefType == WeatherType::Overcast)
	{
		const WeatherOvercastDefinition &overcastDef = weatherDef.overcast;
		this->fog = overcastDef.heavyFog;
		this->rain = false;
		this->snow = false;
		this->fogInst.init(random, textureManager);
	}
	else if (weatherDefType == WeatherType::Rain)
	{
		this->fog = false;
		this->rain = true;
		this->snow = false;

		const WeatherRainDefinition &rainDef = weatherDef.rain;
		Buffer<uint8_t> thunderstormColors = ArenaWeatherUtils::makeThunderstormColors(exeData);
		this->rainInst.init(rainDef.thunderstorm, clock, std::move(thunderstormColors), random, textureManager);
	}
	else if (weatherDefType == WeatherType::Snow)
	{
		const WeatherSnowDefinition &snowDef = weatherDef.snow;
		this->fog = snowDef.heavyFog;
		this->rain = false;
		this->snow = true;
		this->fogInst.init(random, textureManager);
		this->snowInst.init(random);
	}
	else
	{
		DebugNotImplementedMsg(std::to_string(static_cast<int>(weatherDefType)));
	}
}

bool WeatherInstance::hasFog() const
{
	return this->fog;
}

bool WeatherInstance::hasRain() const
{
	return this->rain;
}

bool WeatherInstance::hasSnow() const
{
	return this->snow;
}

const WeatherFogInstance &WeatherInstance::getFog() const
{
	DebugAssert(this->hasFog());
	return this->fogInst;
}

const WeatherRainInstance &WeatherInstance::getRain() const
{
	DebugAssert(this->hasRain());
	return this->rainInst;
}

const WeatherSnowInstance &WeatherInstance::getSnow() const
{
	DebugAssert(this->hasSnow());
	return this->snowInst;
}

void WeatherInstance::update(double dt, const Clock &clock, double aspectRatio,
	Random &random, AudioManager &audioManager)
{
	if (this->hasFog())
	{
		this->fogInst.update(dt);
	}

	if (this->hasRain())
	{
		this->rainInst.update(dt, clock, aspectRatio, random, audioManager);
	}
	
	if (this->hasSnow())
	{
		this->snowInst.update(dt, aspectRatio, random);
	}
}
