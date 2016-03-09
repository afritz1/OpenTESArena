#include <cassert>

#include "Texture.h"

Texture::Texture(unsigned int *pixels, int width, int height)
{
	assert(pixels != nullptr);

	int area = width * height;
	this->diffuse = std::vector<Vector4f>(area);

	for (int i = 0; i < area; ++i)
	{
		this->diffuse.at(i) = Vector4f::fromARGB(pixels[i]);
	}

	this->width = width;
	this->height = height;
}

Texture::~Texture()
{

}

const int &Texture::getWidth() const
{
	return this->width;
}

const int &Texture::getHeight() const
{
	return this->height;
}

const Vector4f *Texture::getPixels() const
{
	return this->diffuse.data();
}