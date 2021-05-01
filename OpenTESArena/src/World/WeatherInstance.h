#ifndef WEATHER_INSTANCE_H
#define WEATHER_INSTANCE_H

#include <optional>

#include "../Math/MathUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

class ExeData;
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

	struct Particle
	{
		// Percent positions on the screen, where (0, 0) is the top left. This should work for any
		// resolution/aspect ratio. The particle's anchor is also at the top left.
		double xPercent, yPercent;

		void init(double xPercent, double yPercent);
	};

	struct RainInstance
	{
		struct Thunderstorm
		{
			Buffer<uint8_t> flashColors; // In here and not WeatherDefinition due to design complications.
			double secondsSincePrevLightning;
			double secondsUntilNextLightning;
			Radians lightningBoltAngle;
			// @todo: generated lightning bolt paletted texture

			void init(Buffer<uint8_t> &&flashColors, Random &random);

			int getFlashColorCount() const;
			uint8_t getFlashColor(int index) const;

			// If a lightning bolt recently flashed, returns how bright the sky is because of the flash.
			// Otherwise returns 0.
			double getFlashPercent() const;

			bool isLightningBoltVisible() const;

			void update(double dt, Random &random);
		};

		Buffer<Particle> particles;
		std::optional<Thunderstorm> thunderstorm;

		void init(bool isThunderstorm, Buffer<uint8_t> &&flashColors, Random &random);

		void update(double dt, double aspectRatio, Random &random);
	};

	struct SnowInstance
	{
		Buffer<Particle> particles;
		Buffer<bool> directions;
		Buffer<double> lastDirectionChangeSeconds;

		void init(Random &random);

		void update(double dt, double aspectRatio, Random &random);
	};
private:
	Type type;
	RainInstance rain;
	SnowInstance snow;
public:
	WeatherInstance();

	void init(const WeatherDefinition &weatherDef, const ExeData &exeData, Random &random);

	Type getType() const;
	const RainInstance &getRain() const;
	const SnowInstance &getSnow() const;

	void update(double dt, double aspectRatio, Random &random);
};

#endif
