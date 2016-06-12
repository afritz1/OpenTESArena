#include "TextureReference.h"

TextureReference::TextureReference(int offset, int width, int height)
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

int TextureReference::getWidth() const
{
	return this->width;
}

int TextureReference::getHeight() const
{
	return this->height;
}
