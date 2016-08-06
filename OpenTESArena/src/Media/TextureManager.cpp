#include <cassert>

#include "SDL.h"
#include "SDL_image.h"

#include "TextureManager.h"

#include "Color.h"
#include "PaletteName.h"
#include "TextureFile.h"
#include "../Assets/Compression.h"
#include "../Assets/IMGFile.h"
#include "../Interface/Surface.h"
#include "../Math/Int2.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

namespace
{
	const std::map<PaletteName, std::string> PaletteFilenames =
	{
		{ PaletteName::Default, "PAL.COL" },
		{ PaletteName::CharSheet, "CHARSHT.COL" },
		{ PaletteName::Daytime, "DAYTIME.COL" },
		{ PaletteName::Dreary, "DREARY.COL" }
	};
}

// This path should be removed once using original Arena files exclusively.
const std::string TextureManager::PATH = "data/textures/";

TextureManager::TextureManager(Renderer &renderer)
	: renderer(renderer)
{
	Debug::mention("Texture Manager", "Initializing.");

	this->palettes = std::map<PaletteName, Palette>();
	this->surfaces = std::unordered_map<std::string, std::map<PaletteName, Surface>>();
	this->textures = std::unordered_map<std::string, std::map<PaletteName, SDL_Texture*>>();
	this->surfaceSets = std::unordered_map<std::string, 
		std::map<PaletteName, std::vector<Surface>>>();
	this->textureSets = std::unordered_map<std::string,
		std::map<PaletteName, std::vector<SDL_Texture*>>>();

	// Load default palette.
	this->setPalette(PaletteName::Default);

	// Intialize PNG file loading.
	int imgFlags = IMG_INIT_PNG;
	Debug::check((IMG_Init(imgFlags) & imgFlags) != SDL_FALSE, "Texture Manager",
		"Couldn't initialize texture loader, " + std::string(IMG_GetError()));
}

TextureManager::~TextureManager()
{
	// Release the SDL_Textures.
	// The SDL_Renderer destroys these itself with SDL_DestroyRenderer(), too.
	for (auto &pair : this->textures)
	{
		for (auto &innerPair : pair.second)
		{
			SDL_DestroyTexture(innerPair.second);
		}
	}

	for (auto &pair : this->textureSets)
	{
		for (auto &innerPair : pair.second)
		{
			for (auto *texture : innerPair.second)
			{
				SDL_DestroyTexture(texture);
			}
		}
	}

	IMG_Quit();
}

TextureManager &TextureManager::operator=(TextureManager &&textureManager)
{
	this->palettes = std::move(textureManager.palettes);
	this->surfaces = std::move(textureManager.surfaces);
	this->textures = std::move(textureManager.textures);
	this->surfaceSets = std::move(textureManager.surfaceSets);
	this->textureSets = std::move(textureManager.textureSets);
	this->renderer = textureManager.renderer;
	this->activePalette = textureManager.activePalette;

	return *this;
}

SDL_Surface *TextureManager::loadPNG(const std::string &fullPath)
{
	// Load the SDL_Surface from file.
	auto *unOptSurface = IMG_Load(fullPath.c_str());
	Debug::check(unOptSurface != nullptr, "Texture Manager",
		"Could not open texture \"" + fullPath + "\".");

	// Try to optimize the SDL_Surface.
	auto *optSurface = SDL_ConvertSurface(unOptSurface, this->renderer.getFormat(), 0);
	SDL_FreeSurface(unOptSurface);
	Debug::check(optSurface != nullptr, "Texture Manager",
		"Could not optimize texture \"" + fullPath + "\".");

	return optSurface;
}

