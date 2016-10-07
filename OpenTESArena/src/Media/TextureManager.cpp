#include <cassert>

#include "SDL.h"

#include "TextureManager.h"

#include "Color.h"
#include "PaletteFile.h"
#include "PaletteName.h"
#include "TextureFile.h"
#include "TextureSequenceName.h"
#include "../Assets/CFAFile.h"
#include "../Assets/CIFFile.h"
#include "../Assets/COLFile.h"
#include "../Assets/Compression.h"
#include "../Assets/DFAFile.h"
#include "../Assets/FLCFile.h"
#include "../Assets/IMGFile.h"
#include "../Assets/RCIFile.h"
#include "../Assets/SETFile.h"
#include "../Math/Int2.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

TextureManager::TextureManager(Renderer &renderer)
	: renderer(renderer), palettes(), surfaces(), textures(),
	surfaceSets(), textureSets()
{
	Debug::mention("Texture Manager", "Initializing.");

	// Load default palette.
	this->setPalette(PaletteFile::fromName(PaletteName::Default));
}

TextureManager::~TextureManager()
{
	// Release the SDL_Surfaces.
	for (auto &pair : this->surfaceSets)
	{
		for (auto *surface : pair.second)
		{
			SDL_FreeSurface(surface);
		}
	}

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

void TextureManager::loadCOLPalette(const std::string &colName)
{
	Palette dstPalette;
	COLFile::toPalette(colName, dstPalette);
	this->palettes.emplace(std::make_pair(colName, dstPalette));
}

void TextureManager::loadIMGPalette(const std::string &imgName)
{
	Palette dstPalette;
	IMGFile::extractPalette(imgName, dstPalette);
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

bool TextureManager::paletteIsBuiltIn(const std::string &paletteName) const
{
	const std::string &builtInName = PaletteFile::fromName(PaletteName::BuiltIn);
	return paletteName.compare(builtInName) == 0;
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
	const bool useBuiltInPalette = this->paletteIsBuiltIn(paletteName);

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
			img.getPixels(), img.getWidth(), img.getHeight(), Renderer::DEFAULT_BPP,
			img.getWidth() * sizeof(*img.getPixels()), 0, 0, 0, 0);

		// Optimize the surface.
		optSurface = SDL_ConvertSurface(unoptSurface, this->renderer.getFormat(), 0);
		SDL_FreeSurface(unoptSurface);
	}
	else
	{
		Debug::crash("Texture Manager", "Unrecognized surface format \"" + filename + "\".");
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

const std::vector<SDL_Surface*> &TextureManager::getSurfaces(
	const std::string &filename, const std::string &paletteName)
{
	// This method deals with animations and movies, so it will check filenames 
	// for ".CFA", ".CIF", ".DFA", ".FLC", ".SET", etc..

	// Use this name when interfacing with the surface sets map.
	const std::string fullName = filename + paletteName;

	// See if the file has already been loaded with the palette.
	auto setIter = this->surfaceSets.find(fullName);
	if (setIter != this->surfaceSets.end())
	{
		// The requested texture set exists.
		return setIter->second;
	}

	// Do not use a built-in palette for surface sets.
	Debug::check(!this->paletteIsBuiltIn(paletteName), "Texture Manager",
		"Image sets (i.e., .SET files) do not have built-in palettes.");

	// See if the palette hasn't already been loaded.
	if (this->palettes.find(paletteName) == this->palettes.end())
	{
		this->loadPalette(paletteName);
	}

	// The file hasn't been loaded with the palette yet, so make a new entry.
	auto iter = this->surfaceSets.emplace(std::make_pair(
		fullName, std::vector<SDL_Surface*>())).first;

	std::vector<SDL_Surface*> &surfaceSet = iter->second;
	const Palette &palette = this->palettes.at(paletteName);

	const std::string extension = String::getExtension(filename);
	const bool isCFA = extension.compare(".CFA") == 0;
	const bool isCIF = extension.compare(".CIF") == 0;
	const bool isDFA = extension.compare(".DFA") == 0;
	const bool isFLC = extension.compare(".FLC") == 0;
	const bool isRCI = extension.compare(".RCI") == 0;
	const bool isSET = extension.compare(".SET") == 0;

	if (isCFA)
	{
		// Load the CFA file.
		CFAFile cfaFile(filename, palette);

		// Create an SDL_Surface for each image in the CFA.
		const int imageCount = cfaFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			uint32_t *pixels = cfaFile.getPixels(i);
			SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels,
				cfaFile.getWidth(), cfaFile.getHeight(), Renderer::DEFAULT_BPP,
				sizeof(*pixels) * cfaFile.getWidth(), 0, 0, 0, 0);

			SDL_Surface *optSurface = SDL_ConvertSurface(surface,
				this->renderer.getFormat(), 0);
			SDL_FreeSurface(surface);

			surfaceSet.push_back(optSurface);
		}
	}
	else if (isCIF)
	{
		// Load the CIF file.
		CIFFile cifFile(filename, palette);

		// Create an SDL_Surface for each image in the CIF.
		const int imageCount = cifFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			uint32_t *pixels = cifFile.getPixels(i);
			SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels,
				cifFile.getWidth(i), cifFile.getHeight(i), Renderer::DEFAULT_BPP,
				sizeof(*pixels) * cifFile.getWidth(i), 0, 0, 0, 0);

			SDL_Surface *optSurface = SDL_ConvertSurface(surface,
				this->renderer.getFormat(), 0);
			SDL_FreeSurface(surface);

			surfaceSet.push_back(optSurface);
		}
	}
	else if (isDFA)
	{
		// Load the DFA file.
		DFAFile dfaFile(filename, palette);

		// Create an SDL_Surface for each image in the DFA.
		const int imageCount = dfaFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			uint32_t *pixels = dfaFile.getPixels(i);
			SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels,
				dfaFile.getWidth(), dfaFile.getHeight(), Renderer::DEFAULT_BPP,
				sizeof(*pixels) * dfaFile.getWidth(), 0, 0, 0, 0);

			SDL_Surface *optSurface = SDL_ConvertSurface(surface,
				this->renderer.getFormat(), 0);
			SDL_FreeSurface(surface);

			surfaceSet.push_back(optSurface);
		}
	}
	else if (isFLC)
	{
		// Load the FLC file.
		FLCFile flcFile(filename);

		// Create an SDL_Surface for each frame in the FLC.
		const int imageCount = flcFile.getFrameCount();
		for (int i = 0; i < imageCount; ++i)
		{
			uint32_t *pixels = flcFile.getPixels(i);
			SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels,
				flcFile.getWidth(), flcFile.getHeight(), Renderer::DEFAULT_BPP,
				sizeof(*pixels) * flcFile.getWidth(), 0, 0, 0, 0);

			SDL_Surface *optSurface = SDL_ConvertSurface(surface,
				this->renderer.getFormat(), 0);
			SDL_FreeSurface(surface);

			surfaceSet.push_back(optSurface);
		}
	}
	else if (isRCI)
	{
		// Load the RCI file.
		RCIFile rciFile(filename, palette);

		// Create an SDL_Surface for each image in the RCI.
		const int imageCount = rciFile.getCount();
		for (int i = 0; i < imageCount; ++i)
		{
			uint32_t *pixels = rciFile.getPixels(i);
			SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels,
				RCIFile::FRAME_WIDTH, RCIFile::FRAME_HEIGHT, Renderer::DEFAULT_BPP,
				sizeof(*pixels) * RCIFile::FRAME_WIDTH, 0, 0, 0, 0);

			SDL_Surface *optSurface = SDL_ConvertSurface(surface,
				this->renderer.getFormat(), 0);
			SDL_FreeSurface(surface);

			surfaceSet.push_back(optSurface);
		}
	}
	else if (isSET)
	{
		// Load the SET file.
		SETFile setFile(filename, palette);

		// Create an SDL_Surface for each image in the SET.
		const int imageCount = setFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			uint32_t *pixels = setFile.getPixels(i);
			SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(pixels,
				SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT, Renderer::DEFAULT_BPP,
				sizeof(*pixels) * SETFile::CHUNK_WIDTH, 0, 0, 0, 0);

			SDL_Surface *optSurface = SDL_ConvertSurface(surface,
				this->renderer.getFormat(), 0);
			SDL_FreeSurface(surface);

			surfaceSet.push_back(optSurface);
		}
	}
	else
	{
		Debug::crash("Texture Manager", "Unrecognized surface list \"" + filename + "\".");
	}

	return surfaceSet;
}

