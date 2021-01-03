#ifndef SKY_SUN_DEFINITION_H
#define SKY_SUN_DEFINITION_H

#include "../Media/TextureUtils.h"

class SkySunDefinition
{
private:
	TextureBuilderID textureBuilderID;
public:
	void init(TextureBuilderID textureBuilderID);

	TextureBuilderID getTextureBuilderID() const;
};

#endif
