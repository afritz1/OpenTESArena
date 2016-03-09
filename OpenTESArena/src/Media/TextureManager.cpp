#include <cassert>
#include <iostream>

#include "SDL2\SDL.h"
#include "SDL2\SDL_image.h"

#include "TextureManager.h"
#include "../Interface/Surface.h"

// The filename of each TextureName. This should be private to the TextureManager.
const auto TextureFilenames = std::map<TextureName, std::string>
{

};

const std::string TextureManager::PATH = "data/textures/";

TextureManager::TextureManager(const SDL_PixelFormat *format)
{
	assert(format != nullptr);

	// Intialize PNG file loading.
	int imgFlags = IMG_INIT_PNG;
	if ((IMG_Init(imgFlags) & imgFlags) == SDL_FALSE)
	{
		std::cerr << "Texture Manager error: couldn't initialize texture loader." << "\n";
		std::cerr << IMG_GetError() << "\n";
		getchar();
		exit(EXIT_FAILURE);
	}

	this->surfaces = std::map<TextureName, Surface>();
	this->format = format;

	assert(this->format == format);
}

TextureManager::~TextureManager()
{
	IMG_Quit();
}

SDL_Surface *TextureManager::loadFromFile(TextureName name)
{
	// Path to the file.
	auto fullPath = TextureManager::PATH + TextureFilenames.at(name);

	// Load the SDL_Surface from file.
	auto unOptSurface = IMG_Load(fullPath.c_str());
	if (unOptSurface == nullptr)
	{
		std::cerr << "Texture Manager error: could not open texture \"" << 
			fullPath << "\"." << "\n";
		std::getchar();
		exit(EXIT_FAILURE);
	}

	// Try to optimize the SDL_Surface.
	auto optSurface = SDL_ConvertSurface(unOptSurface, this->format, 0);
	SDL_FreeSurface(unOptSurface);
	if (optSurface == nullptr)
	{
		std::cerr << "Texture Manager error: could not optimize texture \"" <<
			fullPath << "\". (Is SDL initialized?)" << "\n";
		std::getchar();
		exit(EXIT_FAILURE);
	}

	return optSurface;
}

const SDL_PixelFormat *TextureManager::getFormat() const
{
	return this->format;
}

const Surface &TextureManager::getSurface(TextureName name)
{
	if (this->surfaces.find(name) != this->surfaces.end())
	{
		// Get the existing surface.
		auto &surface = this->surfaces.at(name);
		return surface;
	}
	else
	{
		// Load optimized SDL_Surface from file.
		auto optSurface = this->loadFromFile(name);

		// Create surface from SDL_Surface. No need to optimize it again.
		auto surface = Surface(optSurface);

		// Add the new texture.
		this->surfaces.insert(std::pair<TextureName, Surface>(name, surface));

		// Try this method again.
		assert(this->surfaces.find(name) != this->surfaces.end());
		return this->getSurface(name);
	}
}