const std::vector<SDL_Surface*> &TextureManager::getSurfaces(const std::string &filename)
{
	return this->getSurfaces(filename, this->activePalette);
}

const std::vector<SDL_Texture*> &TextureManager::getTextures(
	const std::string &filename, const std::string &paletteName)
{
	// This method deals with animations and movies, so it will check filenames 
	// for ".CFA", ".CIF", ".DFA", ".FLC", ".SET", etc..

	// Use this name when interfacing with the texture sets map.
	const std::string fullName = filename + paletteName;

	// See if the file has already been loaded with the palette.
	auto setIter = this->textureSets.find(fullName);
	if (setIter != this->textureSets.end())
	{
		// The requested texture set exists.
		return setIter->second;
	}

	// Do not use a built-in palette for texture sets.
	Debug::check(!this->paletteIsBuiltIn(paletteName), "Texture Manager",
		"Image sets (i.e., .SET files) do not have built-in palettes.");

	// See if the palette hasn't already been loaded.
	if (this->palettes.find(paletteName) == this->palettes.end())
	{
		this->loadPalette(paletteName);
	}

	// The file hasn't been loaded with the palette yet, so make a new entry.
	auto iter = this->textureSets.emplace(std::make_pair(
		fullName, std::vector<SDL_Texture*>())).first;

	std::vector<SDL_Texture*> &textureSet = iter->second;
	const Palette &palette = this->palettes.at(paletteName);

	const std::string extension = String::getExtension(filename);
	const bool isCFA = extension.compare(".CFA") == 0;
	const bool isCIF = extension.compare(".CIF") == 0;
	const bool isDFA = extension.compare(".DFA") == 0;
	const bool isFLC = extension.compare(".FLC") == 0;
	const bool isRCI = extension.compare(".RCI") == 0;
	const bool isSET = extension.compare(".SET") == 0;

	if (isCFA)
	{
		// Load the CFA file.
		CFAFile cfaFile(filename, palette);

		// Create an SDL_Texture for each image in the CFA.
		const int imageCount = cfaFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			SDL_Texture *texture = this->renderer.createTexture(
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
				cfaFile.getWidth(), cfaFile.getHeight());

			const uint32_t *pixels = cfaFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				cfaFile.getWidth() * sizeof(*pixels));

			textureSet.push_back(texture);
		}
	}
	else if (isCIF)
	{
		// Load the CIF file.
		CIFFile cifFile(filename, palette);

		// Create an SDL_Texture for each image in the CIF.
		const int imageCount = cifFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			SDL_Texture *texture = this->renderer.createTexture(
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
				cifFile.getWidth(i), cifFile.getHeight(i));

			const uint32_t *pixels = cifFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				cifFile.getWidth(i) * sizeof(*pixels));

			textureSet.push_back(texture);
		}
	}
	else if (isDFA)
	{
		// Load the DFA file.
		DFAFile dfaFile(filename, palette);

		// Create an SDL_Texture for each image in the DFA.
		const int imageCount = dfaFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			SDL_Texture *texture = this->renderer.createTexture(
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
				dfaFile.getWidth(), dfaFile.getHeight());

			const uint32_t *pixels = dfaFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				dfaFile.getWidth() * sizeof(*pixels));

			textureSet.push_back(texture);
		}
	}
	else if (isFLC)
	{
		// Load the FLC file.
		FLCFile flcFile(filename);

		// Create an SDL_Texture for each frame in the FLC.
		const int imageCount = flcFile.getFrameCount();
		for (int i = 0; i < imageCount; ++i)
		{
			SDL_Texture *texture = this->renderer.createTexture(
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
				flcFile.getWidth(), flcFile.getHeight());

			const uint32_t *pixels = flcFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				flcFile.getWidth() * sizeof(*pixels));

			textureSet.push_back(texture);
		}
	}
	else if (isRCI)
	{
		// Load the RCI file.
		RCIFile rciFile(filename, palette);

		// Create an SDL_Texture for each image in the RCI.
		const int imageCount = rciFile.getCount();
		for (int i = 0; i < imageCount; ++i)
		{
			SDL_Texture *texture = this->renderer.createTexture(
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
				RCIFile::FRAME_WIDTH, RCIFile::FRAME_HEIGHT);

			const uint32_t *pixels = rciFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				RCIFile::FRAME_WIDTH * sizeof(*pixels));

			textureSet.push_back(texture);
		}
	}
	else if (isSET)
	{
		// Load the SET file.
		SETFile setFile(filename, palette);

		// Create an SDL_Texture for each image in the SET.
		const int imageCount = setFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			SDL_Texture *texture = this->renderer.createTexture(
				SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
				SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT);

			const uint32_t *pixels = setFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				SETFile::CHUNK_WIDTH * sizeof(*pixels));

			textureSet.push_back(texture);
		}
	}
	else
	{
		Debug::crash("Texture Manager", "Unrecognized texture list \"" + filename + "\".");
	}

	return textureSet;
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
