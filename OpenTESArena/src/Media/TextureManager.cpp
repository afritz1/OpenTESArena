#include <algorithm>
#include <array>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

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

	typedef std::array<Color, 256> Palette;

	Palette DefaultPalette;

	template<typename T>
	void decode04Type(T src, T srcend, std::vector<uint8_t> &out)
	{
		auto dst = out.begin();

		std::array<uint8_t, 4096> history;
		std::fill(history.begin(), history.end(), 0);
		int historypos = 0;

		// This appears to be some form of LZ compression. It starts with a 1-byte-
		// wide bitmask, where each bit declares if the next pixel comes directly
		// from the input, or refers back to a previous run of output pixels that
		// get duplicated. After each bit in the mask is used, another byte is read
		// for another bitmask and the cycle repeats until the end of input.
		int bitcount = 0;
		int mask = 0;
		while (src != srcend)
		{
			if (!bitcount)
			{
				bitcount = 8;
				mask = *(src++);
			}
			else
				mask >>= 1;

			if ((mask & 1))
			{
				if (src == srcend)
					throw std::runtime_error("Unexpected end of image.");
				if (dst == out.end())
					throw std::runtime_error("Decoded image overflow.");
				history[historypos++ & 0x0FFF] = *src;
				*(dst++) = *(src++);
			}
			else
			{
				if (std::distance(src, srcend) < 2)
					throw std::runtime_error("Unexpected end of image.");
				uint8_t byte1 = *(src++);
				uint8_t byte2 = *(src++);
				int tocopy = (byte2 & 0x0F) + 3;
				int copypos = (((byte2 & 0xF0) << 4) | byte1) + 18;

				if (std::distance(dst, out.end()) < tocopy)
					throw std::runtime_error("Decoded image overflow.");

				for (int i = 0; i < tocopy; ++i)
				{
					*dst = history[copypos++ & 0x0FFF];
					history[historypos++ & 0x0FFF] = *(dst++);
				}
			}
			--bitcount;
		}

		std::fill(dst, out.end(), 0);
	}

	// These might be useful as public misc utility functions

	uint16_t getLE16(const uint8_t *buf)
	{
		return buf[0] | (buf[1] << 8);
	}

	uint32_t getLE32(const uint8_t *buf)
	{
		return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	}

	template<typename T>
	std::string to_hexstring(T val)
	{
		static_assert(std::is_integral<T>::value, "to_hexstring given non-integral type.");
		std::stringstream sstr;
		sstr << std::hex << val;
		return sstr.str();
	}

}

// This path might be obsolete soon.
const std::string TextureManager::PATH = "data/textures/";

