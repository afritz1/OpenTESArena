#include <fstream>
#include <iostream>

#include "Debug.h"

const std::string Debug::LOG_FILENAME = "log.txt";

void Debug::mention(const std::string &className, const std::string &message)
{
	std::cout << className << ": " << message << "\n";
}

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
	const std::string &message)
{
	if (!condition)
	{
		std::cerr << className << " error: " << message << "\n";
		std::getchar();
		exit(EXIT_FAILURE);
	}
}

void Debug::crash(const std::string &message)
{
	Debug::check(false, message);
}

void Debug::crash(const std::string &className, const std::string &message)
{
	Debug::check(false, className, message);
}
