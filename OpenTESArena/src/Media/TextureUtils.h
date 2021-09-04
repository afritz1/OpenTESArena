#ifndef TEXTURE_UTILS_H
#define TEXTURE_UTILS_H

#include <string>
#include <vector>

#include "Palette.h"
#include "../Rendering/RenderTextureUtils.h"

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

struct TextureAssetReference;

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

	// 32-bit texture creation convenience functions.
	Surface makeSurfaceFrom8Bit(int width, int height, const uint8_t *pixels, const Palette &palette);
	Texture makeTextureFrom8Bit(int width, int height, const uint8_t *pixels, const Palette &palette,
		Renderer &renderer);

	// Generates a new texture from a pattern.
	Surface generate(TextureUtils::PatternType type, int width, int height, TextureManager &textureManager,
		Renderer &renderer);

	// Generates a tooltip texture with pre-defined font/color/background.
	Texture createTooltip(const std::string &text, FontLibrary &fontLibrary, Renderer &renderer);

	// Generates individual texture asset references from the given filename. This should be used for filenames
	// that point to a set of textures.
	Buffer<TextureAssetReference> makeTextureAssetRefs(const std::string &filename, TextureManager &textureManager);

	// Convenience function for allocating a UI texture. The returned handle must be eventually freed.
	bool tryAllocUiTexture(const TextureAssetReference &textureAssetRef, const TextureAssetReference &paletteTextureAssetRef,
		TextureManager &textureManager, Renderer &renderer, UiTextureID *outID);
}

using PaletteIdGroup = TextureUtils::IdGroup<PaletteID>;
using TextureBuilderIdGroup = TextureUtils::IdGroup<TextureBuilderID>;

#endif
