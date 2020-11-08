#ifndef AIR_OBJECT_DEFINITION_H
#define AIR_OBJECT_DEFINITION_H

#include "../Math/MathUtils.h"
#include "../Media/TextureUtils.h"

class AirObjectDefinition
{
private:
	Radians angleX, angleY;
	ImageID imageID;
public:
	void init(Radians angleX, Radians angleY, ImageID imageID);

	Radians getAngleX() const;
	Radians getAngleY() const;
	ImageID getImageID() const;
};

#endif
