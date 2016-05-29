#ifndef STRING_H
#define STRING_H

#include <vector>

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
};

#endif
