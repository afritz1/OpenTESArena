#include <cstdint>
#include <fstream>
#include <iostream>

#include "Debug.h"
#include "../utilities/String.h"

namespace
{
	const std::pair<DebugMessageType, std::string> DebugMessageTypeNames[] =
	{
		{ DebugMessageType::Status, "" },
		{ DebugMessageType::Warning, "Warning: " },
		{ DebugMessageType::Error, "Error: " }
	};

	const std::string &GetDebugMessageTypeString(DebugMessageType messageType)
	{
		size_t index = 0;
		for (size_t i = 0; i < std::size(DebugMessageTypeNames); i++)
		{
			const auto &pair = DebugMessageTypeNames[i];
			if (pair.first == messageType)
			{
				index = i;
				break;
			}
		}

		return DebugMessageTypeNames[index].second;
	}
}

const std::string Debug::LOG_FILENAME = "log.txt";

std::string Debug::getShorterPath(const char *__file__)
{
	// Replace back-slashes with forward slashes, then split.
	const std::string path = String::replace(std::string(__file__), '\\', '/');
	const Buffer<std::string> tokens = String::split(path, '/');

	std::string shortPath;

	if (tokens.getCount() >= 2)
	{
		shortPath = tokens[tokens.getCount() - 2] + '/' + tokens[tokens.getCount() - 1];
	}
	else if (tokens.getCount() == 1)
	{
		shortPath = tokens[0];
	}

	return shortPath;
}

void Debug::write(DebugMessageType type, const std::string &filePath, int lineNumber, const std::string &message)
{
	const std::string &messageTypeStr = GetDebugMessageTypeString(type);
	std::cerr << "[" << filePath << "(" << std::to_string(lineNumber) << ")] " << messageTypeStr << message << "\n";
}

void Debug::log(const char *__file__, int lineNumber, const std::string &message)
{
	Debug::write(DebugMessageType::Status, Debug::getShorterPath(__file__), lineNumber, message);
}

void Debug::logWarning(const char *__file__, int lineNumber, const std::string &message)
{
	Debug::write(DebugMessageType::Warning, Debug::getShorterPath(__file__), lineNumber, message);
}

void Debug::logError(const char *__file__, int lineNumber, const std::string &message)
{
	Debug::write(DebugMessageType::Error, Debug::getShorterPath(__file__), lineNumber, message);
}

void Debug::crash(const char *__file__, int lineNumber, const std::string &message)
{
	Debug::logError(__file__, lineNumber, message);

#if defined(__APPLE__) && defined(__MACH__)
	// @todo: implement proper logging alternative to SDL message box.
	// macOS .apps close immediately even with getchar(), so a message box is needed.
	//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message.c_str(), nullptr);
#else
	std::getchar();
#endif

	exit(EXIT_FAILURE);
}
