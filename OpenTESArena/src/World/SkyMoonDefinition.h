#ifndef SKY_MOON_DEFINITION_H
#define SKY_MOON_DEFINITION_H

#include "../Media/TextureUtils.h"

class SkyMoonDefinition
{
private:
	TextureBuilderIdGroup textureBuilderIDs;
public:
	void init(const TextureBuilderIdGroup &textureBuilderIDs);

	int getTextureCount() const;
	TextureBuilderID getTextureBuilderID(int index) const;
};

#endif
