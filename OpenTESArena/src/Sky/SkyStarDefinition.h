#ifndef SKY_STAR_DEFINITION_H
#define SKY_STAR_DEFINITION_H

#include <cstdint>

#include "../Assets/TextureAsset.h"

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
		TextureAsset textureAsset;

		void init(TextureAsset &&textureAsset);
	};
private:
	Type type;
	SmallStar smallStar;
	LargeStar largeStar;

	void init(Type type);
public:
	void initSmall(uint8_t paletteIndex);
	void initLarge(TextureAsset &&textureAsset);
	
	Type getType() const;
	const SmallStar &getSmallStar() const;
	const LargeStar &getLargeStar() const;
};

#endif
