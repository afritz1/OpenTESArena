#ifndef UTILITY_H
#define UTILITY_H

#include <vector>

class Utility
{
private:
	Utility() = delete;
	Utility(const Utility&) = delete;
	~Utility() = delete;
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
