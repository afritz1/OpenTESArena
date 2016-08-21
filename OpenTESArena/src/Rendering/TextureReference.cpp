#include "TextureReference.h"

TextureReference::TextureReference(int32_t offset, short width, short height)
{
	this->offset = offset;
	this->width = width;
	this->height = height;
}

TextureReference::~TextureReference()
{

}

int32_t TextureReference::getOffset() const
{
	return this->offset;
}

short TextureReference::getWidth() const
{
	return this->width;
}

short TextureReference::getHeight() const
{
	return this->height;
}
