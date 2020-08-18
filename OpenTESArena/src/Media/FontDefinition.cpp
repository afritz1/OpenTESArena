#include <algorithm>
#include <cstring>

#include "FontDefinition.h"

#include "../Assets/FontFile.h"
#include "components/debug/Debug.h"
#include "components/utilities/String.h"

FontDefinition::FontDefinition()
{
	this->characterHeight = -1;
}

bool FontDefinition::init(const char *filename)
{
	FontFile fontFile;
	if (!fontFile.init(filename))
	{
		DebugLogWarning("Could not init font file \"" + std::string(filename) + "\".");
		return false;
	}

	this->characters.init(fontFile.getCharacterCount());
	this->characterHeight = fontFile.getHeight();
	this->name = std::string(filename);

	for (int i = 0; i < this->characters.getCount(); i++)
	{
		const int characterWidth = fontFile.getWidth(i);
		const FontFile::Pixel *srcPixels = fontFile.getPixels(i);
		const int pixelCount = characterWidth * characterHeight;

		Buffer2D<Pixel> &characterPixels = this->characters.get(i);
		characterPixels.init(characterWidth, characterHeight);
		std::copy(srcPixels, srcPixels + pixelCount, characterPixels.get());

		char c;
		if (!FontFile::tryGetChar(i, &c))
		{
			DebugLogWarning("Couldn't get ASCII character for index \"" +
				std::to_string(i) + "\".");
			continue;
		}

		const std::string charUtf8(1, c);
		std::string lookupStr;
		if (!FontDefinition::tryMakeCharLookupString(charUtf8.c_str(), &lookupStr))
		{
			DebugLogWarning("Couldn't make character look-up string for \"" + charUtf8 + "\".");
			continue;
		}

		const CharID charID = static_cast<CharID>(i);
		this->charIDs.emplace(std::make_pair(std::move(lookupStr), charID));
	}

	return true;
}

const std::string &FontDefinition::getName() const
{
	return this->name;
}

int FontDefinition::getCharacterHeight() const
{
	return this->characterHeight;
}

bool FontDefinition::tryMakeCharLookupString(const char *c, std::string *outString)
{
	if (String::isNullOrEmpty(c))
	{
		DebugLogWarning("Can't make look-up string from null/empty UTF-8 character.");
		return false;
	}

	// For now, only support ASCII.
	if (std::strlen(c) != 1)
	{
		DebugLogWarning("Non-ASCII character encodings not supported yet.");
		return false;
	}

	*outString = std::string(c);
	return true;
}

bool FontDefinition::tryGetCharacterID(const char *c, CharID *outID) const
{
	std::string lookupStr;
	if (!FontDefinition::tryMakeCharLookupString(c, &lookupStr))
	{
		DebugLogWarning("Couldn't make character look-up string for \"" + std::string(c) + "\".");
		return false;
	}

	const auto iter = this->charIDs.find(lookupStr);
	if (iter != this->charIDs.end())
	{
		*outID = iter->second;
		return true;
	}
	else
	{
		return false;
	}
}

const FontDefinition::Character &FontDefinition::getCharacter(CharID id) const
{
	DebugAssert(id >= 0);
	DebugAssert(id < this->characters.getCount());
	return this->characters.get(id);
}
