#ifndef FONT_DEFINITION_H
#define FONT_DEFINITION_H

#include <string>
#include <unordered_map>

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"

class FontDefinition
{
public:
	// Mapping of UTF-8 character to unique ID.
	using CharID = int;

	// If a pixel is set, it contributes to the character's appearance.
	// @todo: if alpha-blending is desired then change bool to float.
	using Pixel = bool;
	using Character = Buffer2D<Pixel>;
private:
	Buffer<Character> characters;
	std::unordered_map<std::string, CharID> charIDs;
	std::string name;
	int characterHeight;

	static bool tryMakeCharLookupString(const char *c, std::string *outString);
public:
	FontDefinition();

	bool init(const char *filename);

	// Gets the uniquely-identifying name of this font.
	const std::string &getName() const;
	
	// Gets the height in pixels for all characters in the font.
	// This can be used to determine the height of a row of text.
	int getCharacterHeight() const;

	// Attempts to get the character ID associated with the given UTF-8 character.
	bool tryGetCharacterID(const char *c, CharID *outID) const;

	const Character &getCharacter(CharID id) const;
};

#endif
