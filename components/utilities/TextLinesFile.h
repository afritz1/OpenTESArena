#pragma once

#include <string>
#include <vector>

class TextLinesFile
{
private:
	static constexpr char COMMENT = '#';

	std::vector<std::string> lines; // Only non-comment lines.
public:
	bool init(const char *filename);

	// Includes only non-comment lines.
	int getLineCount() const;

	const std::string &getLine(int index) const;
};