void TextureManager::initPalette(Palette &palette, PaletteName paletteName)
{
	bool failed = false;
	std::array<uint8_t, 776> rawpal;
	const std::string &filename = PaletteFilenames.at(paletteName);
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());

	if (!stream)
	{
		Debug::mention("Texture Manager",
			"Failed to open palette \"" + filename + "\".");
		failed = true;
	}
	else
	{
		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		if (stream->gcount() != static_cast<std::streamsize>(rawpal.size()))
		{
			Debug::mention("Texture Manager", "Failed to read palette \"" +
				filename + "\", got " + std::to_string(stream->gcount()) + " bytes.");
			failed = true;
		}
	}
	if (!failed)
	{
		uint32_t len = Compression::getLE32(rawpal.data());
		uint32_t ver = Compression::getLE32(rawpal.data() + 4);
		if (len != 776)
		{
			Debug::mention("Texture Manager", "Invalid length for palette \"" +
				filename + "\" (" + std::to_string(len) + " bytes).");
			failed = true;
		}
		else if (ver != 0xB123)
		{
			Debug::mention("Texture Manager", "Invalid version for palette \"" +
				filename + "\", 0x" + String::toHexString(ver) + ".");
			failed = true;
		}
	}
	if (!failed)
	{
		auto iter = rawpal.begin() + 8;

		/* First palette entry is transparent in 8-bit modes, so give it 0 alpha. */
		uint8_t r = *(iter++);
		uint8_t g = *(iter++);
		uint8_t b = *(iter++);
		palette[0] = Color(r, g, b, 0);

		/* Remaining are solid, so give them 255 alpha. */
		std::generate(palette.begin() + 1, palette.end(),
			[&iter]() -> Color
		{
			uint8_t r = *(iter++);
			uint8_t g = *(iter++);
			uint8_t b = *(iter++);
			return Color(r, g, b, 255);
		});
	}
	if (failed)
	{
		// Generate a monochrome palette. Entry 0 is filled with 0 already, so skip it.
		uint8_t count = 0;
		std::generate(palette.begin() + 1, palette.end(),
			[&count]() -> Color
		{
			uint8_t c = ++count;
			return Color(c, c, c, 255);
		});
	}
}

