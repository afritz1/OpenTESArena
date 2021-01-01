#include <algorithm>

#include "TextureBuilder.h"

void TextureBuilder::PalettedTexture::init(int width, int height, const uint8_t *texels)
{
	this->texels.init(width, height);
	std::copy(texels, texels + (width * height), this->texels.get());
}

void TextureBuilder::TrueColorTexture::init(int width, int height, const uint32_t *texels)
{
	this->texels.init(width, height);
	std::copy(texels, texels + (width * height), this->texels.get());
}

TextureBuilder::TextureBuilder()
{
	this->type = static_cast<TextureBuilder::Type>(-1);
}

void TextureBuilder::initPaletted(int width, int height, const uint8_t *texels)
{
	this->type = TextureBuilder::Type::Paletted;
	this->paletteTexture.init(width, height, texels);
}

void TextureBuilder::initTrueColor(int width, int height, const uint32_t *texels)
{
	this->type = TextureBuilder::Type::TrueColor;
	this->trueColorTexture.init(width, height, texels);
}

int TextureBuilder::getWidth() const
{
	if (this->type == TextureBuilder::Type::Paletted)
	{
		return this->paletteTexture.texels.getWidth();
	}
	else if (this->type == TextureBuilder::Type::TrueColor)
	{
		return this->trueColorTexture.texels.getWidth();
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(this->type)));
	}
}

int TextureBuilder::getHeight() const
{
	if (this->type == TextureBuilder::Type::Paletted)
	{
		return this->paletteTexture.texels.getHeight();
	}
	else if (this->type == TextureBuilder::Type::TrueColor)
	{
		return this->trueColorTexture.texels.getHeight();
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(this->type)));
	}
}

TextureBuilder::Type TextureBuilder::getType() const
{
	return this->type;
}

const TextureBuilder::PalettedTexture &TextureBuilder::getPaletted() const
{
	DebugAssert(this->type == TextureBuilder::Type::Paletted);
	return this->paletteTexture;
}

const TextureBuilder::TrueColorTexture &TextureBuilder::getTrueColor() const
{
	DebugAssert(this->type == TextureBuilder::Type::TrueColor);
	return this->trueColorTexture;
}
