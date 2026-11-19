#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "TextureBuilder.h"
#include "TextureFileMetadata.h"
#include "TextureUtils.h"
#include "../Utilities/Palette.h"

#include "components/utilities/Buffer.h"

struct TextureAsset;

class TextureManager
{
private:
	// Mappings of texture filenames to indices/sequences of IDs.
	std::unordered_map<std::string, PaletteIdGroup> paletteIDs;
	std::unordered_map<std::string, TextureBuilderIdGroup> textureBuilderIDs;
	std::unordered_map<std::string, int> metadataIndices;

	// Texture data/metadata for each type. Any groups of textures from the same filename are stored contiguously
	// in the order they appear in the file.
	std::vector<Palette> palettes;
	std::vector<TextureBuilder> textureBuilders;
	std::vector<TextureFileMetadata> metadatas;

	// Returns whether the given filename has the given extension.
	static bool matchesExtension(const char *filename, const char *extension);

	// Helper functions for loading texture files.
	static bool tryLoadPalettes(const char *filename, Buffer<Palette> *outPalettes);
	static bool tryLoadTextureData(const char *filename, Buffer<TextureBuilder> *outTextures, TextureFileMetadata *outMetadata);
public:
	// Texture ID retrieval functions, loading texture data if not loaded. All required palettes
	// must be loaded by the caller in advance -- no palettes are loaded in non-palette loader
	// functions. If the requested file has multiple images but the caller requested only one, the
	// returned ID will be for the first image. Similarly, if the file has a single image but the
	// caller expected several, the returned ID group will have only one ID.
	PaletteIdGroup getPaletteIDs(const char *filename);
	PaletteID getPaletteID(const char *filename);
	PaletteID getPaletteID(const TextureAsset &textureAsset);
	TextureBuilderIdGroup getTextureBuilderIDs(const char *filename);
	TextureBuilderID getTextureBuilderID(const char *filename);
	TextureBuilderID getTextureBuilderID(const TextureAsset &textureAsset);
	TextureFileMetadataID getMetadataID(const char *filename);

	// Texture getter functions, fast look-up. These do not protect against dangling pointers.
	const Palette &getPaletteHandle(PaletteID id) const;
	const TextureBuilder &getTextureBuilderHandle(TextureBuilderID id) const;
	const TextureFileMetadata &getMetadataHandle(TextureFileMetadataID id) const;

	int getTotalBytes() const;
};
