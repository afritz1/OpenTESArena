#include <unordered_map>

#include "PaletteFile.h"
#include "PaletteName.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<PaletteName>
	{
		size_t operator()(const PaletteName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

namespace
{
	const std::unordered_map<PaletteName, std::string> PaletteFilenames =
	{
		{ PaletteName::BuiltIn, "BuiltIn" },
		{ PaletteName::CharSheet, "CHARSHT.COL" },
		{ PaletteName::Default, "PAL.COL" },
		{ PaletteName::Daytime, "DAYTIME.COL" },
		{ PaletteName::Dreary, "DREARY.COL" }
	};
}

const std::string &PaletteFile::fromName(PaletteName paletteName)
{
	const auto &filename = PaletteFilenames.at(paletteName);
	return filename;
}
