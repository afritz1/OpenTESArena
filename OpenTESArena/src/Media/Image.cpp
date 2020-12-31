#include "Image.h"

#include "components/debug/Debug.h"

void Image::init(int width, int height, const std::optional<PaletteID> &paletteID)
{
	DebugAssertMsg(width > 0, "Width must be positive.");
	DebugAssertMsg(height > 0, "Height must be positive.");
	this->pixels.init(width, height);
	this->paletteID = paletteID;
}

int Image::getWidth() const
{
	return this->pixels.getWidth();
}

int Image::getHeight() const
{
	return this->pixels.getHeight();
}

uint8_t *Image::getPixels()
{
	return this->pixels.get();
}

const uint8_t *Image::getPixels() const
{
	return this->pixels.get();
}

PaletteID *Image::getPaletteID()
{
	return this->paletteID.has_value() ? &(*this->paletteID) : nullptr;
}

const PaletteID *Image::getPaletteID() const
{
	return this->paletteID.has_value() ? &(*this->paletteID) : nullptr;
}

uint8_t Image::getPixel(int x, int y) const
{
	return this->pixels.get(x, y);
}

void Image::setPixel(int x, int y, uint8_t color)
{
	this->pixels.set(x, y, color);
}

void Image::clear()
{
	this->pixels.clear();
	this->paletteID = std::nullopt;
}
