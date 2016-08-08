#include <map>

#include "PaletteFile.h"

#include "PaletteName.h"

namespace
{
	const std::map<PaletteName, std::string> PaletteFilenames
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
