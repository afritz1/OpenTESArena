#include <cassert>

#include "Texture.h"

Texture::Texture(unsigned int *pixels, int width, int height)
{
	assert(pixels != nullptr);

	int area = width * height;
	this->diffuse = std::vector<Float4f>(area);

	for (int i = 0; i < area; ++i)
	{
		this->diffuse.at(i) = Float4f::fromARGB(pixels[i]);
	}

	this->width = width;
	this->height = height;
}

Texture::~Texture()
{

}

int Texture::getWidth() const
{
	return this->width;
}

int Texture::getHeight() const
{
	return this->height;
}

const Float4f *Texture::getPixels() const
{
	return this->diffuse.data();
}
