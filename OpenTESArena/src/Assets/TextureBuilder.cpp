#include <algorithm>

#include "TextureBuilder.h"

TextureBuilder::TextureBuilder()
{
	this->width = 0;
	this->height = 0;
	this->bytesPerTexel = 0;
}

void TextureBuilder::initPaletted(int width, int height, const uint8_t *texels)
{
	this->width = width;
	this->height = height;
	this->bytesPerTexel = 1;

	const int texelCount = width * height;
	const int byteCount = texelCount;
	this->texels.init(byteCount);
	std::copy(texels, texels + texelCount, reinterpret_cast<uint8_t*>(this->texels.begin()));
}

void TextureBuilder::initTrueColor(int width, int height, const uint32_t *texels)
{
	this->width = width;
	this->height = height;
	this->bytesPerTexel = 4;

	const int texelCount = width * height;
	const int byteCount = texelCount * 4;
	this->texels.init(byteCount);
	std::copy(texels, texels + texelCount, reinterpret_cast<uint32_t*>(this->texels.begin()));
}

Span2D<const uint8_t> TextureBuilder::getTexels8() const
{
	DebugAssert(this->bytesPerTexel == 1);
	return Span2D<const uint8_t>(reinterpret_cast<const uint8_t*>(this->texels.begin()), this->width, this->height);
}

Span2D<const uint32_t> TextureBuilder::getTexels32() const
{
	DebugAssert(this->bytesPerTexel == 4);
	return Span2D<const uint32_t>(reinterpret_cast<const uint32_t*>(this->texels.begin()), this->width, this->height);
}
