#include <cassert>

#include "SDL.h"

#include "PaletteFile.h"
#include "PaletteName.h"
#include "TextureManager.h"
#include "../Assets/CFAFile.h"
#include "../Assets/CIFFile.h"
#include "../Assets/COLFile.h"
#include "../Assets/Compression.h"
#include "../Assets/DFAFile.h"
#include "../Assets/FLCFile.h"
#include "../Assets/IMGFile.h"
#include "../Assets/RCIFile.h"
#include "../Assets/SETFile.h"
#include "../Math/Vector2.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/Surface.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

TextureManager::TextureManager(Renderer &renderer)
	: renderer(renderer), palettes(), surfaces(), textures(),
	surfaceSets(), textureSets()
{
	DebugMention("Initializing.");

	// Load default palette.
	this->setPalette(PaletteFile::fromName(PaletteName::Default));
}

TextureManager::~TextureManager()
{
	// Release the SDL_Surfaces.
	for (auto &pair : this->surfaces)
	{
		SDL_FreeSurface(pair.second);
	}

	for (auto &pair : this->surfaceSets)
	{
		for (auto *surface : pair.second)
		{
			SDL_FreeSurface(surface);
		}
	}
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
	const bool isCOL = extension == ".COL";
	const bool isIMG = extension == ".IMG";
	const bool isMNU = extension == ".MNU";

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
		DebugCrash("Unrecognized palette \"" + paletteName + "\".");
	}

	// Make sure everything above works as intended.
	assert(this->palettes.find(paletteName) != this->palettes.end());
}

