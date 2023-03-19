#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>

#include "Debug.h"
#include "../utilities/Directory.h"
#include "../utilities/String.h"

namespace
{
	std::tm GetTm()
	{
		const auto clock = std::chrono::system_clock::now();
		const std::time_t clockAsTime = std::chrono::system_clock::to_time_t(clock);

		std::tm tm;
#if defined(_WIN32)
		gmtime_s(&tm, &clockAsTime);
#else
		const std::tm *tmPtr = gmtime(&clockAsTime);
		tm = *tmPtr;
#endif
		return tm;
	}

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

namespace Log
{
	char pathBuffer[1024];
	std::ofstream stream;
}

bool Debug::init(const char *logDirectory)
{
	if (String::isNullOrEmpty(logDirectory))
	{
		std::cerr << "Can't init debug logging with empty directory.\n";
		return false;
	}

	if (!Directory::exists(logDirectory))
	{
		Directory::createRecursively(logDirectory);
	}

	const std::tm tm = GetTm();
	char timeStrBuffer[256];
	std::strftime(timeStrBuffer, std::size(timeStrBuffer), "%H`%M`%S %z %m-%d-%Y", &tm);
	std::snprintf(Log::pathBuffer, std::size(Log::pathBuffer), "%slog %s.txt", logDirectory, timeStrBuffer);

	Log::stream.open(Log::pathBuffer, std::ofstream::trunc);
	if (!Log::stream.is_open())
	{
		std::cerr << "Couldn't open log file stream for path \"" + std::string(Log::pathBuffer) + "\".\n";
		return false;
	}

	return true;
}

void Debug::shutdown()
{
	Log::stream.close();
	std::fill(std::begin(Log::pathBuffer), std::end(Log::pathBuffer), '\0');
}

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
	const std::string lineNumberStr = std::to_string(lineNumber);
	const std::string outputStr = "[" + filePath + "(" + lineNumberStr + ")] " + messageTypeStr + message + '\n';
	std::cerr << outputStr;
	Log::stream << outputStr;
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

	Debug::shutdown();
	exit(EXIT_FAILURE);
}
