#ifndef TEXT_ENTRY_H
#define TEXT_ENTRY_H

#include <cstddef>
#include <string>
#include <string_view>

union SDL_Event;

namespace TextEntry
{
	// Modifies the text parameter based on the given SDL input event and backspace boolean,
	// and returns whether the text was changed. The "charIsAllowed" function decides
	// which characters from the SDL input event can be appended to the string.
	// @todo: deprecated
	bool updateText(std::string &text, const SDL_Event &e, bool backspace,
		bool(*charIsAllowed)(char), size_t maxLength);

	// Attempts to append the given input text. Returns whether the text changed.
	bool append(std::string &text, const std::string_view &inputText, bool(*isCharAllowed)(char),
		size_t maxLength = std::string::npos);

	// Attempts to delete the backmost character. Returns whether the text changed.
	bool backspace(std::string &text);
}

#endif
