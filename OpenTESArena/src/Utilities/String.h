#ifndef STRING_H
#define STRING_H

#include <vector>
#include <sstream>
#include <string>

// This static class offers various string operations and conversions.

class String
{
private:
	String() = delete;
	String(const String&) = delete;
	~String() = delete;
public:
	// Splits a string on the given character.
	static std::vector<std::string> split(const std::string &line, char separator);

	// Splits a string on whitespace.
	static std::vector<std::string> split(const std::string &line);

	// Removes whitespace from a string.
	static std::string trim(const std::string &line);

	// Removes new line characters from a string.
	static std::string trimLines(const std::string &line);

	// Gets the right-most extension from a string, i.e., ".txt".
	static std::string getExtension(const std::string &str);

	// Creates a new string with all 'a' characters replaced by 'b' characters.
	static std::string replace(const std::string &str, char a, char b);

	// Turns an integral value into a hex string.
	template <typename T>
	static std::string toHexString(T val)
	{
		static_assert(std::is_integral<T>::value, "String::toHexString given non-integral type.");
		std::stringstream sstr;
		sstr << std::hex << val;
		return sstr.str();
	}
};

#endif
