#include <fstream>
#include <iostream>

#include "Debug.h"

const std::string Debug::LOG_FILENAME = "log.txt";

void Debug::check(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << "Error: " << message << "\n";
		std::getchar();
		exit(EXIT_FAILURE);
	}
}

void Debug::check(bool condition, const std::string &className,
	const std::string &message, bool crashOnFailure)
{
	if (!condition)
	{
		std::cerr << className << " error: " << message << "\n";
		if (crashOnFailure)
		{
			std::getchar();
			exit(EXIT_FAILURE);
		}
	}
}

void Debug::check(bool condition, const std::string &className,
	const std::string &message)
{
	Debug::check(condition, className, message, true);
}
