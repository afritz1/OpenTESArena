#ifndef DISTANT_SKY_H
#define DISTANT_SKY_H

#include <cstdint>
#include <vector>

#include "../Math/Vector3.h"

// Contains data for distant objects (mountains, clouds, stars). Each distant object's image
// is owned by the texture manager.

class MiscAssets;
class Surface;
class TextureManager;

enum class WeatherType;

class DistantSky
{
public:
	// An object that sits on the horizon.
	class LandObject
	{
	private:
		const Surface *surface;
		double angleRadians;
	public:
		LandObject(const Surface &surface, double angleRadians);

		const Surface &getSurface() const;
		double getAngleRadians() const;
	};

	// An object with an animation that sits on the horizon.
	class AnimatedLandObject
	{
	private:
		static constexpr double DEFAULT_FRAME_TIME = 1.0 / 18.0;

		std::vector<const Surface*> surfaces;
		double angleRadians, targetFrameTime, currentFrameTime;
		int index;
	public:
		// Each frame of animation is added via a separate method.
		AnimatedLandObject(double angleRadians, double frameTime);
		AnimatedLandObject(double angleRadians);

		int getSurfaceCount() const;
		const Surface &getSurface(int index) const;
		double getAngleRadians() const;
		double getFrameTime() const;
		int getIndex() const;

		void addSurface(const Surface &surface);
		void setFrameTime(double frameTime);
		void setIndex(int index);
		void update(double dt);
	};

	// An object in the air, like clouds.
	class AirObject
	{
	private:
		const Surface *surface;
		double angleRadians, height; // 0 height == horizon, 1 height == top of sky gradient.
	public:
		AirObject(const Surface &surface, double angleRadians, double height);

		const Surface &getSurface() const;
		double getAngleRadians() const;
		double getHeight() const;
	};

	// An object in space, like stars, ignoring the sun.
	class SpaceObject
	{
	private:
		const Surface *surface;
		Double3 direction;
	public:
		SpaceObject(const Surface &surface, const Double3 &direction);

		const Surface &getSurface() const;
		const Double3 &getDirection() const;
	};
private:
	// Number of unique directions in 360 degrees.
	static const int UNIQUE_ANGLES;

	std::vector<LandObject> landObjects;
	std::vector<AnimatedLandObject> animLandObjects;
	std::vector<AirObject> airObjects;
	std::vector<SpaceObject> spaceObjects;

	// The sun's position is a function of time of day.
	const Surface *sunSurface;
public:
	DistantSky();

	int getLandObjectCount() const;
	int getAnimatedLandObjectCount() const;
	int getAirObjectCount() const;
	int getSpaceObjectCount() const;

	const LandObject &getLandObject(int index) const;
	const AnimatedLandObject &getAnimatedLandObject(int index) const;
	const AirObject &getAirObject(int index) const;
	const SpaceObject &getSpaceObject(int index) const;
	const Surface &getSunSurface() const;

	void init(int localCityID, int provinceID, WeatherType weatherType, int currentDay,
		const MiscAssets &miscAssets, TextureManager &textureManager);

	void tick(double dt);
};

#endif
