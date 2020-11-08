#ifndef SUN_OBJECT_DEFINITION_H
#define SUN_OBJECT_DEFINITION_H

#include "../Math/Vector3.h"
#include "../Media/TextureUtils.h"

class SunObjectDefinition
{
private:
	// Added to location latitude to get 'sun latitude'.
	double bonusLatitude;

	ImageID imageID;
public:
	void init(double bonusLatitude, ImageID imageID);

	double getBonusLatitude() const;
	ImageID getImageID() const;
};

#endif
