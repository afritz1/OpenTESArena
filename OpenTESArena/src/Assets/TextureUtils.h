#ifndef TEXTURE_UTILS_H
#define TEXTURE_UTILS_H

#include <string>
#include <vector>

#include "../Rendering/RenderTextureUtils.h"
#include "../Utilities/Palette.h"

#include "components/debug/Debug.h"
#include "components/utilities/Buffer.h"

// Various texture handles for use with texture manager.
using PaletteID = int; // 32-bit software surface (generally 256 texels)
using TextureBuilderID = int; // Intermediate 8/32-bit software surface.
using TextureFileMetadataID = int; // Metadata for a texture file (texture count, dimensions, etc.).

class FontLibrary;
class Renderer;
class Surface;
class Texture;
class TextureManager;

struct TextureAsset;

namespace TextureUtils
{
	// Generated texture types. These refer to patterns used with pop-ups and buttons.
	// @todo: move these to an Arena namespace eventually
	enum class PatternType
	{
		Parchment,
		Dark,
		Custom1 // Light gray with borders.
	};

	// Defines a contiguous group of IDs for referencing textures.
	template <typename T>
	struct IdGroup
	{
	private:
		static_assert(std::is_integral_v<T>);

		T startID;
		int count;
	public:
		IdGroup(T startID, int count)
		{
			this->startID = startID;
			this->count = count;
		}

		IdGroup() : IdGroup(-1, -1) { }

		int getCount() const
		{
			return this->count;
		}

		T getID(int index) const
		{
			DebugAssert(index >= 0);
			DebugAssert(index < count);
			return this->startID + index;
		}
	};

	// Generates a new texture from a pattern.
	Surface generate(TextureUtils::PatternType type, int width, int height, TextureManager &textureManager,
		Renderer &renderer);

	// Generates a tooltip texture with pre-defined font/color/background.
	Surface createTooltip(const std::string &text, const FontLibrary &fontLibrary);

	// Generates individual texture asset references from the given filename. This should be used for filenames
	// that point to a set of textures.
	Buffer<TextureAsset> makeTextureAssets(const std::string &filename, TextureManager &textureManager);

	// Convenience function for allocating a UI texture. The returned handle must be eventually freed.
	bool tryAllocUiTexture(const TextureAsset &textureAsset, const TextureAsset &paletteTextureAsset,
		TextureManager &textureManager, Renderer &renderer, UiTextureID *outID);

	// Convenience function for allocating a UI texture from an SDL surface. Note that the usage of this generally
	// means there is waste with the allocation of the input surface, and this should just be a UI texture allocation
	// and write instead eventually (instead of a copy).
	bool tryAllocUiTextureFromSurface(const Surface &surface, TextureManager &textureManager, Renderer &renderer,
		UiTextureID *outID);
}

using PaletteIdGroup = TextureUtils::IdGroup<PaletteID>;
using TextureBuilderIdGroup = TextureUtils::IdGroup<TextureBuilderID>;

#endif