SDL_Surface *TextureManager::getSurface(const std::string &filename,
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
	const bool useBuiltInPalette = Palette::isBuiltIn(paletteName);

	// See if the palette hasn't already been loaded.
	const bool paletteIsLoaded = this->palettes.find(paletteName) != this->palettes.end();
	const bool imagePaletteIsLoaded = this->palettes.find(filename) != this->palettes.end();
	if ((!useBuiltInPalette && !paletteIsLoaded) ||
		(useBuiltInPalette && !imagePaletteIsLoaded))
	{
		// Use the filename (i.e., TAMRIEL.IMG) if using the built-in palette.
		// Otherwise, use the given palette name (i.e., PAL.COL).
		this->loadPalette(useBuiltInPalette ? filename : paletteName);
	}

	// The image hasn't been loaded with the palette yet, so make a new entry.
	// Check what kind of file extension the filename has.
	const std::string extension = String::getExtension(filename);
	const bool isCOL = extension == ".COL";
	const bool isIMG = extension == ".IMG";
	const bool isMNU = extension == ".MNU";

	SDL_Surface *surface = nullptr;

	if (isCOL)
	{
		// A palette was requested as the primary image. Convert it to a surface.
		Palette colPalette;
		COLFile::toPalette(filename, colPalette);

		assert(colPalette.get().size() == 256);
		surface = Surface::createSurfaceWithFormat(16, 16, Renderer::DEFAULT_BPP, 
			Renderer::DEFAULT_PIXELFORMAT);
		
		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		for (size_t i = 0; i < colPalette.get().size(); ++i)
		{
			pixels[i] = colPalette.get()[i].toARGB();
		}
	}
	else if (isIMG || isMNU)
	{
		// Decide if the IMG will use its own palette or not.
		const Palette *palette = useBuiltInPalette ? nullptr : 
			&this->palettes.at(paletteName);

		// Load the IMG file.
		IMGFile img(filename, palette);
		
		// Create a surface from the IMG.
		surface = Surface::createSurfaceWithFormat(img.getWidth(), img.getHeight(),
			Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
		SDL_memcpy(surface->pixels, img.getPixels(), surface->pitch * surface->h);
	}
	else
	{
		DebugCrash("Unrecognized surface format \"" + filename + "\".");
	}

	// Add the new surface and return it.
	auto iter = this->surfaces.emplace(std::make_pair(fullName, surface)).first;
	return iter->second;
}

SDL_Surface *TextureManager::getSurface(const std::string &filename)
{
	return this->getSurface(filename, this->activePalette);
}

const Texture &TextureManager::getTexture(const std::string &filename,
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
	// Attempt to use the image's built-in palette if requested.
	const bool useBuiltInPalette = Palette::isBuiltIn(paletteName);

	// See if the palette hasn't already been loaded.
	const bool paletteIsLoaded = this->palettes.find(paletteName) != this->palettes.end();
	const bool imagePaletteIsLoaded = this->palettes.find(filename) != this->palettes.end();
	if ((!useBuiltInPalette && !paletteIsLoaded) || 
		(useBuiltInPalette && !imagePaletteIsLoaded))
	{
		// Use the filename (i.e., TAMRIEL.IMG) if using the built-in palette.
		// Otherwise, use the given palette name (i.e., PAL.COL).
		this->loadPalette(useBuiltInPalette ? filename : paletteName);
	}

	// The image hasn't been loaded with the palette yet, so make a new entry.
	// Check what kind of file extension the filename has.
	const std::string extension = String::getExtension(filename);
	const bool isIMG = extension == ".IMG";
	const bool isMNU = extension == ".MNU";

	SDL_Texture *texture = nullptr;

	if (isIMG || isMNU)
	{
		// Decide if the IMG will use its own palette or not.
		const Palette *palette = useBuiltInPalette ? nullptr : 
			&this->palettes.at(paletteName);

		// Load the IMG file.
		IMGFile img(filename, palette);

		// Create a texture from the IMG.
		texture = this->renderer.createTexture(Renderer::DEFAULT_PIXELFORMAT,
			SDL_TEXTUREACCESS_STATIC, img.getWidth(), img.getHeight());

		uint32_t *pixels = img.getPixels();
		SDL_UpdateTexture(texture, nullptr, pixels, img.getWidth() * sizeof(*pixels));

		// Set alpha transparency on.
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	}
	else
	{
		DebugCrash("Unrecognized texture format \"" + filename + "\".");
	}

	// Add the new texture and return it.
	auto iter = this->textures.emplace(std::make_pair(fullName, Texture(texture))).first;
	return iter->second;
}

const Texture &TextureManager::getTexture(const std::string &filename)
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
	DebugAssert(!Palette::isBuiltIn(paletteName), 
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
	const bool isCFA = extension == ".CFA";
	const bool isCIF = extension == ".CIF";
	const bool isCEL = extension == ".CEL";
	const bool isDFA = extension == ".DFA";
	const bool isFLC = extension == ".FLC";
	const bool isRCI = extension == ".RCI";
	const bool isSET = extension == ".SET";

	if (isCFA)
	{
		// Load the CFA file.
		CFAFile cfaFile(filename, palette);

		// Create an SDL_Surface for each image in the CFA.
		const int imageCount = cfaFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			uint32_t *pixels = cfaFile.getPixels(i);
			SDL_Surface *surface = Surface::createSurfaceWithFormat(
				cfaFile.getWidth(), cfaFile.getHeight(),
				Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
			SDL_memcpy(surface->pixels, pixels, surface->pitch * surface->h);

			surfaceSet.push_back(surface);
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
			SDL_Surface *surface = Surface::createSurfaceWithFormat(
				cifFile.getWidth(i), cifFile.getHeight(i),
				Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
			SDL_memcpy(surface->pixels, pixels, surface->pitch * surface->h);

			surfaceSet.push_back(surface);
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
			SDL_Surface *surface = Surface::createSurfaceWithFormat(
				dfaFile.getWidth(), dfaFile.getHeight(),
				Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
			SDL_memcpy(surface->pixels, pixels, surface->pitch * surface->h);

			surfaceSet.push_back(surface);
		}
	}
	else if (isFLC || isCEL)
	{
		// Load the FLC file. CELs are basically identical to FLCs.
		FLCFile flcFile(filename);

		// Create an SDL_Surface for each frame in the FLC.
		const int imageCount = flcFile.getFrameCount();
		for (int i = 0; i < imageCount; ++i)
		{
			uint32_t *pixels = flcFile.getPixels(i);
			SDL_Surface *surface = Surface::createSurfaceWithFormat(
				flcFile.getWidth(), flcFile.getHeight(),
				Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
			SDL_memcpy(surface->pixels, pixels, surface->pitch * surface->h);

			surfaceSet.push_back(surface);
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
			SDL_Surface *surface = Surface::createSurfaceWithFormat(
				RCIFile::FRAME_WIDTH, RCIFile::FRAME_HEIGHT,
				Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
			SDL_memcpy(surface->pixels, pixels, surface->pitch * surface->h);

			surfaceSet.push_back(surface);
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
			SDL_Surface *surface = Surface::createSurfaceWithFormat(
				SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT,
				Renderer::DEFAULT_BPP, Renderer::DEFAULT_PIXELFORMAT);
			SDL_memcpy(surface->pixels, pixels, surface->pitch * surface->h);

			surfaceSet.push_back(surface);
		}
	}
	else
	{
		DebugCrash("Unrecognized surface list \"" + filename + "\".");
	}

	return surfaceSet;
}

const std::vector<SDL_Surface*> &TextureManager::getSurfaces(const std::string &filename)
{
	return this->getSurfaces(filename, this->activePalette);
}

const std::vector<Texture> &TextureManager::getTextures(
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
	DebugAssert(!Palette::isBuiltIn(paletteName), 
		"Image sets (i.e., .SET files) do not have built-in palettes.");

	// See if the palette hasn't already been loaded.
	if (this->palettes.find(paletteName) == this->palettes.end())
	{
		this->loadPalette(paletteName);
	}

	// The file hasn't been loaded with the palette yet, so make a new entry.
	auto iter = this->textureSets.emplace(std::make_pair(
		fullName, std::vector<Texture>())).first;

	std::vector<Texture> &textureSet = iter->second;
	const Palette &palette = this->palettes.at(paletteName);

	const std::string extension = String::getExtension(filename);
	const bool isCFA = extension == ".CFA";
	const bool isCIF = extension == ".CIF";
	const bool isCEL = extension == ".CEL";
	const bool isDFA = extension == ".DFA";
	const bool isFLC = extension == ".FLC";
	const bool isRCI = extension == ".RCI";
	const bool isSET = extension == ".SET";

	if (isCFA)
	{
		// Load the CFA file.
		CFAFile cfaFile(filename, palette);

		// Create an SDL_Texture for each image in the CFA.
		const int imageCount = cfaFile.getImageCount();
		for (int i = 0; i < imageCount; ++i)
		{
			SDL_Texture *texture = this->renderer.createTexture(
				Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STATIC,
				cfaFile.getWidth(), cfaFile.getHeight());

			const uint32_t *pixels = cfaFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				cfaFile.getWidth() * sizeof(*pixels));

			textureSet.push_back(Texture(texture));
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
				Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STATIC,
				cifFile.getWidth(i), cifFile.getHeight(i));

			const uint32_t *pixels = cifFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				cifFile.getWidth(i) * sizeof(*pixels));

			textureSet.push_back(Texture(texture));
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
				Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STATIC,
				dfaFile.getWidth(), dfaFile.getHeight());

			const uint32_t *pixels = dfaFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				dfaFile.getWidth() * sizeof(*pixels));

			textureSet.push_back(Texture(texture));
		}
	}
	else if (isFLC || isCEL)
	{
		// Load the FLC file. CELs are basically identical to FLCs.
		FLCFile flcFile(filename);

		// Create an SDL_Texture for each frame in the FLC.
		const int imageCount = flcFile.getFrameCount();
		for (int i = 0; i < imageCount; ++i)
		{
			SDL_Texture *texture = this->renderer.createTexture(
				Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STATIC,
				flcFile.getWidth(), flcFile.getHeight());

			const uint32_t *pixels = flcFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				flcFile.getWidth() * sizeof(*pixels));

			textureSet.push_back(Texture(texture));
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
				Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STATIC,
				RCIFile::FRAME_WIDTH, RCIFile::FRAME_HEIGHT);

			const uint32_t *pixels = rciFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				RCIFile::FRAME_WIDTH * sizeof(*pixels));

			textureSet.push_back(Texture(texture));
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
				Renderer::DEFAULT_PIXELFORMAT, SDL_TEXTUREACCESS_STATIC,
				SETFile::CHUNK_WIDTH, SETFile::CHUNK_HEIGHT);

			const uint32_t *pixels = setFile.getPixels(i);
			SDL_UpdateTexture(texture, nullptr, pixels,
				SETFile::CHUNK_WIDTH * sizeof(*pixels));

			textureSet.push_back(Texture(texture));
		}
	}
	else
	{
		DebugCrash("Unrecognized texture list \"" + filename + "\".");
	}

	// Set alpha transparency on for each texture.
	for (auto &texture : textureSet)
	{
		SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
	}

	return textureSet;
}

const std::vector<Texture> &TextureManager::getTextures(const std::string &filename)
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
