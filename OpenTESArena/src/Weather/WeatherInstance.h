#ifndef WEATHER_INSTANCE_H
#define WEATHER_INSTANCE_H

#include <optional>

#include "../Math/MathUtils.h"
//#include "../Rendering/ArenaRenderUtils.h"

#include "components/utilities/Buffer.h"

class AudioManager;
class Random;
class TextureManager;

struct Clock;
struct ExeData;
struct WeatherDefinition;

struct WeatherParticle
{
	// Percent positions on the screen, where (0, 0) is the top left. This should work for any
	// resolution/aspect ratio. The particle's anchor is also at the top left.
	double xPercent, yPercent;

	void init(double xPercent, double yPercent);
};

struct WeatherFogInstance
{
	//ArenaRenderUtils::FogMatrix fogMatrix; // Used for drawing fog.
	// @todo: other fog state values (position changing over time, etc.)

	void init(Random &random, TextureManager &textureManager);

	void update(double dt);
};

struct WeatherRainInstance
{
	// @todo: see if this should be refactored so part of it is in SkyDefinition/SkyInstance instead.
	struct Thunderstorm
	{
		Buffer<uint8_t> flashColors; // In here and not WeatherDefinition due to design complications.
		double secondsSincePrevLightning;
		double secondsUntilNextLightning;
		Radians lightningBoltAngle;
		bool active; // Whether the thunderstorm can flash/have lightning bolts.

		void init(Buffer<uint8_t> &&flashColors, bool active, Random &random);

		// If a lightning bolt recently flashed, returns how bright the sky is because of the flash.
		std::optional<double> getFlashPercent() const;

		// If a lightning bolt recently flashed, returns how far through its animation it is.
		std::optional<double> getLightningBoltPercent() const;

		void update(double dt, const Clock &clock, Random &random, AudioManager &audioManager);
	};

	Buffer<WeatherParticle> particles;
	std::optional<Thunderstorm> thunderstorm;

	void init(bool isThunderstorm, const Clock &clock, Buffer<uint8_t> &&flashColors, Random &random,
		TextureManager &textureManager);

	void update(double dt, const Clock &clock, double aspectRatio, Random &random, AudioManager &audioManager);
};

struct WeatherSnowInstance
{
	Buffer<WeatherParticle> particles;
	Buffer<bool> directions;
	Buffer<double> lastDirectionChangeSeconds;

	void init(Random &random);

	void update(double dt, double aspectRatio, Random &random);
};

class WeatherInstance
{
private:
	bool fog, rain, snow;
	WeatherFogInstance fogInst;
	WeatherRainInstance rainInst;
	WeatherSnowInstance snowInst;
public:
	WeatherInstance();

	void init(const WeatherDefinition &weatherDef, const Clock &clock, const ExeData &exeData, Random &random,
		TextureManager &textureManager);

	bool hasFog() const;
	bool hasRain() const;
	bool hasSnow() const;
	const WeatherFogInstance &getFog() const;
	const WeatherRainInstance &getRain() const;
	const WeatherSnowInstance &getSnow() const;

	void update(double dt, const Clock &clock, double aspectRatio, Random &random, AudioManager &audioManager);
};

#endif
