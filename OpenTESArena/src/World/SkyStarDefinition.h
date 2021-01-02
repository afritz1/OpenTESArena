#ifndef SKY_STAR_DEFINITION_H
#define SKY_STAR_DEFINITION_H

#include <cstdint>

#include "../Math/MathUtils.h"
#include "../Media/TextureUtils.h"

class SkyStarDefinition
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
		TextureBuilderID textureBuilderID;

		void init(TextureBuilderID textureBuilderID);
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
	void initLarge(TextureBuilderID textureBuilderID);
	
	Type getType() const;
	const SmallStar &getSmallStar() const;
	const LargeStar &getLargeStar() const;
};

#endif
