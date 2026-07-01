#pragma once

#include <string>
#include <unordered_map>

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"

using FontDefinitionPixel = bool;
using FontDefinitionCharacter = Buffer2D<FontDefinitionPixel>;

// Points to a presentable character in the font. -1 means not presentable (like a tab).
using FontDefinitionCharacterID = int;

struct FontDefinition
{
	Buffer<FontDefinitionCharacter> characters;
	std::unordered_map<std::string, FontDefinitionCharacterID> charIDs;
	std::string name;

	// The height in pixels for all characters in the font. Determines the height of a row of text.
	int characterHeight;

	FontDefinition();

	bool init(const char *filename);

	const FontDefinitionCharacter &getCharacter(FontDefinitionCharacterID id) const;
	bool tryGetCharacterID(const char *charUtf8, FontDefinitionCharacterID *outID) const;
};
