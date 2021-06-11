#ifndef CURSOR_DATA_H
#define CURSOR_DATA_H

#include "../Media/TextureUtils.h"

enum class CursorAlignment;

class CursorData // @todo: rename to CursorDisplayState?
{
private:
	TextureBuilderID textureBuilderID; // @todo: maybe should be a UI texture handle at some point.
	PaletteID paletteID;
	CursorAlignment alignment;
public:
	CursorData(TextureBuilderID textureBuilderID, PaletteID paletteID, CursorAlignment alignment);

	TextureBuilderID getTextureBuilderID() const;
	PaletteID getPaletteID() const;
	CursorAlignment getAlignment() const;
};

#endif
