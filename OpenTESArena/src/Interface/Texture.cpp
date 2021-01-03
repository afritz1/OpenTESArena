#include "SDL.h"

#include "Texture.h"

#include "components/debug/Debug.h"

Texture::Texture()
{
	this->texture = nullptr;
}

Texture::Texture(Texture &&texture)
{
	this->texture = texture.texture;
	texture.texture = nullptr;
}

Texture::~Texture()
{
	if (this->texture != nullptr)
	{
		SDL_DestroyTexture(this->texture);
	}
}

Texture &Texture::operator=(Texture &&texture)
{
	if (this->texture != texture.texture)
	{
		if (this->texture != nullptr)
		{
			SDL_DestroyTexture(this->texture);
		}

		this->texture = texture.texture;
	}

	texture.texture = nullptr;
	return *this;
}

int Texture::getWidth() const
{
	DebugAssert(this->texture != nullptr);

	int width;
	SDL_QueryTexture(this->texture, nullptr, nullptr, &width, nullptr);

	return width;
}

int Texture::getHeight() const
{
	DebugAssert(this->texture != nullptr);

	int height;
	SDL_QueryTexture(this->texture, nullptr, nullptr, nullptr, &height);

	return height;
}

SDL_Texture *Texture::get() const
{
	return this->texture;
}

void Texture::init(SDL_Texture *texture)
{
	DebugAssert(this->texture == nullptr);
	this->texture = texture;
}

void Texture::clear()
{
	if (this->texture != nullptr)
	{
		SDL_DestroyTexture(this->texture);
		this->texture = nullptr;
	}
}
