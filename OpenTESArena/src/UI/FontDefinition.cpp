#include <algorithm>
#include <cstring>

#include "FontDefinition.h"

#include "../Assets/FontFile.h"
#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	bool TryMakeCharLookupString(const char *charUtf8, std::string *outString)
	{
		if (String::isNullOrEmpty(charUtf8))
		{
			DebugLogWarning("Can't make look-up string from null/empty UTF-8 character.");
			return false;
		}

		// For now, only support ASCII.
		if (std::strlen(charUtf8) != 1)
		{
			DebugLogWarning("Non-ASCII character encodings not supported yet.");
			return false;
		}

		*outString = std::string(charUtf8);
		return true;
	}
}

FontDefinition::FontDefinition()
{
	this->characterHeight = -1;
}

bool FontDefinition::init(const char *filename)
{
	FontFile fontFile;
	if (!fontFile.init(filename))
	{
		DebugLogWarningFormat("Could not init font file \"%s\".", filename);
		return false;
	}

	this->characters.init(fontFile.getCharacterCount());
	this->characterHeight = fontFile.getHeight();
	this->name = std::string(filename);

	for (int i = 0; i < this->characters.getCount(); i++)
	{
		Span2D<const FontFilePixel> srcPixels = fontFile.getPixels(i);
		Buffer2D<FontDefinitionPixel> &characterPixels = this->characters.get(i);
		characterPixels.init(srcPixels.getWidth(), this->characterHeight);
		std::copy(srcPixels.begin(), srcPixels.end(), characterPixels.begin());

		char c;
		if (!FontFile::tryGetChar(i, &c))
		{
			DebugLogWarningFormat("Couldn't get ASCII character at index %d.", i);
			continue;
		}

		const std::string charUtf8(1, c);
		std::string lookupStr;
		if (!TryMakeCharLookupString(charUtf8.c_str(), &lookupStr))
		{
			DebugLogWarningFormat("Couldn't make character look-up string for \"%s\".", charUtf8.c_str());
			continue;
		}

		const FontDefinitionCharacterID charID = static_cast<FontDefinitionCharacterID>(i);
		this->charIDs.emplace(std::move(lookupStr), charID);
	}

	return true;
}

const FontDefinitionCharacter &FontDefinition::getCharacter(FontDefinitionCharacterID id) const
{
	DebugAssert(id >= 0);
	DebugAssert(id < this->characters.getCount());
	return this->characters.get(id);
}

bool FontDefinition::tryGetCharacterID(const char *charUtf8, FontDefinitionCharacterID *outID) const
{
	std::string lookupStr;
	if (!TryMakeCharLookupString(charUtf8, &lookupStr))
	{
		DebugLogWarningFormat("Couldn't make character look-up string for \"%s\".", charUtf8);
		return false;
	}

	const auto iter = this->charIDs.find(lookupStr);
	if (iter == this->charIDs.end())
	{
		return false;
	}

	*outID = iter->second;
	return true;
}
