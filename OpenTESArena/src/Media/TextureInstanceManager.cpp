#include <algorithm>

#include "SDL.h"

#include "TextureInstanceManager.h"
#include "../Rendering/Renderer.h"

namespace
{
	template <typename TextureT, typename Id>
	int GetNextFreeID(std::vector<TextureT> &textures, std::vector<Id> &freeIDs,
		std::unordered_map<Id, int> &refCounts)
	{
		static_assert(std::is_integral_v<Id>);

		Id id;
		if (freeIDs.size() > 0)
		{
			id = freeIDs.back();
			freeIDs.pop_back();
		}
		else
		{
			textures.push_back(TextureT());
			id = static_cast<Id>(textures.size()) - 1;
		}

		// A free ID should never have refCount > 0.
		DebugAssert(refCounts.find(id) == refCounts.end());
		refCounts.emplace(std::make_pair(id, 0));

		return id;
	}

	template <typename Id>
	bool TryIncrementRefCount(Id id, std::unordered_map<Id, int> &refCounts)
	{
		const auto iter = refCounts.find(id);
		if (iter != refCounts.end())
		{
			iter->second++;
			return true;
		}
		else
		{
			return false;
		}
	}

	template <typename Id>
	void DecrementRefCount(Id id, std::vector<Id> &freeIDs, std::unordered_map<Id, int> &refCounts)
	{
		const auto iter = refCounts.find(id);
		if (iter != refCounts.end())
		{
			iter->second--;
			if (iter->second == 0)
			{
				refCounts.erase(iter);
				freeIDs.push_back(id);
			}
		}
	}
}

ImageInstanceID TextureInstanceManager::getNextFreeImageID()
{
	return GetNextFreeID(this->images, this->freeImageIDs, this->imageRefCounts);
}

SurfaceInstanceID TextureInstanceManager::getNextFreeSurfaceID()
{
	return GetNextFreeID(this->surfaces, this->freeSurfaceIDs, this->surfaceRefCounts);
}

TextureInstanceID TextureInstanceManager::getNextFreeTextureID()
{
	return GetNextFreeID(this->textures, this->freeTextureIDs, this->textureRefCounts);
}

ImageInstanceID TextureInstanceManager::makeImage(int width, int height, const PaletteID *paletteID)
{
	ImageInstanceID id = this->getNextFreeImageID();
	if (id == TextureInstanceManager::NO_ID)
	{
		DebugLogError("Couldn't get free image ID (" + std::to_string(width) + "x" +
			std::to_string(height) + ", palette: " + ((paletteID != nullptr) ? "yes" : "no") + ").");
		return TextureInstanceManager::NO_ID;
	}

	Image &image = this->images[id];
	image.init(width, height, paletteID);
	return id;
}

SurfaceInstanceID TextureInstanceManager::makeSurface(int width, int height)
{
	SurfaceInstanceID id = this->getNextFreeSurfaceID();
	if (id == TextureInstanceManager::NO_ID)
	{
		DebugLogError("Couldn't get free surface ID (" + std::to_string(width) +
			"x" + std::to_string(height) + ").");
		return TextureInstanceManager::NO_ID;
	}

	Surface &surface = this->surfaces[id];
	surface = Surface::createWithFormat(width, height, Renderer::DEFAULT_BPP,
		Renderer::DEFAULT_PIXELFORMAT);
	return id;
}

TextureInstanceID TextureInstanceManager::makeTexture(int width, int height, Renderer &renderer)
{
	TextureInstanceID id = this->getNextFreeTextureID();
	if (id == TextureInstanceManager::NO_ID)
	{
		DebugLogError("Couldn't get free texture ID (" + std::to_string(width) +
			"x" + std::to_string(height) + ").");
		return TextureInstanceManager::NO_ID;
	}

	Texture &texture = this->textures[id];
	texture = renderer.createTexture(Renderer::DEFAULT_PIXELFORMAT,
		SDL_TEXTUREACCESS_STREAMING, width, height);
	return id;
}

ImageInstanceRef TextureInstanceManager::getImageRef(ImageInstanceID id) const
{
	return ImageInstanceRef(&this->images, id);
}

SurfaceInstanceRef TextureInstanceManager::getSurfaceRef(SurfaceInstanceID id) const
{
	return SurfaceInstanceRef(&this->surfaces, id);
}

TextureInstanceRef TextureInstanceManager::getTextureRef(TextureInstanceID id) const
{
	return TextureInstanceRef(&this->textures, id);
}

Image &TextureInstanceManager::getImageHandle(ImageInstanceID id)
{
	return this->images[id];
}

const Image &TextureInstanceManager::getImageHandle(ImageInstanceID id) const
{
	return this->images[id];
}

Surface &TextureInstanceManager::getSurfaceHandle(SurfaceInstanceID id)
{
	return this->surfaces[id];
}

const Surface &TextureInstanceManager::getSurfaceHandle(SurfaceInstanceID id) const
{
	return this->surfaces[id];
}

Texture &TextureInstanceManager::getTextureHandle(TextureInstanceID id)
{
	return this->textures[id];
}

const Texture &TextureInstanceManager::getTextureHandle(TextureInstanceID id) const
{
	return this->textures[id];
}

bool TextureInstanceManager::tryIncrementImageRefCount(ImageInstanceID id)
{
	return TryIncrementRefCount(id, this->imageRefCounts);
}

bool TextureInstanceManager::tryIncrementSurfaceRefCount(SurfaceInstanceID id)
{
	return TryIncrementRefCount(id, this->surfaceRefCounts);
}

bool TextureInstanceManager::tryIncrementTextureRefCount(TextureInstanceID id)
{
	return TryIncrementRefCount(id, this->textureRefCounts);
}

void TextureInstanceManager::decrementImageRefCount(ImageInstanceID id)
{
	DecrementRefCount(id, this->freeImageIDs, this->imageRefCounts);
	if (this->imageRefCounts.find(id) == this->imageRefCounts.end())
	{
		// Free image.
		Image &image = this->images[id];
		image.clear();
	}
}

void TextureInstanceManager::decrementSurfaceRefCount(SurfaceInstanceID id)
{
	DecrementRefCount(id, this->freeSurfaceIDs, this->surfaceRefCounts);
	if (this->surfaceRefCounts.find(id) == this->surfaceRefCounts.end())
	{
		// Free surface.
		Surface &surface = this->surfaces[id];
		surface.clear();
	}
}

void TextureInstanceManager::decrementTextureRefCount(TextureInstanceID id)
{
	DecrementRefCount(id, this->freeTextureIDs, this->textureRefCounts);
	if (this->textureRefCounts.find(id) == this->textureRefCounts.end())
	{
		// Free texture.
		Texture &texture = this->textures[id];
		texture.clear();
	}
}

void TextureInstanceManager::clear()
{
	this->images.clear();
	this->surfaces.clear();
	this->textures.clear();

	this->imageRefCounts.clear();
	this->surfaceRefCounts.clear();
	this->textureRefCounts.clear();

	this->freeImageIDs.clear();
	this->freeSurfaceIDs.clear();
	this->freeTextureIDs.clear();
}
