#ifndef TEXT_ENTRY_H
#define TEXT_ENTRY_H

#include <cstddef>
#include <string>

union SDL_Event;

namespace TextEntry
{
	// Modifies the text parameter based on the given SDL input event and backspace boolean,
	// and returns whether the text was changed. The "charIsAllowed" function decides
	// which characters from the SDL input event can be appended to the string.
	bool updateText(std::string &text, const SDL_Event &e, bool backspace,
		bool(*charIsAllowed)(char), size_t maxLength);
}

#endif
