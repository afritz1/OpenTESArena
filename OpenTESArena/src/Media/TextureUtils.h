#ifndef TEXTURE_UTILS_H
#define TEXTURE_UTILS_H

#include "components/debug/Debug.h"

// Various texture handles for use with texture manager.
using PaletteID = int; // 256 color 32-bit software surface
using ImageID = int; // 8-bit software surface
using SurfaceID = int; // 32-bit software surface
using TextureID = int; // 32-bit hardware surface

// Texture instance handles, same as texture manager but for generated textures not loaded
// from a file.
using ImageInstanceID = int; // 8-bit software surface
using SurfaceInstanceID = int; // 32-bit software surface
using TextureInstanceID = int; // 32-bit hardware surface

namespace TextureUtils
{
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

	using PaletteIdGroup = IdGroup<PaletteID>;
	using ImageIdGroup = IdGroup<ImageID>;
	using SurfaceIdGroup = IdGroup<SurfaceID>;
	using TextureIdGroup = IdGroup<TextureID>;
}

#endif
