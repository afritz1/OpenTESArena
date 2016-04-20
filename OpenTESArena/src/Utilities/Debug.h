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
	// Crash the program with a message if the condition is false.
	static void check(bool condition, const std::string &message);

	// For use in a class context with an optional argument for crashing if the condition
	// fails.
	static void check(bool condition, const std::string &className,
		const std::string &message, bool crashOnFailure);

	// For use in a class context, the class name is what class the executing method is in.
	static void check(bool condition, const std::string &className, 
		const std::string &message);
};

#endif
