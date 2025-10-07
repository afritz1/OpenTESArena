#ifndef STRING_H
#define STRING_H

#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

#include "Buffer.h"
#include "Span.h"

// Various string operations and conversions.
namespace String
{
	static constexpr char SPACE = ' ';
	static constexpr char CARRIAGE_RETURN = '\r';
	static constexpr char NEWLINE = '\n';
	static constexpr char TAB = '\t';
	static constexpr char FILE_EXTENSION_SEPARATOR = '.';

	// Returns whether the given C string is null or the empty string.
	bool isNullOrEmpty(const char *str);

	// Performs a case-insensitive ASCII string comparison.
	bool caseInsensitiveEquals(const std::string &a, const std::string &b);

	int compare(const std::string &a, const std::string &b);

	template<typename... Args>
	static std::string format(const char *formatStr, Args... args)
	{
		const int charCount = std::snprintf(nullptr, 0, formatStr, args...);
		std::string buffer(charCount + 1, '\0');
		std::snprintf(buffer.data(), buffer.size(), formatStr, args...);
		return buffer;
	}

	// Splits a string on the given character.
	Buffer<std::string> split(const std::string &str, char separator);

	// Splits a string on whitespace.
	Buffer<std::string> split(const std::string &str);

	// Splits a string on the given character without allocating the destination array. Breaks
	// early if too many splits are encountered. Returns whether the split count matches the
	// destination size.
	template<int T>
	bool splitExpected(const std::string &str, char separator, Span<std::string> dst)
	{
		static_assert(T > 0);

		if (dst.getCount() != T)
		{
			return false;
		}

		int dstIndex = 0;
		for (const char c : str)
		{
			if (c == separator)
			{
				// Move to the next destination string.
				dstIndex++;
				if (dstIndex == T)
				{
					return false;
				}
			}
			else
			{
				// Put the character on the end of the current string.
				dst[dstIndex].push_back(c);
			}
		}

		return dstIndex == (T - 1);
	}

	// Splits a string on whitespace without allocating the destination array. Breaks early if
	// too many splits are encountered. Returns whether the split count matches the destination
	// size.
	template<int T>
	bool splitExpected(const std::string &str, Span<std::string> dst)
	{
		return String::splitExpected<T>(str, String::SPACE, dst);
	}

	// Removes all whitespace from a string.
	std::string trim(const std::string &str);

	// Removes leading whitespace from a string.
	std::string trimFront(const std::string &str);
	void trimFrontInPlace(std::string &str);

	// Removes trailing whitespace from a string.
	std::string trimBack(const std::string &str);
	void trimBackInPlace(std::string &str);

	// Removes new line characters from a string.
	std::string trimLines(const std::string &str);

	// Removes extra whitespace from a string.
	std::string trimExtra(const std::string &str);

	// Replaces spaces with newlines relative to the given character limit per line.
	std::string distributeNewlines(const std::string &str, int charLimit);

	// Gets the right-most extension from a string, i.e., ".txt".
	std::string getExtension(const std::string &str);

	// Adds a forward slash at the end if there is not one. Intended for paths.
	std::string addTrailingSlashIfMissing(const std::string &str);

	// Creates a new string with all 'a' characters replaced by 'b' characters.
	std::string replace(const std::string &str, char a, char b);

	// Creates a new string with all "a" substrings replaced by "b" strings.
	std::string replace(const std::string &str, const std::string &a, const std::string &b);

	// Converts each ASCII character in the given string to uppercase.
	std::string toUppercase(const std::string &str);

	// Converts each ASCII character in the given string to lowercase.
	std::string toLowercase(const std::string &str);

	// Converts an integral value to a hex string.
	template <typename T>
	std::string toHexString(T value)
	{
		static_assert(std::is_integral_v<T>);

		std::stringstream ss;
		ss << std::uppercase << std::hex << value;
		return ss.str();
	}

	// Converts a floating-point value to a string with a set number of decimal places.
	template <typename T>
	std::string fixedPrecision(T value, int precision)
	{
		static_assert(std::is_floating_point_v<T>);

		std::stringstream ss;
		ss << std::fixed << std::setprecision(precision) << value;
		return ss.str();
	}

	// Attempts to copy the source string to the destination buffer. Returns whether
	// the entire source string was copied.
	bool tryCopy(const char *src, char *dst, size_t dstSize);
}

#endif