TextureManager::TextureManager(const SDL_PixelFormat *format)
{
	Debug::mention("Texture Manager", "Initializing.");

	assert(format != nullptr);

	this->surfaces = std::map<std::string, Surface>();
	this->format = format;

	// Intialize PNG file loading.
	int imgFlags = IMG_INIT_PNG;
	Debug::check((IMG_Init(imgFlags) & imgFlags) != SDL_FALSE, "Texture Manager",
		"Couldn't initialize texture loader, " + std::string(IMG_GetError()));

	// Load default palette "PAL.COL".
	bool failed = false;
	std::array<uint8_t, 776> rawpal;
	VFS::IStreamPtr stream = VFS::Manager::get().open("PAL.COL");
	if (!stream)
	{
		Debug::mention("Texture Manager", "PAL.COL: Failed to open palette.");
		failed = true;
	}
	else
	{
		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		if (stream->gcount() != static_cast<std::streamsize>(rawpal.size()))
		{
			Debug::mention("Texture Manager", "PAL.COL: Failed to read palette, got " +
				std::to_string(stream->gcount()) + " bytes.");
			failed = true;
		}
	}
	if (!failed)
	{
		uint32_t len = getLE32(rawpal.data());
		uint32_t ver = getLE32(rawpal.data() + 4);
		if (len != 776)
		{
			Debug::mention("Texture Manager", "PAL.COL: Invalid palette length, " +
				std::to_string(len) + " bytes.");
			failed = true;
		}
		else if (ver != 0xb123)
		{
			Debug::mention("Texture Manager", "PAL.COL: Invalid palette version, 0x" +
				to_hexstring(ver) + ".");
			failed = true;
		}
	}
	if (!failed)
	{
		auto iter = rawpal.begin() + 8;
		/* First palette entry is transparent in 8-bit modes, so give it 0
		 * alpha. */
		uint8_t r = *(iter++);
		uint8_t g = *(iter++);
		uint8_t b = *(iter++);
		DefaultPalette[0] = Color(r, g, b, 0);
		/* Remaining are solid, so give them 255 alpha. */
		std::generate(DefaultPalette.begin() + 1, DefaultPalette.end(),
			[&iter]() -> Color
		{
			uint8_t r = *(iter++);
			uint8_t g = *(iter++);
			uint8_t b = *(iter++);
			return Color(r, g, b, 255);
		}
		);
	}
	if (failed)
	{
		// Generate a monochrome palette. Entry 0 is filled with 0 already, so skip it.
		uint8_t count = 0;
		std::generate(DefaultPalette.begin() + 1, DefaultPalette.end(),
			[&count]() -> Color
		{
			uint8_t c = ++count;
			return Color(c, c, c, 255);
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

SDL_Surface* TextureManager::loadImgFile(const std::string& fullPath)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(fullPath.c_str());
	Debug::check(stream != nullptr, "Texture Manager",
		"Could not open texture \"" + fullPath + "\".");

	std::array<uint8_t, 12> imghdr;
	stream->read(reinterpret_cast<char*>(imghdr.data()), imghdr.size());
	Debug::check(stream->gcount() == static_cast<std::streamsize>(imghdr.size()), 
		"Texture Manager", "Could not read texture \"" + fullPath + "\" header.");

	uint16_t xoff = getLE16(imghdr.data());
	uint16_t yoff = getLE16(imghdr.data() + 2);
	uint16_t width = getLE16(imghdr.data() + 4);
	uint16_t height = getLE16(imghdr.data() + 6);
	uint16_t flags = getLE16(imghdr.data() + 8);
	uint16_t srclen = getLE16(imghdr.data() + 10);

	std::vector<uint8_t> srcdata(srclen);
	stream->read(reinterpret_cast<char*>(srcdata.data()), srcdata.size());
	Debug::check(stream->gcount() == static_cast<std::streamsize>(srcdata.size()), 
		"Texture Manager", "Could not read texture \"" + fullPath + "\" data.");

	Palette custompal;
	if (flags & 0x0100)
	{
		std::array<uint8_t, 768> rawpal;

		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		Debug::check(stream->gcount() == static_cast<std::streamsize>(rawpal.size()), 
			"Texture Manager", "Could not read texture \"" + fullPath + "\" palette.");

		auto iter = rawpal.begin();
		/* Unlike COL files, embedded palettes are stored with components in
		 * the range of 0...63 rather than 0...255 (this was because old VGA
		 * hardware only had 6-bit DACs, giving a maximum intensity value of
		 * 63, while newer hardware had 8-bit DACs for up to 255.
		 */
		uint8_t r = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		uint8_t g = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		uint8_t b = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		custompal[0] = Color(r, g, b, 0);
		/* Remaining are solid, so give them 255 alpha. */
		std::generate(custompal.begin() + 1, custompal.end(),
			[&iter]() -> Color
		{
			uint8_t r = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			uint8_t g = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			uint8_t b = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			return Color(r, g, b, 255);
		}
		);
	}

	const Palette &palette = (flags & 0x0100) ? custompal : DefaultPalette;
	if ((flags & 0x00ff) == 0x0000)
	{
		// Uncompressed IMG.
		assert(srcdata.size() == width * height);

		// Create temporary ARGB surface.
		SDL_Surface *surface = SDL_CreateRGBSurface(
			0, width, height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000
		);
		if (SDL_LockSurface(surface) == 0)
		{
			uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
			std::transform(srcdata.begin(), srcdata.end(), pixels,
				[&palette](uint8_t col) -> uint32_t
			{
				return palette[col].toARGB();
			}
			);
			SDL_UnlockSurface(surface);
		}

		auto *optSurface = SDL_ConvertSurface(surface, this->format, 0);
		SDL_FreeSurface(surface);

		return optSurface;
	}
	if ((flags & 0x00ff) == 0x0004)
	{
		// Type 4 compression.
		std::vector<uint8_t> decomp(width * height);
		decode04Type(srcdata.begin(), srcdata.end(), decomp);

		// Create temporary ARGB surface
		SDL_Surface *surface = SDL_CreateRGBSurface(
			0, width, height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000
		);
		if (SDL_LockSurface(surface) == 0)
		{
			uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
			std::transform(decomp.begin(), decomp.end(), pixels,
				[&palette](uint8_t col) -> uint32_t
			{
				return palette[col].toARGB();
			}
			);
			SDL_UnlockSurface(surface);
		}

		auto *optSurface = SDL_ConvertSurface(surface, this->format, 0);
		SDL_FreeSurface(surface);

		return optSurface;
	}

	Debug::crash("Texture Manager", "Unhandled IMG flags, 0x" + to_hexstring(flags) + ".");
	return nullptr;
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

	auto dot = filename.rfind('.');
	if (dot != std::string::npos && filename.compare(dot, filename.length() - dot, ".IMG") == 0)
	{
		auto *optSurface = this->loadImgFile(filename);

		// Create surface from SDL_Surface. No need to optimize it again.
		Surface surface(optSurface);

		// Add the new texture.
		auto iter = this->surfaces.insert(std::make_pair(filename, surface)).first;
		SDL_FreeSurface(optSurface);
		return iter->second;
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
