#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <cstdint>
#include <string_view>

#include "Buffer.h"
#include "String.h"

namespace StringView
{
	// Performs a typical ASCII string comparison (mostly intended for const char* convenience).
	bool equals(std::string_view a, std::string_view b);

	// Performs a case-insensitive ASCII string comparison.
	bool caseInsensitiveEquals(std::string_view a, std::string_view b);

	// Returns a substring of a string view. Intended for use with strings, since
	// std::string::substr() returns a new string which is bad for string_view.
	std::string_view substr(std::string_view str, size_t offset, size_t count);

	// Splits a string view on the given character.
	Buffer<std::string_view> split(std::string_view str, char separator);

	// Splits a string view on whitespace.
	Buffer<std::string_view> split(std::string_view str);

	// Splits a string view on the given character without allocating the destination array.
	// Breaks early if too many splits are encountered. Returns whether the split count matches
	// the destination size.
	template<int T>
	bool splitExpected(std::string_view str, char separator, BufferView<std::string_view> dst)
	{
		static_assert(T > 0);

		if (dst.getCount() != T)
		{
			return false;
		}

		// Bootstrap the loop.
		dst[0] = std::string_view(str.data(), 0);

		int dstIndex = 0;
		for (int i = 0; i < static_cast<int>(str.size()); i++)
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
	template<int T>
	bool splitExpected(std::string_view str, BufferView<std::string_view> dst)
	{
		return StringView::splitExpected<T>(str, String::SPACE, dst);
	}

	// Removes leading whitespace from a string view.
	std::string_view trimFront(std::string_view str);

	// Removes trailing whitespace from a string view.
	std::string_view trimBack(std::string_view str);

	// Gets the right-most extension from a string view, i.e., "txt".
	std::string_view getExtension(std::string_view str);
}

#endif
