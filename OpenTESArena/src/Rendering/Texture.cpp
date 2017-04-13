#include <cassert>

#include "SDL.h"

#include "Texture.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"

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

SDL_Texture *Texture::generate(Texture::PatternType type, int width, int height,
	TextureManager &textureManager, Renderer &renderer)
{
	// Initialize the scratch surface to transparent.
	SDL_Surface *surface = Surface::createSurfaceWithFormat(width, height,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
	SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 0));

	if (type == Texture::PatternType::Parchment)
	{
		// Minimum dimensions of parchment pop-up.
		assert(width >= 40);
		assert(height >= 40);

		// Get the nine parchment tiles.
		const auto &tiles = textureManager.getSurfaces(
			TextureFile::fromName(TextureName::Parchment), "STARTGAM.MNU");

		// Four corner tiles.
		SDL_Surface *topLeft = tiles.at(0);
		SDL_Surface *topRight = tiles.at(2);
		SDL_Surface *bottomLeft = tiles.at(6);
		SDL_Surface *bottomRight = tiles.at(8);

		// Four side tiles.
		SDL_Surface *top = tiles.at(1);
		SDL_Surface *left = tiles.at(3);
		SDL_Surface *right = tiles.at(5);
		SDL_Surface *bottom = tiles.at(7);

		// One body tile.
		SDL_Surface *body = tiles.at(4);

		// Draw body tiles.
		for (int y = topLeft->h; y < (surface->h - topRight->h); y += body->h)
		{
			for (int x = topLeft->w; x < (surface->w - topRight->w); x += body->w)
			{
				SDL_Rect rect;
				rect.x = x;
				rect.y = y;
				rect.w = body->w;
				rect.h = body->h;

				SDL_BlitSurface(body, nullptr, surface, &rect);
			}
		}

		// Draw edge tiles.
		for (int y = topLeft->h; y < (surface->h - bottomLeft->h); y += left->h)
		{
			SDL_Rect leftRect;
			leftRect.x = 0;
			leftRect.y = y;
			leftRect.w = left->w;
			leftRect.h = left->h;

			SDL_Rect rightRect;
			rightRect.x = surface->w - right->w;
			rightRect.y = y;
			rightRect.w = right->w;
			rightRect.h = right->h;

			SDL_BlitSurface(left, nullptr, surface, &leftRect);
			SDL_BlitSurface(right, nullptr, surface, &rightRect);
		}

		for (int x = topLeft->w; x < (surface->w - topRight->w); x += top->w)
		{
			SDL_Rect topRect;
			topRect.x = x;
			topRect.y = 0;
			topRect.w = top->w;
			topRect.h = top->h;

			SDL_Rect bottomRect;
			bottomRect.x = x;
			bottomRect.y = surface->h - bottom->h;
			bottomRect.w = bottom->w;
			bottomRect.h = bottom->h;

			SDL_BlitSurface(top, nullptr, surface, &topRect);
			SDL_BlitSurface(bottom, nullptr, surface, &bottomRect);
		}

		// Draw corner tiles.
		SDL_Rect topLeftRect;
		topLeftRect.x = 0;
		topLeftRect.y = 0;
		topLeftRect.w = topLeft->w;
		topLeftRect.h = topLeft->h;

		SDL_Rect topRightRect;
		topRightRect.x = surface->w - topRight->w;
		topRightRect.y = 0;
		topRightRect.w = topRight->w;
		topRightRect.h = topRight->h;

		SDL_Rect bottomLeftRect;
		bottomLeftRect.x = 0;
		bottomLeftRect.y = surface->h - bottomLeft->h;
		bottomLeftRect.w = bottomLeft->w;
		bottomLeftRect.h = bottomLeft->h;

		SDL_Rect bottomRightRect;
		bottomRightRect.x = surface->w - bottomRight->w;
		bottomRightRect.y = surface->h - bottomRight->h;
		bottomRightRect.w = bottomRight->w;
		bottomRightRect.h = bottomRight->h;

		SDL_BlitSurface(topLeft, nullptr, surface, &topLeftRect);
		SDL_BlitSurface(topRight, nullptr, surface, &topRightRect);
		SDL_BlitSurface(bottomLeft, nullptr, surface, &bottomLeftRect);
		SDL_BlitSurface(bottomRight, nullptr, surface, &bottomRightRect);
	}
	else if (type == Texture::PatternType::Dark)
	{
		// Minimum dimensions of dark pop-up.
		assert(width >= 4);
		assert(height >= 4);

		// Get all the colors used with the dark pop-up.		
		const uint32_t fillColor = SDL_MapRGBA(surface->format, 28, 24, 36, 255);
		const uint32_t topColor = SDL_MapRGBA(surface->format, 36, 36, 48, 255);
		const uint32_t bottomColor = SDL_MapRGBA(surface->format, 12, 12, 24, 255);
		const uint32_t rightColor = SDL_MapRGBA(surface->format, 56, 69, 77, 255);
		const uint32_t leftColor = bottomColor;
		const uint32_t topRightColor = SDL_MapRGBA(surface->format, 69, 85, 89, 255);
		const uint32_t bottomRightColor = SDL_MapRGBA(surface->format, 36, 36, 48, 255);

		// Fill with dark-bluish color.
		SDL_FillRect(surface, nullptr, fillColor);

		// Color edges.
		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		for (int x = 0; x < surface->w; ++x)
		{
			pixels[x] = topColor;
			pixels[x + surface->w] = topColor;
			pixels[x + ((surface->h - 2) * surface->w)] = bottomColor;
			pixels[x + ((surface->h - 1) * surface->w)] = bottomColor;
		}

		for (int y = 0; y < surface->h; ++y)
		{
			pixels[y * surface->w] = leftColor;
			pixels[1 + (y * surface->w)] = leftColor;
			pixels[(surface->w - 2) + (y * surface->w)] = rightColor;
			pixels[(surface->w - 1) + (y * surface->w)] = rightColor;
		}

		// Color corners.
		pixels[1] = topColor;
		pixels[surface->w - 2] = topColor;
		pixels[surface->w - 1] = topRightColor;
		pixels[(surface->w - 2) + surface->w] = topRightColor;
		pixels[(surface->w - 2) + ((surface->h - 2) * surface->w)] = bottomRightColor;
		pixels[(surface->w - 2) + ((surface->h - 1) * surface->w)] = bottomColor;
		pixels[(surface->w - 1) + ((surface->h - 1) * surface->w)] = bottomRightColor;
	}
	else if (type == Texture::PatternType::Custom1)
	{
		// Minimum dimensions of light-gray pattern.
		assert(width >= 3);
		assert(height >= 3);

		const uint32_t fillColor = SDL_MapRGBA(surface->format, 85, 85, 97, 255);
		const uint32_t lightBorder = SDL_MapRGBA(surface->format, 125, 125, 145, 255);
		const uint32_t darkBorder = SDL_MapRGBA(surface->format, 40, 40, 48, 255);

		// Fill with light gray color.
		SDL_FillRect(surface, nullptr, fillColor);

		// Color edges.
		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		for (int x = 0; x < surface->w; ++x)
		{
			pixels[x] = lightBorder;
			pixels[x + ((surface->h - 1) * surface->w)] = darkBorder;
		}

		for (int y = 0; y < surface->h; ++y)
		{
			pixels[y * surface->w] = darkBorder;
			pixels[(surface->w - 1) + (y * surface->w)] = lightBorder;
		}

		// Color corners.
		pixels[0] = fillColor;
		pixels[(surface->w - 1) + ((surface->h - 1) * surface->w)] = fillColor;
	}
	else
	{
		Debug::crash("Texture", "Unrecognized pattern type.");
	}

	SDL_Texture *texture = renderer.createTextureFromSurface(surface);
	SDL_FreeSurface(surface);

	return texture;
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
