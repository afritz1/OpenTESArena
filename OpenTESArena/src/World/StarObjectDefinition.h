#ifndef STAR_OBJECT_DEFINITION_H
#define STAR_OBJECT_DEFINITION_H

#include <cstdint>

#include "../Math/MathUtils.h"
#include "../Media/TextureUtils.h"

class StarObjectDefinition
{
public:
	enum class Type
	{
		Small,
		Large
	};

	struct SmallStar
	{
		uint32_t colorARGB;

		void init(uint32_t colorARGB);
	};

	struct LargeStar
	{
		ImageID imageID;

		void init(ImageID imageID);
	};
private:
	Radians angleX, angleY;

	Type type;

	union
	{
		SmallStar smallStar;
		LargeStar largeStar;
	};

	void init(Type type, Radians angleX, Radians angleY);
public:
	void initSmall(Radians angleX, Radians angleY, uint32_t colorARGB);
	void initLarge(Radians angleX, Radians angleY, ImageID imageID);

	Radians getAngleX() const;
	Radians getAngleY() const;
	
	Type getType() const;
	const SmallStar &getSmallStar() const;
	const LargeStar &getLargeStar() const;
};

#endif
