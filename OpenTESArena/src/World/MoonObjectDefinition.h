#ifndef MOON_OBJECT_DEFINITION_H
#define MOON_OBJECT_DEFINITION_H

#include "../Math/Vector3.h"
#include "../Media/TextureUtils.h"

class MoonObjectDefinition
{
private:
	// Base position in the sky before latitude and time-of-day adjustments.
	Double3 baseDir;

	// Added to location latitude to get 'moon latitude'.
	double bonusLatitude;

	int phaseCount; // Number of days in period.
	int phaseIndexDayOffset; // Bias to phase start.

	ImageID imageID;
public:
	void init(const Double3 &baseDir, double bonusLatitude, int phaseCount,
		int phaseIndexDayOffset, ImageID imageID);

	const Double3 &getBaseDirection() const;
	double getBonusLatitude() const;
	int getPhaseCount() const;
	int getPhaseIndexDayOffset() const;
	ImageID getImageID() const;
};

#endif
