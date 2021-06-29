#include "ArenaFontName.h"
#include "FontLibrary.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"
#include "components/utilities/StringView.h"

bool FontLibrary::init()
{
	// Read a hardcoded set of fonts from file.
	for (const char *fontName : ArenaFontName::FontPtrs)
	{
		FontDefinition fontDef;
		if (!fontDef.init(fontName))
		{
			DebugLogWarning("Couldn't init font definition \"" + std::string(fontName) + "\".");
			return false;
		}

		this->defs.emplace_back(std::move(fontDef));
	}

	return true;
}

int FontLibrary::getDefinitionCount() const
{
	return static_cast<int>(this->defs.size());
}

bool FontLibrary::tryGetDefinitionIndex(const char *name, int *outIndex) const
{
	if (String::isNullOrEmpty(name))
	{
		return false;
	}

	const int defCount = static_cast<int>(this->defs.size());
	for (int i = 0; i < defCount; i++)
	{
		const FontDefinition &def = this->defs[i];
		if (StringView::caseInsensitiveEquals(def.getName(), name))
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

const FontDefinition &FontLibrary::getDefinition(int index) const
{
	DebugAssertIndex(this->defs, index);
	return this->defs[index];
}
