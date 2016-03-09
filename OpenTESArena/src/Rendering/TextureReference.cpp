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

const int &TextureReference::getOffset() const
{
	return this->offset;
}

const int &TextureReference::getWidth() const
{
	return this->width;
}

const int &TextureReference::getHeight() const
{
	return this->height;
}
