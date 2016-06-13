#ifndef DEBUG_H
#define DEBUG_H

#include <string>

// This is a static class for replacing asserts or program exits that should be 
// accompanied with messages and logging. Plain old asserts like "assert(width > 0)" 
// are for sanity checks and don't need to use these heavier Debug methods which
// are intended for error handling and reporting purposes.

// The line number doesn't need to be given with the debug message like with an
// assert because the message that is printed is disambiguating enough.

class Debug
{
private:
	Debug() = delete;
	Debug(const Debug&) = delete;
	~Debug() = delete;

	static const std::string LOG_FILENAME;
public:
	// Mention something about the program state.
	static void mention(const std::string &className, const std::string &message);

	// If the condition is false, crash the program with a message.
	static void check(bool condition, const std::string &message);

	// If the condition is false, crash the program with a class name and message.
	static void check(bool condition, const std::string &className, 
		const std::string &message);

	// Crash the program with a message.
	static void crash(const std::string &message);

	// Crash the program with a class name and message.
	static void crash(const std::string &className, const std::string &message);
};

#endif
