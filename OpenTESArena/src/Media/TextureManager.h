#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Image.h"
#include "Palette.h"
#include "TextureBuilder.h"
#include "TextureUtils.h"
#include "../Interface/Surface.h"
#include "../Interface/Texture.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferRef.h"
#include "components/utilities/BufferRef2D.h"

class Renderer;

// BufferRef variations for avoiding returning easily-stale handles from texture manager.
// All references are read-only interfaces.
using PaletteRef = BufferRef<const std::vector<Palette>, const Palette>;
using ImageRef = BufferRef2D<const std::vector<Image>, const Image>;
using SurfaceRef = BufferRef2D<const std::vector<Surface>, const Surface>;
using TextureRef = BufferRef2D<const std::vector<Texture>, const Texture>;
using TextureBuilderRef = BufferRef2D<const std::vector<TextureBuilder>, const TextureBuilder>;

class TextureManager
{
private:
	// Mappings of texture filenames to their ID(s). 32-bit texture functions need to accept a
	// palette ID and append it to the texture name behind the scenes so the same texture filename
	// can map to different instances depending on the palette.
	std::unordered_map<std::string, PaletteIdGroup> paletteIDs;
	std::unordered_map<std::string, TextureUtils::ImageIdGroup> imageIDs;
	std::unordered_map<std::string, TextureUtils::SurfaceIdGroup> surfaceIDs;
	std::unordered_map<std::string, TextureUtils::TextureIdGroup> textureIDs;
	std::unordered_map<std::string, TextureBuilderIdGroup> textureBuilderIDs;

	// Texture data for each texture type. Any groups of textures from the same filename are
	// stored contiguously in the order they appear in the file.
	std::vector<Palette> palettes;
	std::vector<Image> images;
	std::vector<Surface> surfaces;
	std::vector<Texture> textures;
	std::vector<TextureBuilder> textureBuilders;

	// Returns whether the given filename has the given extension.
	static bool matchesExtension(const char *filename, const char *extension);

	// Texture name mapping function, for combining a texture name with an optional
	// palette ID so the same texture name can be used with multiple palettes.
	static std::string makeTextureMappingName(const char *filename, const std::optional<PaletteID> &paletteID);

	// Helper functions for loading texture files.
	static bool tryLoadPalettes(const char *filename, Buffer<Palette> *outPalettes);
	static bool tryLoadImages(const char *filename, const std::optional<PaletteID> &paletteID,
		Buffer<Image> *outImages);
	static bool tryLoadSurfaces(const char *filename, const Palette &palette,
		Buffer<Surface> *outSurfaces);
	static bool tryLoadTextures(const char *filename, const Palette &palette,
		Renderer &renderer, Buffer<Texture> *outTextures);
	static bool tryLoadTextureBuilders(const char *filename, Buffer<TextureBuilder> *outTextures);
public:
	static constexpr int NO_ID = -1;

	// Texture ID retrieval functions, loading texture data if not loaded. All required palettes
	// must be loaded by the caller in advance -- no palettes are loaded in non-palette loader
	// functions. If the requested file has multiple images but the caller requested only one, the
	// returned ID will be for the first image. Similarly, if the file has a single image but the
	// caller expected several, the returned ID group will have only one ID.
	bool tryGetPaletteIDs(const char *filename, PaletteIdGroup *outIDs);
	bool tryGetImageIDs(const char *filename, const std::optional<PaletteID> &paletteID,
		TextureUtils::ImageIdGroup *outIDs);
	bool tryGetImageIDs(const char *filename, TextureUtils::ImageIdGroup *outIDs);
	bool tryGetSurfaceIDs(const char *filename, PaletteID paletteID, TextureUtils::SurfaceIdGroup *outIDs);
	bool tryGetTextureIDs(const char *filename, PaletteID paletteID, Renderer &renderer,
		TextureUtils::TextureIdGroup *outIDs);
	std::optional<TextureBuilderIdGroup> tryGetTextureBuilderIDs(const char *filename);
	bool tryGetPaletteID(const char *filename, PaletteID *outID);
	bool tryGetImageID(const char *filename, const std::optional<PaletteID> &paletteID, ImageID *outID);
	bool tryGetImageID(const char *filename, ImageID *outID);
	bool tryGetSurfaceID(const char *filename, PaletteID paletteID, SurfaceID *outID);
	bool tryGetTextureID(const char *filename, PaletteID paletteID, Renderer &renderer, TextureID *outID);
	std::optional<TextureBuilderID> tryGetTextureBuilderID(const char *filename);

	// Texture getter functions, fast look-up. These return reference wrappers to avoid
	// dangling pointer issues with internal buffer resizing.
	PaletteRef getPaletteRef(PaletteID id) const;
	ImageRef getImageRef(ImageID id) const;
	SurfaceRef getSurfaceRef(SurfaceID id) const;
	TextureRef getTextureRef(TextureID id) const;
	TextureBuilderRef getTextureBuilderRef(TextureBuilderID id) const;

	// Texture getter functions, fast look-up. These do not protect against dangling pointers.
	const Palette &getPaletteHandle(PaletteID id) const;
	const Image &getImageHandle(ImageID id) const;
	const Surface &getSurfaceHandle(SurfaceID id) const;
	const Texture &getTextureHandle(TextureID id) const;
	const TextureBuilder &getTextureBuilderHandle(TextureBuilderID id) const;
};

#endif
