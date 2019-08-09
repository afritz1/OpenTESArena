#ifndef STRING_H
#define STRING_H

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// This static class offers various string operations and conversions.

class String
{
private:
	String() = delete;
	~String() = delete;
public:
	// Performs a case-insensitive ASCII string comparison.
	static bool caseInsensitiveEquals(const std::string &a, const std::string &b);

	// Splits a string on the given character.
	static std::vector<std::string> split(const std::string &str, char separator);

	// Splits a string on whitespace.
	static std::vector<std::string> split(const std::string &str);

	// Splits a string on the given character without allocating the destination array. Breaks
	// early if too many splits are encountered. Returns whether the split count matches the
	// destination size.
	template <size_t T>
	static bool splitExpected(const std::string &str, char separator,
		std::array<std::string, T> &dst)
	{
		static_assert(T > 0);

		size_t dstIndex = 0;
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
	template <size_t T>
	static bool splitExpected(const std::string &str, std::array<std::string, T> &dst)
	{
		return String::splitExpected(str, ' ', dst);
	}

	// Removes all whitespace from a string.
	static std::string trim(const std::string &str);

	// Removes leading whitespace from a string.
	static std::string trimFront(const std::string &str);
	static void trimFrontInPlace(std::string &str);

	// Removes trailing whitespace from a string.
	static std::string trimBack(const std::string &str);
	static void trimBackInPlace(std::string &str);

	// Removes new line characters from a string.
	static std::string trimLines(const std::string &str);

	// Removes extra whitespace from a string.
	static std::string trimExtra(const std::string &str);

	// Replaces spaces with newlines relative to the given character limit per line.
	static std::string distributeNewlines(const std::string &str, int charLimit);

	// Gets the right-most extension from a string, i.e., ".txt".
	static std::string getExtension(const std::string &str);

	// Adds a forward slash at the end if there is not one. Intended for paths.
	static std::string addTrailingSlashIfMissing(const std::string &str);

	// Creates a new string with all 'a' characters replaced by 'b' characters.
	static std::string replace(const std::string &str, char a, char b);

	// Creates a new string with all "a" substrings replaced by "b" strings.
	static std::string replace(const std::string &str, const std::string &a,
		const std::string &b);

	// Converts each ASCII character in the given string to uppercase.
	static std::string toUppercase(const std::string &str);

	// Converts each ASCII character in the given string to lowercase.
	static std::string toLowercase(const std::string &str);

	// Converts an integral value to a hex string.
	template <typename T>
	static std::string toHexString(T value)
	{
		static_assert(std::is_integral<T>::value);

		std::stringstream ss;
		ss << std::hex << value;
		return ss.str();
	}

	// Converts a floating-point value to a string with a set number of decimal places.
	template <typename T>
	static std::string fixedPrecision(T value, int precision)
	{
		static_assert(std::is_floating_point<T>::value);

		std::stringstream ss;
		ss << std::fixed << std::setprecision(precision) << value;
		return ss.str();
	}
};

#endif
