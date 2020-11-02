#ifndef SKY_OBJECT_INSTANCE_H
#define SKY_OBJECT_INSTANCE_H

#include "../Math/MathUtils.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Media/TextureUtils.h"

class SkyObjectDefinition;

class SkyObjectInstance
{
private:
	struct Land
	{
		Radians angleX;

		void init(Radians angleX);
	};

	struct Air
	{
		Radians angleX, angleY;

		void init(Radians angleX, Radians angleY);
	};

	struct Sun
	{
		// Added to location latitude to get 'sun latitude'.
		double bonusLatitude;

		void init(double bonusLatitude);
	};

	struct Moon
	{
		// Base position in the sky before latitude and time-of-day adjustments.
		double baseDirX, baseDirY, baseDirZ;

		// Added to location latitude to get 'moon latitude'.
		double bonusLatitude;

		// Percent through phases (full/half/new/etc.). Affects which texture is used.
		double phasePercent;

		int phaseCount;
		int phaseIndexDayOffset;

		void init(double baseDirX, double baseDirY, double baseDirZ, double bonusLatitude,
			double phasePercent, int phaseCount, int phaseIndexDayOffset);
	};

	struct Star
	{
		Radians angleX, angleY;

		void init(Radians angleX, Radians angleY);
	};

	Double3 calculatedDir; // Actual direction based on current game state.
	double curAnimSeconds; // Seconds through animation.
	ImageID curImageID; // Currently displayed texture.
	
	int defIndex; // Index in sky definition objects list. Determines union access.

	union
	{
		Land land;
		Air air;
		Sun sun;
		Moon moon;
		Star star;
	};

	void init(int skyObjectDefIndex);
public:
	SkyObjectInstance();

	void initLand(Radians angleX, int skyObjectDefIndex);
	void initAir(Radians angleX, Radians angleY, int skyObjectDefIndex);
	void initSun(double bonusLatitude, int skyObjectDefIndex);
	void initMoon(double baseDirX, double baseDirY, double baseDirZ, double bonusLatitude, double phasePercent,
		int phaseCount, int phaseIndexDayOffset, int skyObjectDefIndex);
	void initStar(Radians angleX, Radians angleY, int skyObjectDefIndex);

	const Double3 &getCalculatedDirection() const;
	ImageID getImageID() const;
	int getDefIndex() const;

	void update(double dt, const SkyObjectDefinition &skyObjectDef);
};

#endif
