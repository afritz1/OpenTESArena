#include "SDL.h"

#include "TextEntry.h"

bool TextEntry::updateText(std::string &text, const SDL_Event &e, bool backspace,
	bool(*charIsAllowed)(char), size_t maxLength)
{
	if (backspace)
	{
		// Erase one letter if able.
		if (text.size() > 0)
		{
			text.pop_back();
			return true;
		}
	}

	const bool charReceived = e.type == SDL_TEXTINPUT;
	const bool textIsShortEnough = text.size() < maxLength;

	// Only process the input if a character was received and the string has
	// space remaining.
	if (charReceived && textIsShortEnough)
	{
		const char c = e.text.text[0];

		if (charIsAllowed(c))
		{
			// Append to the string.
			text.push_back(c);
			return true;
		}
	}

	// No change in the displayed text.
	return false;
}
