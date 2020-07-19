#include "PaletteFile.h"
#include "PaletteName.h"
#include "PaletteUtils.h"

bool PaletteUtils::isBuiltIn(const std::string &filename)
{
	const std::string &builtInName = PaletteFile::fromName(PaletteName::BuiltIn);
	return filename == builtInName;
}
