#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>

#include "SDL_messagebox.h"

#include "Debug.h"
#include "../utilities/Directory.h"
#include "../utilities/String.h"

// Always show error dialog when crashing; this helps on Windows in exclusive fullscreen.
#if true //defined(__APPLE__) && defined(__MACH__)
#define SHOW_ERROR_MESSAGE_BOX 1
#endif

namespace
{
	std::tm GetCalendarDateTime()
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
}

namespace Log
{
	constexpr int MAX_FILES = 10;

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

	const int existingLogFileCount = Directory::getFileCount(logDirectory);
	if (existingLogFileCount >= Log::MAX_FILES)
	{
		Directory::deleteOldestFile(logDirectory);
	}

	const std::tm tm = GetCalendarDateTime();
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
	if (Log::stream.is_open())
	{
		Log::stream.close();
	}
	
	std::fill(std::begin(Log::pathBuffer), std::end(Log::pathBuffer), '\0');
}

void Debug::exitApplication()
{
#ifndef SHOW_ERROR_MESSAGE_BOX
	std::getchar();
#endif

	Debug::shutdown();
	exit(EXIT_FAILURE);
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

std::string Debug::makeOutputString(const char *__file__, int lineNumber, const char *messagePrefix, const std::string &message)
{
	const std::string shorterPath = Debug::getShorterPath(__file__);
	const std::string outputString = Debug::stringFormat("[%s(%d)] %s%s\n", shorterPath.c_str(), lineNumber, messagePrefix, message.c_str());
	return outputString;
}

void Debug::write(const char *message)
{
	std::cerr << message;
	Log::stream << message;
}

void Debug::showErrorMessageBox(const char *message)
{
#ifdef SHOW_ERROR_MESSAGE_BOX
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenTESArena Error", message, nullptr);
#endif
}
