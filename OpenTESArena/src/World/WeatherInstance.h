#ifndef WEATHER_INSTANCE_H
#define WEATHER_INSTANCE_H

#include <optional>

#include "../Math/MathUtils.h"
#include "../Media/TextureBuilder.h"
#include "../Media/TextureUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

class AudioManager;
class Clock;
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
			Buffer<TextureBuilderIdGroup> lightningBoltTextureBuilderIDs;
			double secondsSincePrevLightning;
			double secondsUntilNextLightning;
			Radians lightningBoltAngle;
			int lightningBoltGroupIndex;
			bool active; // Whether the thunderstorm can flash/have lightning bolts.

			void init(Buffer<uint8_t> &&flashColors, Buffer<TextureBuilderIdGroup> &&lightningBoltTextureBuilderIDs,
				bool active, Random &random);

			// If a lightning bolt recently flashed, returns how bright the sky is because of the flash.
			std::optional<double> getFlashPercent() const;

			// If a lightning bolt recently flashed, returns how far through its animation it is.
			std::optional<double> getLightningBoltPercent() const;

			void update(double dt, const Clock &clock, Random &random, AudioManager &audioManager);
		};

		Buffer<Particle> particles;
		std::optional<Thunderstorm> thunderstorm;

		void init(bool isThunderstorm, const Clock &clock, Buffer<uint8_t> &&flashColors, Random &random,
			TextureManager &textureManager);

		void update(double dt, const Clock &clock, double aspectRatio, Random &random, AudioManager &audioManager);
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

	void init(const WeatherDefinition &weatherDef, const Clock &clock, const ExeData &exeData, Random &random,
		TextureManager &textureManager);

	Type getType() const;
	const RainInstance &getRain() const;
	const SnowInstance &getSnow() const;

	void update(double dt, const Clock &clock, double aspectRatio, Random &random, AudioManager &audioManager);
};

#endif
