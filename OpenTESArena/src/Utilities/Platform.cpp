#include "SDL.h"

#include "Platform.h"

#include "Debug.h"
#include "String.h"

std::string Platform::getPlatform()
{
	return std::string(SDL_GetPlatform());
}

std::string Platform::getBasePath()
{
	// Allocate the base path from SDL.
	char *basePathPtr = SDL_GetBasePath();

	if (basePathPtr == nullptr)
	{
		DebugMention("SDL_GetBasePath() not available on this platform.");
		basePathPtr = SDL_strdup("./");
	}

	const std::string basePathString(basePathPtr);
	SDL_free(basePathPtr);

	// Convert Windows backslashes to forward slashes.
	return String::replace(basePathString, '\\', '/');
}

std::string Platform::getOptionsPath()
{
	// SDL_GetPrefPath() creates the desired folder if it doesn't exist.
	char *optionsPathPtr = SDL_GetPrefPath("OpenTESArena", "options");

	if (optionsPathPtr == nullptr)
	{
		DebugMention("SDL_GetPrefPath() not available on this platform.");
		optionsPathPtr = SDL_strdup("options/");
	}

	const std::string optionsPathString(optionsPathPtr);
	SDL_free(optionsPathPtr);

	// Convert Windows backslashes to forward slashes.
	return String::replace(optionsPathString, '\\', '/');
}

std::string Platform::getLogPath()
{
	// Unfortunately there's no SDL_GetLogPath(), so we need to make our own.
	const std::string platform = Platform::getPlatform();

	if (platform == "Windows")
	{
		char *logPathPtr = SDL_GetPrefPath("OpenTESArena", "log");

		if (logPathPtr == nullptr)
		{
			DebugMention("SDL_GetPrefPath() not available on this platform.");
			logPathPtr = SDL_strdup("log/");
		}

		const std::string logPathString(logPathPtr);
		SDL_free(logPathPtr);

		// Convert Windows backslashes to forward slashes.
		return String::replace(logPathString, '\\', '/');
	}
	else if (platform == "Linux")
	{
		return "/var/logs/";
	}
	else if (platform == "Mac OS X")
	{
		return "~/Library/Logs/";
	}
	else
	{
		DebugMention("No default log path on this platform.");
		return "log/";
	}
}
