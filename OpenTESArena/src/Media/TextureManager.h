#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Palette.h"
#include "TextureBuilder.h"
#include "TextureFileMetadata.h"
#include "TextureUtils.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferRef.h"
#include "components/utilities/BufferRef2D.h"

struct TextureAssetReference;

// BufferRef variations for avoiding returning easily-stale handles from texture manager.
// All references are read-only interfaces.
using PaletteRef = BufferRef<const std::vector<Palette>, const Palette>;
using TextureBuilderRef = BufferRef2D<const std::vector<TextureBuilder>, const TextureBuilder>;

class TextureManager
{
private:
	// Mappings of texture filenames to sequences of IDs.
	std::unordered_map<std::string, PaletteIdGroup> paletteIDs;
	std::unordered_map<std::string, TextureBuilderIdGroup> textureBuilderIDs;

	// Texture data for each type. Any groups of textures from the same filename are stored contiguously
	// in the order they appear in the file.
	std::vector<Palette> palettes;
	std::vector<TextureBuilder> textureBuilders;

	// Returns whether the given filename has the given extension.
	static bool matchesExtension(const char *filename, const char *extension);

	// Helper functions for loading texture files.
	static bool tryLoadPalettes(const char *filename, Buffer<Palette> *outPalettes);
	static bool tryLoadTextureBuilders(const char *filename, Buffer<TextureBuilder> *outTextures);
public:
	// Returns metadata about a texture file if it exists and is valid.
	std::optional<TextureFileMetadata> tryGetMetadata(const char *filename);

	// Texture ID retrieval functions, loading texture data if not loaded. All required palettes
	// must be loaded by the caller in advance -- no palettes are loaded in non-palette loader
	// functions. If the requested file has multiple images but the caller requested only one, the
	// returned ID will be for the first image. Similarly, if the file has a single image but the
	// caller expected several, the returned ID group will have only one ID.
	std::optional<PaletteIdGroup> tryGetPaletteIDs(const char *filename);
	std::optional<PaletteID> tryGetPaletteID(const char *filename);
	std::optional<PaletteID> tryGetPaletteID(const TextureAssetReference &textureAssetRef);
	std::optional<TextureBuilderIdGroup> tryGetTextureBuilderIDs(const char *filename);
	std::optional<TextureBuilderID> tryGetTextureBuilderID(const char *filename);
	std::optional<TextureBuilderID> tryGetTextureBuilderID(const TextureAssetReference &textureAssetRef);

	// Texture getter functions, fast look-up. These return reference wrappers to avoid
	// dangling pointer issues with internal buffer resizing.
	PaletteRef getPaletteRef(PaletteID id) const;
	TextureBuilderRef getTextureBuilderRef(TextureBuilderID id) const;

	// Texture getter functions, fast look-up. These do not protect against dangling pointers.
	const Palette &getPaletteHandle(PaletteID id) const;
	const TextureBuilder &getTextureBuilderHandle(TextureBuilderID id) const;
};

#endif
