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
	this->bytes.init(byteCount);
	std::copy(texels, texels + texelCount, reinterpret_cast<uint8_t*>(this->bytes.begin()));
}

void TextureBuilder::initHighColor(int width, int height, const uint16_t *texels)
{
	this->width = width;
	this->height = height;
	this->bytesPerTexel = 2;

	const int texelCount = width * height;
	const int byteCount = texelCount * 2;
	this->bytes.init(byteCount);
	std::copy(texels, texels + texelCount, reinterpret_cast<uint16_t*>(this->bytes.begin()));
}

void TextureBuilder::initTrueColor(int width, int height, const uint32_t *texels)
{
	this->width = width;
	this->height = height;
	this->bytesPerTexel = 4;

	const int texelCount = width * height;
	const int byteCount = texelCount * 4;
	this->bytes.init(byteCount);
	std::copy(texels, texels + texelCount, reinterpret_cast<uint32_t*>(this->bytes.begin()));
}

Span2D<const uint8_t> TextureBuilder::getTexels8() const
{
	DebugAssert(this->bytesPerTexel == 1);
	return Span2D<const uint8_t>(reinterpret_cast<const uint8_t*>(this->bytes.begin()), this->width, this->height);
}

Span2D<const uint16_t> TextureBuilder::getTexels16() const
{
	DebugAssert(this->bytesPerTexel == 2);
	return Span2D<const uint16_t>(reinterpret_cast<const uint16_t*>(this->bytes.begin()), this->width, this->height);
}

Span2D<const uint32_t> TextureBuilder::getTexels32() const
{
	DebugAssert(this->bytesPerTexel == 4);
	return Span2D<const uint32_t>(reinterpret_cast<const uint32_t*>(this->bytes.begin()), this->width, this->height);
}
