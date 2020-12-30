#include "TextureBuilder.h"

void TextureBuilder::PalettedTexture::init(Buffer2D<uint8_t> &&texels, const std::optional<PaletteID> &paletteID)
{
	this->texels = std::move(texels);
	this->paletteID = paletteID;
}

void TextureBuilder::TrueColorTexture::init(Buffer2D<uint32_t> &&texels)
{
	this->texels = std::move(texels);
}

TextureBuilder::TextureBuilder()
{
	this->type = static_cast<TextureBuilder::Type>(-1);
}

void TextureBuilder::initPaletted(Buffer2D<uint8_t> &&texels, const std::optional<PaletteID> &paletteID)
{
	this->type = TextureBuilder::Type::Paletted;
	this->paletteTexture.init(std::move(texels), paletteID);
}

void TextureBuilder::initPaletted(Buffer2D<uint8_t> &&texels)
{
	this->initPaletted(std::move(texels), std::nullopt);
}

void TextureBuilder::initTrueColor(Buffer2D<uint32_t> &&texels)
{
	this->type = TextureBuilder::Type::TrueColor;
	this->trueColorTexture.init(std::move(texels));
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
