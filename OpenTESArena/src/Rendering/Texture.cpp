#include "SDL.h"

#include "Texture.h"
#include "../Math/Rect.h"
#include "../Media/TextureFile.h"
#include "../Media/TextureManager.h"
#include "../Media/TextureName.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"

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

Texture Texture::generate(Texture::PatternType type, int width, int height,
	TextureManager &textureManager, Renderer &renderer)
{
	// Initialize the scratch surface to transparent.
	Surface surface = Surface::createWithFormat(width, height,
		Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
	const uint32_t clearColor = surface.mapRGBA(0, 0, 0, 0);
	surface.fill(clearColor);

	if (type == Texture::PatternType::Parchment)
	{
		// Minimum dimensions of parchment pop-up.
		DebugAssert(width >= 40);
		DebugAssert(height >= 40);

		// Get the nine parchment tiles.
		const std::string tilesPaletteFilename = "STARTGAM.MNU";
		PaletteID tilesPaletteID;
		if (!textureManager.tryGetPaletteID(tilesPaletteFilename.c_str(), &tilesPaletteID))
		{
			DebugCrash("Couldn't get palette ID for \"" + tilesPaletteFilename + "\".");
		}

		const std::string &tilesFilename = TextureFile::fromName(TextureName::Parchment);
		TextureManager::IdGroup<SurfaceID> tileIDs;
		if (!textureManager.tryGetSurfaceIDs(tilesFilename.c_str(), tilesPaletteID, &tileIDs))
		{
			DebugCrash("Couldn't get surface IDs for \"" + tilesFilename + "\".");
		}

		// Four corner tiles.
		SDL_Surface *topLeft = textureManager.getSurfaceHandle(tileIDs.getID(0)).get();
		SDL_Surface *topRight = textureManager.getSurfaceHandle(tileIDs.getID(2)).get();
		SDL_Surface *bottomLeft = textureManager.getSurfaceHandle(tileIDs.getID(6)).get();
		SDL_Surface *bottomRight = textureManager.getSurfaceHandle(tileIDs.getID(8)).get();

		// Four side tiles.
		SDL_Surface *top = textureManager.getSurfaceHandle(tileIDs.getID(1)).get();
		SDL_Surface *left = textureManager.getSurfaceHandle(tileIDs.getID(3)).get();
		SDL_Surface *right = textureManager.getSurfaceHandle(tileIDs.getID(5)).get();
		SDL_Surface *bottom = textureManager.getSurfaceHandle(tileIDs.getID(7)).get();

		// One body tile.
		SDL_Surface *body = textureManager.getSurfaceHandle(tileIDs.getID(4)).get();

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
			const Rect leftRect(0, y, left->w, left->h);
			const Rect rightRect(surface.getWidth() - right->w, y, right->w, right->h);

			// Remove any traces of body tiles underneath.
			surface.fillRect(leftRect, clearColor);
			surface.fillRect(rightRect, clearColor);

			SDL_BlitSurface(left, nullptr, surface.get(), const_cast<SDL_Rect*>(&leftRect.getRect()));
			SDL_BlitSurface(right, nullptr, surface.get(), const_cast<SDL_Rect*>(&rightRect.getRect()));
		}

		for (int x = topLeft->w; x < (surface.getWidth() - topRight->w); x += top->w)
		{
			const Rect topRect(x, 0, top->w, top->h);
			const Rect bottomRect(x, surface.getHeight() - bottom->h, bottom->w, bottom->h);

			// Remove any traces of other tiles underneath.
			surface.fillRect(topRect, clearColor);
			surface.fillRect(bottomRect, clearColor);

			SDL_BlitSurface(top, nullptr, surface.get(), const_cast<SDL_Rect*>(&topRect.getRect()));
			SDL_BlitSurface(bottom, nullptr, surface.get(), const_cast<SDL_Rect*>(&bottomRect.getRect()));
		}

		// Draw corner tiles.
		const Rect topLeftRect(0, 0, topLeft->w, topLeft->h);
		const Rect topRightRect(surface.getWidth() - topRight->w, 0, topRight->w, topRight->h);
		const Rect bottomLeftRect(0, surface.getHeight() - bottomLeft->h, bottomLeft->w, bottomLeft->h);
		const Rect bottomRightRect(surface.getWidth() - bottomRight->w,
			surface.getHeight() - bottomRight->h, bottomRight->w, bottomRight->h);

		// Remove any traces of other tiles underneath.
		surface.fillRect(topLeftRect, clearColor);
		surface.fillRect(topRightRect, clearColor);
		surface.fillRect(bottomLeftRect, clearColor);
		surface.fillRect(bottomRightRect, clearColor);

		SDL_BlitSurface(topLeft, nullptr, surface.get(), const_cast<SDL_Rect*>(&topLeftRect.getRect()));
		SDL_BlitSurface(topRight, nullptr, surface.get(), const_cast<SDL_Rect*>(&topRightRect.getRect()));
		SDL_BlitSurface(bottomLeft, nullptr, surface.get(), const_cast<SDL_Rect*>(&bottomLeftRect.getRect()));
		SDL_BlitSurface(bottomRight, nullptr, surface.get(), const_cast<SDL_Rect*>(&bottomRightRect.getRect()));
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
		surface.fill(fillColor);

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
		surface.fill(fillColor);

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

	Texture texture = renderer.createTextureFromSurface(surface);
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
