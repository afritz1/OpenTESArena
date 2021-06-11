#include "CursorData.h"

CursorData::CursorData(TextureBuilderID textureBuilderID, PaletteID paletteID, CursorAlignment alignment)
{
	this->textureBuilderID = textureBuilderID;
	this->paletteID = paletteID;
	this->alignment = alignment;
}

TextureBuilderID CursorData::getTextureBuilderID() const
{
	return this->textureBuilderID;
}

PaletteID CursorData::getPaletteID() const
{
	return this->paletteID;
}

CursorAlignment CursorData::getAlignment() const
{
	return this->alignment;
}
