#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "Palette.h"
#include "../Rendering/Surface.h"
#include "../Rendering/Texture.h"

class Renderer;

class TextureManager
{
private:
	std::unordered_map<std::string, Palette> palettes;

	// The filename and palette name are concatenated when mapping to avoid using two 
	// maps. I.e., "EQUIPMEN.IMG" and "PAL.COL" become "EQUIPMEN.IMGPAL.COL".
	std::unordered_map<std::string, Surface> surfaces;
	std::unordered_map<std::string, Texture> textures;
	std::unordered_map<std::string, std::vector<Surface>> surfaceSets;
	std::unordered_map<std::string, std::vector<Texture>> textureSets;
	std::string activePalette;

	// Specialty method for loading a COL file into the palettes map.
	void loadCOLPalette(const std::string &colName);

	// Specialty method for loading the palette from an IMG file into the palettes map.
	void loadIMGPalette(const std::string &imgName);

	// Helper method for loading a palette file into the palettes map.
	void loadPalette(const std::string &paletteName);
public:
	~TextureManager();

	TextureManager &operator=(TextureManager &&textureManager) = delete;

	// Creates a 32-bit image with the given dimensions and settings from an 8-bit image
	// and a 256 color palette.
	static Surface make32BitFromPaletted(int width, int height,
		const uint8_t *srcPixels, const Palette &palette);

	// Gets a surface by filename. It will be loaded if not already stored with the 
	// requested palette. If no palette name is given, the active one is used.
	const Surface &getSurface(const std::string &filename, const std::string &paletteName);
	const Surface &getSurface(const std::string &filename);

	// Similar to getSurface() but for hardware-accelerated textures.
	const Texture &getTexture(const std::string &filename, const std::string &paletteName,
		Renderer &renderer);
	const Texture &getTexture(const std::string &filename, Renderer &renderer);
	
	// Gets a set of surfaces by filename. Intended for files that contain a collection
	// of individual images.
	const std::vector<Surface> &getSurfaces(const std::string &filename,
		const std::string &paletteName);
	const std::vector<Surface> &getSurfaces(const std::string &filename);

	// Similar to getSurfaces() but for hardware-accelerated textures.
	const std::vector<Texture> &getTextures(const std::string &filename,
		const std::string &paletteName, Renderer &renderer);
	const std::vector<Texture> &getTextures(const std::string &filename, Renderer &renderer);

	void init();

	// Sets the palette to use for subsequent images. The source of the palette can be
	// from a loose .COL file, or can be built into an .IMG. If the .IMG does not have a 
	// built-in palette, an error occurs.
	void setPalette(const std::string &paletteName);
};

#endif
