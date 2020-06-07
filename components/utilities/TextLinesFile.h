#ifndef TEXT_LINES_FILE_H
#define TEXT_LINES_FILE_H

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

#endif