const Surface &TextureManager::getSurface(const std::string &filename,
	PaletteName paletteName)
{
	// See if the image file already exists with any palette. Otherwise,
	// decide how to load it further down.
	auto surfaceIter = this->surfaces.find(filename);
	if (surfaceIter != this->surfaces.end())
	{
		// Now see if the image exists with the requested palette.
		const auto &paletteMap = surfaceIter->second;
		auto paletteIter = paletteMap.find(paletteName);

		if (paletteIter != paletteMap.end())
		{
			// The requested surface exists.
			return paletteIter->second;
		}
	}
	else
	{
		// The image hasn't been loaded with any palettes yet, so make a new entry.
		surfaceIter = this->surfaces.emplace(std::make_pair(
			filename, std::map<PaletteName, Surface>())).first;
	}

	// Check what kind of file extension is used. Every texture should have an
	// extension, so the "dot position" might be unnecessary once PNGs are no
	// longer used.
	size_t dotPos = filename.rfind('.');
	bool hasDot = (dotPos < filename.length()) && (dotPos != std::string::npos);
	bool isIMG = hasDot &&
		(filename.compare(dotPos, filename.length() - dotPos, ".IMG") == 0);
	bool isMNU = hasDot &&
		(filename.compare(dotPos, filename.length() - dotPos, ".MNU") == 0);

	SDL_Surface *optSurface = nullptr;

	if (isIMG || isMNU)
	{
		// Decide if the IMG will use its own palette or not.
		Palette *palette = (paletteName == PaletteName::BuiltIn) ? nullptr :
			&this->palettes.at(paletteName);

		// Load the IMG file.
		IMGFile img(filename, palette, paletteName);

		// Create an unoptimized SDL surface from the IMG.
		SDL_Surface *unoptSurface = SDL_CreateRGBSurfaceFrom(
			img.getPixels(), img.getWidth(), img.getHeight(), Surface::DEFAULT_BPP,
			img.getWidth() * sizeof(uint32_t),
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

		// Optimize the surface.
		optSurface = SDL_ConvertSurface(unoptSurface, this->renderer.getFormat(), 0);
		SDL_FreeSurface(unoptSurface);
	}
	else
	{
		// PNG file. This "else" case should eventually throw a runtime error instead
		// once PNGs aren't used anymore.
		std::string fullPath(TextureManager::PATH + filename + ".png");
		optSurface = this->loadPNG(fullPath);
	}

	// Create surface from optimized SDL_Surface.
	Surface surface(optSurface);
	SDL_FreeSurface(optSurface);

	// Add the new surface and return it.
	auto &paletteMap = surfaceIter->second;	
	auto iter = paletteMap.emplace(std::make_pair(paletteName, surface)).first;
	return iter->second;
}

const Surface &TextureManager::getSurface(const std::string &filename)
{
	return this->getSurface(filename, this->activePalette);
}

SDL_Texture *TextureManager::getTexture(const std::string &filename,
	PaletteName paletteName)
{
	// See if the image file already exists with any palette. Otherwise,
	// decide how to load it further down.
	auto textureIter = this->textures.find(filename);
	if (textureIter != this->textures.end())
	{
		// Now see if the image exists with the requested palette.
		const auto &paletteMap = textureIter->second;
		auto paletteIter = paletteMap.find(paletteName);

		if (paletteIter != paletteMap.end())
		{
			// The requested texture exists.
			return paletteIter->second;
		}
	}
	else
	{
		// The image hasn't been loaded with any palettes yet, so make a new entry.
		textureIter = this->textures.emplace(std::make_pair(
			filename, std::map<PaletteName, SDL_Texture*>())).first;
	}

	// Make a texture from the surface. It's okay if the surface isn't used except
	// for, say, texture dimensions (instead of doing SDL_QueryTexture()).
	const Surface &surface = this->getSurface(filename, paletteName);		
	SDL_Texture *texture = this->renderer.createTextureFromSurface(surface);
		
	// Add the new texture and return it.
	auto &paletteMap = textureIter->second;
	auto iter = paletteMap.emplace(std::make_pair(paletteName, texture)).first;
	return iter->second;	
}

SDL_Texture *TextureManager::getTexture(const std::string &filename)
{
	return this->getTexture(filename, this->activePalette);
}

const std::vector<Surface> &TextureManager::getSurfaces(const std::string &filename, 
	PaletteName paletteName)
{
	// I would like this method to deal with the animations and movies, so it'll 
	// check filenames for ".CFA", ".CIF", ".DFA", ".FLC", etc..

	Debug::crash("Texture Manager", "getSurfaces() not implemented.");
	return this->surfaceSets.at("").at(PaletteName::Default); // Dummy placeholder.
}

const std::vector<Surface> &TextureManager::getSurfaces(const std::string &filename)
{
	return this->getSurfaces(filename, this->activePalette);
}

const std::vector<SDL_Texture*> &TextureManager::getTextures(const std::string &filename, 
	PaletteName paletteName)
{
	// This method will just take the surfaces from getSurfaces() and turn them into 
	// SDL_Textures if not already loaded. I'm not too worried about memory consumption 
	// and unused surfaces at this point. A fullscreen (320x200) uncompressed 32-bit 
	// image is only 256KB, and with all the movies and animations combined, that's 
	// like... 450MB? I think an "unloadTexture()" method is a bit too far ahead right now.

	Debug::crash("Texture Manager", "getTextures() not implemented.");
	return this->textureSets.at("").at(PaletteName::Default); // Dummy placeholder.
}

const std::vector<SDL_Texture*> &TextureManager::getTextures(const std::string &filename)
{
	return this->getTextures(filename, this->activePalette);
}

void TextureManager::setPalette(PaletteName paletteName)
{
	// Error if the palette name is "built-in".
	assert(paletteName != PaletteName::BuiltIn);

	this->activePalette = paletteName;

	if (this->palettes.find(paletteName) == this->palettes.end())
	{
		Palette palette;
		this->initPalette(palette, paletteName);
		this->palettes.emplace(std::make_pair(paletteName, palette));
	}
}

void TextureManager::preloadSequences()
{
	Debug::mention("Texture Manager", "Preloading sequences.");

	for (const auto name : TextureFile::getSequenceNames())
	{
		std::vector<std::string> filenames = TextureFile::fromName(name);
		for (const auto &filename : filenames)
		{
			this->getTexture(filename);
		}
	}
}
