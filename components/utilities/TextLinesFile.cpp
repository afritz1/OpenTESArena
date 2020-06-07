#include <cctype>
#include <sstream>

#include "File.h"
#include "String.h"
#include "TextLinesFile.h"
#include "../debug/Debug.h"

bool TextLinesFile::init(const char *filename)
{
	if (!File::exists(filename))
	{
		DebugLogError("Could not find \"" + std::string(filename) + "\".");
		return false;
	}

	std::stringstream ss(File::readAllText(filename));
	std::string line;

	this->lines.clear();
	while (std::getline(ss, line))
	{
		String::trimFrontInPlace(line);
		String::trimBackInPlace(line);

		if (line.size() > 0)
		{
			const bool isComment = line[0] == TextLinesFile::COMMENT;
			if (!isComment)
			{
				this->lines.emplace_back(std::move(line));
			}
		}
	}

	return true;
}

int TextLinesFile::getLineCount() const
{
	return static_cast<int>(this->lines.size());
}

const std::string &TextLinesFile::getLine(int index) const
{
	DebugAssertIndex(this->lines, index);
	return this->lines[index];
}
