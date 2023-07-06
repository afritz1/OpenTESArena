#ifndef SKY_STAR_DEFINITION_H
#define SKY_STAR_DEFINITION_H

#include <cstdint>

#include "../Assets/TextureAsset.h"

enum class SkyStarType
{
	Small,
	Large
};

struct SkySmallStarDefinition
{
	uint8_t paletteIndex;

	void init(uint8_t paletteIndex);
};

struct SkyLargeStarDefinition
{
	TextureAsset textureAsset;

	void init(TextureAsset &&textureAsset);
};

class SkyStarDefinition
{
private:
	SkyStarType type;
	SkySmallStarDefinition smallStar;
	SkyLargeStarDefinition largeStar;

	void init(SkyStarType type);
public:
	void initSmall(uint8_t paletteIndex);
	void initLarge(TextureAsset &&textureAsset);
	
	SkyStarType getType() const;
	const SkySmallStarDefinition &getSmallStar() const;
	const SkyLargeStarDefinition &getLargeStar() const;
};

#endif
