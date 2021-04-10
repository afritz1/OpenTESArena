#ifndef WEATHER_INSTANCE_H
#define WEATHER_INSTANCE_H

#include <optional>

#include "../Math/MathUtils.h"

#include "components/utilities/Buffer.h"

class Random;
class WeatherDefinition;

class WeatherInstance
{
public:
	enum class Type
	{
		// @todo: may eventually add the fog mask here since it animates in the original game.
		None, // No extra data/simulation needed for the weather type.
		Rain,
		Snow
	};

	struct RainInstance
	{
		struct Raindrop
		{
			// Percent positions on the screen, where (0, 0) is the top left. This should work for any
			// resolution/aspect ratio. The raindrop's anchor is also at the top left.
			double xPercent, yPercent;

			void init(double xPercent, double yPercent);
		};

		struct Thunderstorm
		{
			double secondsSincePrevLightning;
			double secondsUntilNextLightning;
			Radians lightningBoltAngle;
			// @todo: generated lightning bolt paletted texture

			void init(Random &random);

			// If a lightning bolt recently flashed, returns how bright the sky is because of the flash.
			// Otherwise returns 0.
			double getFlashPercent() const;

			bool isLightningBoltVisible() const;

			void update(double dt, Random &random);
		};

		Buffer<Raindrop> raindrops;
		std::optional<Thunderstorm> thunderstorm;

		void init(bool isThunderstorm, Random &random);

		void update(double dt, double aspectRatio, Random &random);
	};
private:
	Type type;
	RainInstance rain;
public:
	WeatherInstance();

	void init(const WeatherDefinition &weatherDef);

	Type getType() const;
	const RainInstance &getRain() const;

	void update(double dt, double aspectRatio, Random &random);
};

#endif
