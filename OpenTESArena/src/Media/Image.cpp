#include "Image.h"

#include "components/debug/Debug.h"

void Image::init(int width, int height, const Palette *palette)
{
	DebugAssertMsg(width > 0, "Width must be positive.");
	DebugAssertMsg(height > 0, "Height must be positive.");
	this->pixels.init(width, height);
	this->palette = (palette != nullptr) ? std::make_unique<Palette>(*palette) : nullptr;
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

Palette *Image::getPalette()
{
	return this->palette.get();
}

const Palette *Image::getPalette() const
{
	return this->palette.get();
}

uint8_t Image::getPixel(int x, int y) const
{
	return this->pixels.get(x, y);
}

void Image::setPixel(int x, int y, uint8_t color)
{
	this->pixels.set(x, y, color);
}
