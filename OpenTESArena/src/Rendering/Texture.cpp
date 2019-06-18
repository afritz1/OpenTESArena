#include "SDL.h"

#include "Texture.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"

Texture::Texture()
{
	this->texture = nullptr;
}

Texture::Texture(SDL_Texture *texture)
{
	DebugAssert(texture != nullptr);
	this->texture = texture;
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
	this->texture = texture.texture;
	texture.texture = nullptr;
	return *this;
}

Texture Texture::generate(Texture::PatternType type, int width, int height,
	TextureManager &textureManager, Renderer &renderer)
{
	// Initialize the scratch surface to transparent.
	Surface surface = Surface::createWithFormat(width, height,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
	const uint32_t clearColor = surface.mapRGBA(0, 0, 0, 0);
	SDL_FillRect(surface.get(), nullptr, clearColor);

	if (type == Texture::PatternType::Parchment)
	{
		// Minimum dimensions of parchment pop-up.
		DebugAssert(width >= 40);
		DebugAssert(height >= 40);

		// Get the nine parchment tiles.
		const auto &tiles = textureManager.getSurfaces(
			TextureFile::fromName(TextureName::Parchment), "STARTGAM.MNU");

		// Four corner tiles.
		SDL_Surface *topLeft = tiles.at(0).get();
		SDL_Surface *topRight = tiles.at(2).get();
		SDL_Surface *bottomLeft = tiles.at(6).get();
		SDL_Surface *bottomRight = tiles.at(8).get();

		// Four side tiles.
		SDL_Surface *top = tiles.at(1).get();
		SDL_Surface *left = tiles.at(3).get();
		SDL_Surface *right = tiles.at(5).get();
		SDL_Surface *bottom = tiles.at(7).get();

		// One body tile.
		SDL_Surface *body = tiles.at(4).get();

		// Draw body tiles.
		for (int y = topLeft->h; y < (surface.getHeight() - topRight->h); y += body->h)
		{
			for (int x = topLeft->w; x < (surface.getWidth() - topRight->w); x += body->w)
			{
				SDL_Rect rect;
				rect.x = x;
				rect.y = y;
				rect.w = body->w;
				rect.h = body->h;

				SDL_BlitSurface(body, nullptr, surface.get(), &rect);
			}
		}

		// Draw edge tiles.
		for (int y = topLeft->h; y < (surface.getHeight() - bottomLeft->h); y += left->h)
		{
			SDL_Rect leftRect;
			leftRect.x = 0;
			leftRect.y = y;
			leftRect.w = left->w;
			leftRect.h = left->h;

			SDL_Rect rightRect;
			rightRect.x = surface.getWidth() - right->w;
			rightRect.y = y;
			rightRect.w = right->w;
			rightRect.h = right->h;

			// Remove any traces of body tiles underneath.
			SDL_FillRect(surface.get(), &leftRect, clearColor);
			SDL_FillRect(surface.get(), &rightRect, clearColor);

			SDL_BlitSurface(left, nullptr, surface.get(), &leftRect);
			SDL_BlitSurface(right, nullptr, surface.get(), &rightRect);
		}

		for (int x = topLeft->w; x < (surface.getWidth() - topRight->w); x += top->w)
		{
			SDL_Rect topRect;
			topRect.x = x;
			topRect.y = 0;
			topRect.w = top->w;
			topRect.h = top->h;

			SDL_Rect bottomRect;
			bottomRect.x = x;
			bottomRect.y = surface.getHeight() - bottom->h;
			bottomRect.w = bottom->w;
			bottomRect.h = bottom->h;

			// Remove any traces of other tiles underneath.
			SDL_FillRect(surface.get(), &topRect, clearColor);
			SDL_FillRect(surface.get(), &bottomRect, clearColor);

			SDL_BlitSurface(top, nullptr, surface.get(), &topRect);
			SDL_BlitSurface(bottom, nullptr, surface.get(), &bottomRect);
		}

		// Draw corner tiles.
		SDL_Rect topLeftRect;
		topLeftRect.x = 0;
		topLeftRect.y = 0;
		topLeftRect.w = topLeft->w;
		topLeftRect.h = topLeft->h;

		SDL_Rect topRightRect;
		topRightRect.x = surface.getWidth() - topRight->w;
		topRightRect.y = 0;
		topRightRect.w = topRight->w;
		topRightRect.h = topRight->h;

		SDL_Rect bottomLeftRect;
		bottomLeftRect.x = 0;
		bottomLeftRect.y = surface.getHeight() - bottomLeft->h;
		bottomLeftRect.w = bottomLeft->w;
		bottomLeftRect.h = bottomLeft->h;

		SDL_Rect bottomRightRect;
		bottomRightRect.x = surface.getWidth() - bottomRight->w;
		bottomRightRect.y = surface.getHeight() - bottomRight->h;
		bottomRightRect.w = bottomRight->w;
		bottomRightRect.h = bottomRight->h;

		// Remove any traces of other tiles underneath.
		SDL_FillRect(surface.get(), &topLeftRect, clearColor);
		SDL_FillRect(surface.get(), &topRightRect, clearColor);
		SDL_FillRect(surface.get(), &bottomLeftRect, clearColor);
		SDL_FillRect(surface.get(), &bottomRightRect, clearColor);

		SDL_BlitSurface(topLeft, nullptr, surface.get(), &topLeftRect);
		SDL_BlitSurface(topRight, nullptr, surface.get(), &topRightRect);
		SDL_BlitSurface(bottomLeft, nullptr, surface.get(), &bottomLeftRect);
		SDL_BlitSurface(bottomRight, nullptr, surface.get(), &bottomRightRect);
	}
	else if (type == Texture::PatternType::Dark)
	{
		// Minimum dimensions of dark pop-up.
		DebugAssert(width >= 4);
		DebugAssert(height >= 4);

		// Get all the colors used with the dark pop-up.
		const uint32_t fillColor = surface.mapRGBA(28, 24, 36, 255);
		const uint32_t topColor = surface.mapRGBA(36, 36, 48, 255);
		const uint32_t bottomColor = surface.mapRGBA(12, 12, 24, 255);
		const uint32_t rightColor = surface.mapRGBA(56, 69, 77, 255);
		const uint32_t leftColor = bottomColor;
		const uint32_t topRightColor = surface.mapRGBA(69, 85, 89, 255);
		const uint32_t bottomRightColor = surface.mapRGBA(36, 36, 48, 255);

		// Fill with dark-bluish color.
		SDL_FillRect(surface.get(), nullptr, fillColor);

		// Color edges.
		uint32_t *pixels = static_cast<uint32_t*>(surface.get()->pixels);
		for (int x = 0; x < surface.getWidth(); x++)
		{
			pixels[x] = topColor;
			pixels[x + surface.getWidth()] = topColor;
			pixels[x + ((surface.getHeight() - 2) * surface.getWidth())] = bottomColor;
			pixels[x + ((surface.getHeight() - 1) * surface.getWidth())] = bottomColor;
		}

		for (int y = 0; y < surface.getHeight(); y++)
		{
			pixels[y * surface.getWidth()] = leftColor;
			pixels[1 + (y * surface.getWidth())] = leftColor;
			pixels[(surface.getWidth() - 2) + (y * surface.getWidth())] = rightColor;
			pixels[(surface.getWidth() - 1) + (y * surface.getWidth())] = rightColor;
		}

		// Color corners.
		pixels[1] = topColor;
		pixels[surface.getWidth() - 2] = topColor;
		pixels[surface.getWidth() - 1] = topRightColor;
		pixels[(surface.getWidth() - 2) + surface.getWidth()] = topRightColor;
		pixels[(surface.getWidth() - 2) + ((surface.getHeight() - 2) * surface.getWidth())] = bottomRightColor;
		pixels[(surface.getWidth() - 2) + ((surface.getHeight() - 1) * surface.getWidth())] = bottomColor;
		pixels[(surface.getWidth() - 1) + ((surface.getHeight() - 1) * surface.getWidth())] = bottomRightColor;
	}
	else if (type == Texture::PatternType::Custom1)
	{
		// Minimum dimensions of light-gray pattern.
		DebugAssert(width >= 3);
		DebugAssert(height >= 3);

		const uint32_t fillColor = surface.mapRGBA(85, 85, 97, 255);
		const uint32_t lightBorder = surface.mapRGBA(125, 125, 145, 255);
		const uint32_t darkBorder = surface.mapRGBA(40, 40, 48, 255);

		// Fill with light gray color.
		SDL_FillRect(surface.get(), nullptr, fillColor);

		// Color edges.
		uint32_t *pixels = static_cast<uint32_t*>(surface.get()->pixels);
		for (int x = 0; x < surface.getWidth(); x++)
		{
			pixels[x] = lightBorder;
			pixels[x + ((surface.getHeight() - 1) * surface.getWidth())] = darkBorder;
		}

		for (int y = 0; y < surface.getHeight(); y++)
		{
			pixels[y * surface.getWidth()] = darkBorder;
			pixels[(surface.getWidth() - 1) + (y * surface.getWidth())] = lightBorder;
		}

		// Color corners.
		pixels[0] = fillColor;
		pixels[(surface.getWidth() - 1) + ((surface.getHeight() - 1) * surface.getWidth())] = fillColor;
	}
	else
	{
		DebugCrash("Unrecognized pattern type.");
	}

	Texture texture(renderer.createTextureFromSurface(surface.get()));
	return texture;
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
