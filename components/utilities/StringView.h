#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "String.h"

namespace StringView
{
	// Performs a case-insensitive ASCII string comparison.
	bool caseInsensitiveEquals(const std::string_view &a, const std::string_view &b);

	// Returns a substring of a string view. Intended for use with strings, since
	// std::string::substr() returns a new string which is bad for string_view.
	std::string_view substr(const std::string_view &str, size_t offset, size_t count);

	// Splits a string view on the given character.
	std::vector<std::string_view> split(const std::string_view &str, char separator);

	// Splits a string view on whitespace.
	std::vector<std::string_view> split(const std::string_view &str);

	// Splits a string view on the given character without allocating the destination array.
	// Breaks early if too many splits are encountered. Returns whether the split count matches
	// the destination size.
	template <size_t T>
	bool splitExpected(const std::string_view &str, char separator,
		std::array<std::string_view, T> &dst)
	{
		static_assert(T > 0);

		// Bootstrap the loop.
		dst[0] = std::string_view(str.data(), 0);

		size_t dstIndex = 0;
		for (size_t i = 0; i < str.size(); i++)
		{
			const char c = str[i];

			if (c == separator)
			{
				// Start a new string.
				dstIndex++;
				if (dstIndex == T)
				{
					return false;
				}

				dst[dstIndex] = std::string_view(str.data() + i + 1, 0);
			}
			else
			{
				// Put the character on the end of the current string.
				std::string_view old = dst[dstIndex];
				dst[dstIndex] = std::string_view(old.data(), old.size() + 1);
			}
		}

		return dstIndex == (T - 1);
	}

	// Splits a string view on whitespace without allocating the destination array. Breaks early
	// if too many splits are encountered. Returns whether the split count matches the destination
	// size.
	template <size_t T>
	bool splitExpected(const std::string_view &str, std::array<std::string_view, T> &dst)
	{
		return StringView::splitExpected(str, String::SPACE, dst);
	}

	// Removes leading whitespace from a string view.
	std::string_view trimFront(const std::string_view &str);

	// Removes trailing whitespace from a string view.
	std::string_view trimBack(const std::string_view &str);

	// Gets the right-most extension from a string view, i.e., "txt".
	std::string_view getExtension(const std::string_view &str);
}

#endif
