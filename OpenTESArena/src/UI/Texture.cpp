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
	if (this->texture == nullptr)
	{
		DebugLogError("Can't query width of null SDL_Texture.");
		return 0;
	}

	int width;
	const int status = SDL_QueryTexture(this->texture, nullptr, nullptr, &width, nullptr);
	if (status != 0)
	{
		DebugLogError("Couldn't query SDL_Texture width (" + std::string(SDL_GetError()) + ").");
		return 0;
	}

	return width;
}

int Texture::getHeight() const
{
	if (this->texture == nullptr)
	{
		DebugLogError("Can't query height of null SDL_Texture.");
		return 0;
	}

	int height;
	const int status = SDL_QueryTexture(this->texture, nullptr, nullptr, nullptr, &height);
	if (status != 0)
	{
		DebugLogError("Couldn't query SDL_Texture height (" + std::string(SDL_GetError()) + ").");
		return 0;
	}

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

void Texture::destroy()
{
	if (this->texture != nullptr)
	{
		SDL_DestroyTexture(this->texture);
		this->texture = nullptr;
	}
}
