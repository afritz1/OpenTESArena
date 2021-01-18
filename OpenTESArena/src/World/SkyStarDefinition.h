#ifndef SKY_STAR_DEFINITION_H
#define SKY_STAR_DEFINITION_H

#include <cstdint>

#include "../Assets/TextureAssetReference.h"

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
		TextureAssetReference textureAssetRef;

		void init(TextureAssetReference &&textureAssetRef);
	};
private:
	Type type;
	SmallStar smallStar;
	LargeStar largeStar;

	void init(Type type);
public:
	void initSmall(uint8_t paletteIndex);
	void initLarge(TextureAssetReference &&textureAssetRef);
	
	Type getType() const;
	const SmallStar &getSmallStar() const;
	const LargeStar &getLargeStar() const;
};

#endif
