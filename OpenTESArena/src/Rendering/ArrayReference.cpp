#include "ArrayReference.h"

// -- ArrayReference --

ArrayReference::ArrayReference(int offset, int count)
{
	this->offset = offset;
	this->count = count;
}

// -- TextureReference --

TextureReference::TextureReference(int offset, short width, short height)
{
	this->offset = offset;
	this->width = width;
	this->height = height;
}
