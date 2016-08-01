#include "TextureReference.h"

TextureReference::TextureReference(int offset, short width, short height)
{
	this->offset = offset;
	this->width = width;
	this->height = height;
}

TextureReference::~TextureReference()
{

}

int TextureReference::getOffset() const
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
