#include <algorithm>

#include "TextureBuilder.h"

void TextureBuilder::PalettedTexture::init(int width, int height, const uint8_t *texels)
{
	this->texels.init(width, height);
	std::copy(texels, texels + (width * height), this->texels.begin());
}

void TextureBuilder::TrueColorTexture::init(int width, int height, const uint32_t *texels)
{
	this->texels.init(width, height);
	std::copy(texels, texels + (width * height), this->texels.begin());
}

TextureBuilder::TextureBuilder()
{
	this->type = static_cast<TextureBuilderType>(-1);
}

void TextureBuilder::initPaletted(int width, int height, const uint8_t *texels)
{
	this->type = TextureBuilderType::Paletted;
	this->paletteTexture.init(width, height, texels);
}

void TextureBuilder::initTrueColor(int width, int height, const uint32_t *texels)
{
	this->type = TextureBuilderType::TrueColor;
	this->trueColorTexture.init(width, height, texels);
}

int TextureBuilder::getWidth() const
{
	if (this->type == TextureBuilderType::Paletted)
	{
		return this->paletteTexture.texels.getWidth();
	}
	else if (this->type == TextureBuilderType::TrueColor)
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
	if (this->type == TextureBuilderType::Paletted)
	{
		return this->paletteTexture.texels.getHeight();
	}
	else if (this->type == TextureBuilderType::TrueColor)
	{
		return this->trueColorTexture.texels.getHeight();
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(this->type)));
	}
}

int TextureBuilder::getBytesPerTexel() const
{
	if (this->type == TextureBuilderType::Paletted)
	{
		return 1;
	}
	else if (this->type == TextureBuilderType::TrueColor)
	{
		return 4;
	}
	else
	{
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(this->type)));
	}
}

TextureBuilderType TextureBuilder::getType() const
{
	return this->type;
}

const TextureBuilder::PalettedTexture &TextureBuilder::getPaletted() const
{
	DebugAssert(this->type == TextureBuilderType::Paletted);
	return this->paletteTexture;
}

const TextureBuilder::TrueColorTexture &TextureBuilder::getTrueColor() const
{
	DebugAssert(this->type == TextureBuilderType::TrueColor);
	return this->trueColorTexture;
}
