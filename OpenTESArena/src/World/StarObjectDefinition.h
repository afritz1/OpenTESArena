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
		uint8_t paletteIndex;

		void init(uint8_t paletteIndex);
	};

	struct LargeStar
	{
		ImageID imageID;

		void init(ImageID imageID);
	};
private:
	Type type;

	union
	{
		SmallStar smallStar;
		LargeStar largeStar;
	};

	void init(Type type);
public:
	void initSmall(uint8_t paletteIndex);
	void initLarge(ImageID imageID);
	
	Type getType() const;
	const SmallStar &getSmallStar() const;
	const LargeStar &getLargeStar() const;
};

#endif
