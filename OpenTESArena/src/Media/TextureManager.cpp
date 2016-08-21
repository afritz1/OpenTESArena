#include <cassert>

#include "SDL.h"
#include "SDL_image.h"

#include "TextureManager.h"

#include "Color.h"
#include "PaletteFile.h"
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

// This path should be removed once using original Arena files exclusively.
const std::string TextureManager::PATH = "data/textures/";

TextureManager::TextureManager(Renderer &renderer)
	: renderer(renderer), palettes(), surfaces(), textures(), 
	surfaceSets(), textureSets()
{
	Debug::mention("Texture Manager", "Initializing.");

	// Load default palette.
	this->setPalette(PaletteFile::fromName(PaletteName::Default));

	// Intialize PNG file loading (remove this code once PNGs are no longer used).
	int32_t imgFlags = IMG_INIT_PNG;
	Debug::check((IMG_Init(imgFlags) & imgFlags) != SDL_FALSE, "Texture Manager",
		"Couldn't initialize texture loader, " + std::string(IMG_GetError()));
}

TextureManager::~TextureManager()
{
	// Release the SDL_Textures.
	// The SDL_Renderer destroys these itself with SDL_DestroyRenderer(), too.
	for (auto &pair : this->textures)
	{
		SDL_DestroyTexture(pair.second);
	}

	for (auto &pair : this->textureSets)
	{
		for (auto *texture : pair.second)
		{
			SDL_DestroyTexture(texture);
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

void TextureManager::loadCOLPalette(const std::string &colName)
{
	bool failed = false;
	std::array<uint8_t, 776> rawpal;
	VFS::IStreamPtr stream = VFS::Manager::get().open(colName.c_str());

	if (!stream)
	{
		Debug::mention("Texture Manager",
			"Failed to open palette \"" + colName + "\".");
		failed = true;
	}
	else
	{
		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		if (stream->gcount() != static_cast<std::streamsize>(rawpal.size()))
		{
			Debug::mention("Texture Manager", "Failed to read palette \"" +
				colName + "\", got " + std::to_string(stream->gcount()) + " bytes.");
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
				colName + "\" (" + std::to_string(len) + " bytes).");
			failed = true;
		}
		else if (ver != 0xB123)
		{
			Debug::mention("Texture Manager", "Invalid version for palette \"" +
				colName + "\", 0x" + String::toHexString(ver) + ".");
			failed = true;
		}
	}

	// The palette to write new colors into and then place in the palettes map.
	Palette palette;

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

	// Add the new palette into the palettes map.
	this->palettes.emplace(std::make_pair(colName, palette));
}

void TextureManager::loadIMGPalette(const std::string &imgName)
{
	Palette dstPalette;
	IMGFile::extractPalette(dstPalette, imgName);
	this->palettes.emplace(std::make_pair(imgName, dstPalette));
}

void TextureManager::loadPalette(const std::string &paletteName)
{
	// Don't load the same palette more than once.
	assert(this->palettes.find(paletteName) == this->palettes.end());

	// Get file extension of the palette name.
	const std::string extension = String::getExtension(paletteName);
	const bool isCOL = extension.compare(".COL") == 0;
	const bool isIMG = extension.compare(".IMG") == 0;
	const bool isMNU = extension.compare(".MNU") == 0;

	if (isCOL)
	{
		this->loadCOLPalette(paletteName);
	}
	else if (isIMG || isMNU)
	{
		this->loadIMGPalette(paletteName);
	}
	else
	{
		Debug::crash("Texture Manager", "Unrecognized palette \"" + paletteName + "\".");
	}

	// Make sure everything above works as intended.
	assert(this->palettes.find(paletteName) != this->palettes.end());
}

const Surface &TextureManager::getSurface(const std::string &filename,
	const std::string &paletteName)
{
	// Use this name when interfacing with the surfaces map.
	const std::string fullName = filename + paletteName;

	// See if the image file has already been loaded with the palette.
	auto surfaceIter = this->surfaces.find(fullName);
	if (surfaceIter != this->surfaces.end())
	{
		// The requested surface exists.
		return surfaceIter->second;
	}

	// Attempt to use the image's built-in palette if requested.
	const bool useBuiltInPalette = paletteName.compare(
		PaletteFile::fromName(PaletteName::BuiltIn)) == 0;

	// See if the palette hasn't already been loaded.
	if ((!useBuiltInPalette && (this->palettes.find(paletteName) == this->palettes.end())) ||
		(useBuiltInPalette && (this->palettes.find(filename) == this->palettes.end())))
	{
		// Use the filename (i.e., TAMRIEL.IMG) if using the built-in palette.
		// Otherwise, use the given palette name (i.e., PAL.COL).
		this->loadPalette(useBuiltInPalette ? filename : paletteName);
	}
	
	// The image hasn't been loaded with the palette yet, so make a new entry.
	// Check what kind of file extension is used (every texture should have an
	// extension, so the "dot position" might be unnecessary once PNGs are no
	// longer used).
	const std::string extension = String::getExtension(filename);
	const bool isIMG = extension.compare(".IMG") == 0;
	const bool isMNU = extension.compare(".MNU") == 0;

	SDL_Surface *optSurface = nullptr;

	if (isIMG || isMNU)
	{
		// Decide if the IMG will use its own palette or not.
		Palette *palette = useBuiltInPalette ? nullptr : &this->palettes.at(paletteName);

		// Load the IMG file.
		IMGFile img(filename, palette);

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
		// Assume PNG for now.
		// Remove this once PNGs aren't needed anymore.
		optSurface = this->loadPNG(TextureManager::PATH + filename + ".png");

		// Uncomment this once PNGs are gone.
		//Debug::crash("Texture Manager", "Unrecognized image \"" + filename + "\".");
	}

	// Create surface from optimized SDL_Surface.
	Surface surface(optSurface);
	SDL_FreeSurface(optSurface);

	// Add the new surface and return it.
	auto iter = this->surfaces.emplace(std::make_pair(fullName, surface)).first;
	return iter->second;
}

const Surface &TextureManager::getSurface(const std::string &filename)
{
	return this->getSurface(filename, this->activePalette);
}

SDL_Texture *TextureManager::getTexture(const std::string &filename,
	const std::string &paletteName)
{
	// Use this name when interfacing with the textures map.
	const std::string fullName = filename + paletteName;

	// See if the image file has already been loaded with the palette.
	auto textureIter = this->textures.find(fullName);
	if (textureIter != this->textures.end())
	{
		// The requested texture exists.
		return textureIter->second;
	}

	// The image hasn't been loaded with the palette yet, so make a new entry.
	// Make a texture from the surface. It's okay if the surface isn't used except
	// for, say, texture dimensions (instead of doing SDL_QueryTexture()).
	const Surface &surface = this->getSurface(filename, paletteName);
	SDL_Texture *texture = this->renderer.createTextureFromSurface(surface);
		
	// Add the new texture and return it.
	auto iter = this->textures.emplace(std::make_pair(fullName, texture)).first;
	return iter->second;	
}

SDL_Texture *TextureManager::getTexture(const std::string &filename)
{
	return this->getTexture(filename, this->activePalette);
}

const std::vector<Surface> &TextureManager::getSurfaces(const std::string &filename, 
	const std::string &paletteName)
{
	// I would like this method to deal with the animations and movies, so it'll 
	// check filenames for ".CFA", ".CIF", ".DFA", ".FLC", etc..

	Debug::crash("Texture Manager", "getSurfaces() not implemented.");
	return this->surfaceSets.at(""); // Dummy placeholder.
}

const std::vector<Surface> &TextureManager::getSurfaces(const std::string &filename)
{
	return this->getSurfaces(filename, this->activePalette);
}

const std::vector<SDL_Texture*> &TextureManager::getTextures(const std::string &filename, 
	const std::string &paletteName)
{
	// This method will just take the surfaces from getSurfaces() and turn them into 
	// SDL_Textures if not already loaded. I'm not too worried about memory consumption 
	// and unused surfaces at this point. A fullscreen (320x200) uncompressed 32-bit 
	// image is only 256KB, and with all the movies and animations combined, that's 
	// like... 450MB? I think an "unloadTexture()" method is a bit too far ahead right now.

	Debug::crash("Texture Manager", "getTextures() not implemented.");
	return this->textureSets.at(""); // Dummy placeholder.
}

const std::vector<SDL_Texture*> &TextureManager::getTextures(const std::string &filename)
{
	return this->getTextures(filename, this->activePalette);
}

void TextureManager::setPalette(const std::string &paletteName)
{
	// Check if the palette hasn't already been loaded.
	if (this->palettes.find(paletteName) == this->palettes.end())
	{
		this->loadPalette(paletteName);
	}

	this->activePalette = paletteName;
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
