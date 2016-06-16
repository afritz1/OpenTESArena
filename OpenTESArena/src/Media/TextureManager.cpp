#include <cassert>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "SDL.h"
#include "SDL_image.h"

#include "TextureManager.h"

#include "Color.h"
#include "TextureFile.h"
#include "../Interface/Surface.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

namespace
{

// The format extension for all textures will be PNG (this will soon no longer be
// the case).
const std::string TextureExtension = ".png";

typedef std::array<Color,256> Palette;

Palette DefaultPalette;

// These might be useful as public misc utility functions

uint32_t getLE32(const uint8_t *buf)
{
    return buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
}

template<typename T>
std::string to_hexstring(T val)
{
    static_assert(std::is_integral<T>::value, "to_hexstring given non-integral type");
    std::stringstream sstr;
    sstr<<std::hex<<val;
    return sstr.str();
}

}

// This path might be obsolete soon.
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

    bool failed = false;
    std::array<uint8_t,776> rawpal;
    VFS::IStreamPtr stream = VFS::Manager::get().open("PAL.COL");
    if(!stream)
    {
        Debug::mention("Texture Manager", "PAL.COL: Failed to open palette.");
        failed = true;
    }
    else
    {
        stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
        if(stream->gcount() != rawpal.size())
        {
            Debug::mention("Texture Manager", "PAL.COL: Failed to read palette, got "+
                                              std::to_string(stream->gcount())+" bytes.");
            failed = true;
        }
    }
    if(!failed)
    {
        uint32_t len = getLE32(rawpal.data());
        uint32_t ver = getLE32(rawpal.data()+4);
        if(len != 776)
        {
            Debug::mention("Texture Manager", "PAL.COL: Invalid palette length, "+
                                              std::to_string(len)+" bytes.");
            failed = true;
        }
        else if(ver != 0xb123)
        {
            Debug::mention("Texture Manager", "PAL.COL: Invalid palette version, 0x"+
                                              to_hexstring(ver)+".");
            failed = true;
        }
    }
    if(!failed)
    {
        auto iter = rawpal.begin() + 8;
        /* First palette entry is transparent in 8-bit modes, so give it 0
         * alpha. */
        uint8_t r = *(iter++);
        uint8_t g = *(iter++);
        uint8_t b = *(iter++);
        DefaultPalette[0] = Color(r, g, b, 0);
        /* Remaining are solid, so give them 255 alpha. */
        std::generate(DefaultPalette.begin()+1, DefaultPalette.end(),
            [&iter]() -> Color
            {
                uint8_t r = *(iter++);
                uint8_t g = *(iter++);
                uint8_t b = *(iter++);
                return {r, g, b, 255};
            }
        );
    }
    if(failed)
    {
        // Generate a monochrome palette. Entry 0 is filled with 0 already, so skip it.
        uint8_t count = 0;
        std::generate(DefaultPalette.begin()+1, DefaultPalette.end(),
            [&count]() -> Color
            {
                uint8_t c = ++count;
                return {c, c, c, 255};
            }
        );
    }
}

TextureManager::~TextureManager()
{
	IMG_Quit();
}

SDL_Surface *TextureManager::loadFromFile(const std::string &fullPath)
{
	// Load the SDL_Surface from file.
	auto *unOptSurface = IMG_Load(fullPath.c_str());
	Debug::check(unOptSurface != nullptr, "Texture Manager",
		"Could not open texture \"" + fullPath + "\".");

	// Try to optimize the SDL_Surface.
	auto *optSurface = SDL_ConvertSurface(unOptSurface, this->format, 0);
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
		std::string fullPath(TextureManager::PATH + filename + TextureExtension);
		auto *optSurface = this->loadFromFile(fullPath);

		// Create surface from SDL_Surface. No need to optimize it again.
		Surface surface(optSurface);

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
		std::vector<std::string> filenames = TextureFile::fromName(name);
		for (const auto &filename : filenames)
		{
			this->getSurface(filename);
		}
	}
}
