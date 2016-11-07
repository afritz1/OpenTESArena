#include <cassert>

#include "SDL.h"

#include "Texture.h"

Texture::Texture(SDL_Texture *texture)
{
	assert(texture != nullptr);
	this->texture = texture;
}

Texture::Texture(Texture &&texture)
{
	this->texture = texture.texture;
	texture.texture = nullptr;
}

Texture::~Texture()
{
	SDL_DestroyTexture(this->texture);
}

int Texture::getWidth() const
{
	int width;
	SDL_QueryTexture(this->texture, nullptr, nullptr, &width, nullptr);

	return width;
}

int Texture::getHeight() const
{
	int height;
	SDL_QueryTexture(this->texture, nullptr, nullptr, nullptr, &height);

	return height;
}

SDL_Texture *Texture::get() const
{
	return this->texture;
}
