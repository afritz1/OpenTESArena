#include <algorithm>
#include <sstream>
#include <vector>

#include "TextAssets.h"

#include "ExeUnpacker.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

TextAssets::TextAssets()
{
	// Decompress A.EXE and place it in a string for later use.
	ExeUnpacker exe("A.EXE");
	this->aExe = exe.getText();

	// Read in TEMPLATE.DAT, using "#..." as keys and the text as values.
	this->parseTemplateDat();
}

TextAssets::~TextAssets()
{

}

void TextAssets::parseTemplateDat()
{
	const std::string filename = "TEMPLATE.DAT";

	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "Text Assets", "Could not open \"" + filename + "\".");

	// Read TEMPLATE.DAT into a string.
	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<char> bytes(fileSize);
	stream->read(bytes.data(), fileSize);

	const std::string text(bytes.data(), fileSize);

	// Step line by line through the text, inserting keys and values into the map.
	std::istringstream iss(text);
	std::string line, key, value;

	while (std::getline(iss, line))
	{
		const char poundSign = '#';
		if (line.at(0) == poundSign)
		{
			// Add the previous key/value pair into the map. There are multiple copies of 
			// some texts in TEMPLATE.DAT, so it's important to skip existing ones.
			if (this->templateDat.find(key) == this->templateDat.end())
			{
				// Clean up the text first so the caller has to do less.
				value = String::replace(value, '\r', '\n');

				while ((value.size() > 0) && (value.at(value.size() - 1) == '\n'))
				{
					value.pop_back();
				}

				// Remove the annoying ampersand at the end of most texts.
				if ((value.size() > 0) && (value.at(value.size() - 1) == '&'))
				{
					value.pop_back();
				}

				this->templateDat.insert(std::make_pair(key, value));
			}

			// Reset the key and value for the next paragraph(s) of text.
			key = String::trim(String::trimLines(line));
			value = "";
		}
		else
		{
			// Add the current line of text onto the value.
			value.append(line);
		}
	}

	// Remove the one empty string added at the start (when key is "").
	this->templateDat.erase("");
}

const std::string &TextAssets::getAExeSegment(const std::pair<int, int> &offsetAndSize)
{
	// Check if the segment has been loaded.
	auto segmentIter = this->aExeSegments.find(offsetAndSize);

	if (segmentIter != this->aExeSegments.end())
	{
		return segmentIter->second;
	}
	else
	{
		// Load the segment and return it.
		std::string segment = this->aExe.substr(offsetAndSize.first, offsetAndSize.second);
		segmentIter = this->aExeSegments.insert(std::make_pair(
			offsetAndSize, std::move(segment))).first;
		return segmentIter->second;
	}
}

const std::string &TextAssets::getTemplateDatText(const std::string &key)
{
	const auto iter = this->templateDat.find(key);
	Debug::check(iter != this->templateDat.end(),
		"Text Assets", "TEMPLATE.DAT key \"" + key + "\" not found.");

	const std::string &value = iter->second;
	return value;
}
