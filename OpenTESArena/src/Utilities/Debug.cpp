#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <SDL_messagebox.h>

#include "Debug.h"

#include "String.h"

namespace
{
	const std::map<Debug::MessageType, std::string> DebugMessageTypeNames =
	{
		{ Debug::MessageType::Info, "" },
		{ Debug::MessageType::Warning, "Warning: " },
		{ Debug::MessageType::Error, "Error: " },
	};
}

const std::string Debug::LOG_FILENAME = "log.txt";

std::string Debug::getShorterPath(const char *__file__)
{
	// Replace back-slashes with forward slashes, then split.
	const std::string path = String::replace(std::string(__file__), '\\', '/');
	const std::vector<std::string> tokens = String::split(path, '/');

	std::string shortPath;

	if (tokens.size() >= 2)
	{
		shortPath = tokens.at(tokens.size() - 2) + '/' + tokens.back();
	}
	else if (tokens.size() == 1)
	{
		shortPath = tokens.front();
	}

	return shortPath;
}

void Debug::write(Debug::MessageType type, const std::string &filePath, 
	int lineNumber, const std::string &message)
{
	const std::string &messageType = DebugMessageTypeNames.at(type);
	std::cerr << "[" << filePath << "(" << std::to_string(lineNumber) << ")] " <<
		messageType << message << "\n";
}

void Debug::mention(const char *__file__, int lineNumber, const std::string &message)
{
	Debug::write(Debug::MessageType::Info, Debug::getShorterPath(__file__), 
		lineNumber, message);
}

void Debug::warning(const char *__file__, int lineNumber, const std::string &message)
{
	Debug::write(Debug::MessageType::Warning, Debug::getShorterPath(__file__), 
		lineNumber, message);
}

void Debug::crash(const char *__file__, int lineNumber, const std::string &message)
{
	Debug::write(Debug::MessageType::Error, Debug::getShorterPath(__file__),
		lineNumber, message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), NULL);
	exit(EXIT_FAILURE);
}

void Debug::check(bool condition, const char *__file__, int lineNumber, 
	const std::string &message)
{
	if (!condition)
	{
		Debug::crash(__file__, lineNumber, message);
	}
}
