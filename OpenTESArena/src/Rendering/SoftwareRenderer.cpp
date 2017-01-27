#include <algorithm>

#include "SoftwareRenderer.h"

SoftwareRenderer::SoftwareRenderer(int width, int height)
{
	// Initialize 2D frame buffer.
	const int pixelCount = width * height;
	this->pixels = std::vector<uint32_t>(pixelCount);
	std::fill(this->pixels.begin(), this->pixels.end(), 0);

	this->width = width;
	this->height = height;
}

SoftwareRenderer::~SoftwareRenderer()
{

}

int SoftwareRenderer::getWidth() const
{
	return this->width;
}

int SoftwareRenderer::getHeight() const
{
	return this->height;
}

const uint32_t *SoftwareRenderer::getPixels() const
{
	return this->pixels.data();
}

void SoftwareRenderer::render()
{
	// Placeholder rendering. Just draws a red gradient.
	const float widthReal = static_cast<float>(this->width);
	const float heightReal = static_cast<float>(this->height);

	for (int y = 0; y < this->height; ++y)
	{
		const float yReal = static_cast<float>(y);
		for (int x = 0; x < this->width; ++x)
		{
			const float xReal = static_cast<float>(x);
			const float redPercent = 1.0f - (xReal / widthReal);

			const uint32_t color = static_cast<uint8_t>(redPercent * 255.0f) << 16;

			const int index = x + (y * this->width);
			this->pixels[index] = color;
		}
	}
}
