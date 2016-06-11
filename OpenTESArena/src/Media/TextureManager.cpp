#include <cassert>
#include <iostream>

#include "SDL.h"
#include "SDL_image.h"

#include "TextureManager.h"

#include "TextureFile.h"
#include "../Interface/Surface.h"
#include "../Utilities/Debug.h"

// The format extension for all textures will be PNG.
const std::string TextureExtension = ".png";

const std::string TextureManager::PATH = "data/textures/";

TextureManager::TextureManager(const SDL_PixelFormat *format)
{
	Debug::mention("Texture Manager", "Initializing.");

	assert(format != nullptr);

	// Intialize PNG file loading.
	int imgFlags = IMG_INIT_PNG;
	Debug::check((IMG_Init(imgFlags) & imgFlags) != SDL_FALSE, "Texture Manager",
		"Couldn't initialize texture loader, " + std::string(IMG_GetError()));

	this->surfaces = std::map<std::string, Surface>();
	this->format = format;

	assert(this->surfaces.size() == 0);
	assert(this->format == format);
}

TextureManager::~TextureManager()
{
	IMG_Quit();
}

SDL_Surface *TextureManager::loadFromFile(const std::string &fullPath)
{
	// Load the SDL_Surface from file.
	auto unOptSurface = IMG_Load(fullPath.c_str());
	Debug::check(unOptSurface != nullptr, "Texture Manager",
		"Could not open texture \"" + fullPath + "\".");

	// Try to optimize the SDL_Surface.
	auto optSurface = SDL_ConvertSurface(unOptSurface, this->format, 0);
	SDL_FreeSurface(unOptSurface);
	Debug::check(optSurface != nullptr, "Texture Manager",
		"Could not optimize texture \"" + fullPath + "\".");

	return optSurface;
}

const SDL_PixelFormat *TextureManager::getFormat() const
{
	return this->format;
}

const Surface &TextureManager::getSurface(const std::string &filename)
{
	if (this->surfaces.find(filename) != this->surfaces.end())
	{
		// Get the existing surface.
		const auto &surface = this->surfaces.at(filename);
		return surface;
	}
	else
	{
		// Load optimized SDL_Surface from file.
		auto fullPath = TextureManager::PATH + filename + TextureExtension;
		auto optSurface = this->loadFromFile(fullPath);

		// Create surface from SDL_Surface. No need to optimize it again.
		auto surface = Surface(optSurface);

		// Add the new texture.
		this->surfaces.insert(std::pair<std::string, Surface>(filename, surface));
		SDL_FreeSurface(optSurface);

		// Try this method again.
		assert(this->surfaces.find(filename) != this->surfaces.end());
		return this->getSurface(filename);
	}
}

void TextureManager::preloadSequences()
{
	Debug::mention("Texture Manager", "Preloading sequences.");

	for (const auto &name : TextureFile::getSequenceNames())
	{
		auto filenames = TextureFile::fromName(name);
		for (const auto &filename : filenames)
		{
			this->getSurface(filename);
		}
	}
}
