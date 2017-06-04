#ifndef DEBUG_H
#define DEBUG_H

#include <string>

// Below are various debug methods and macros for replacing asserts or program exits 
// that should be accompanied with messages and logging. Plain old asserts like 
// "assert(width > 0)" are for sanity checks and don't need to use these heavier methods.

class Debug
{
public:
	enum class MessageType
	{
		Info,
		Warning,
		Error
	};
private:
	static const std::string LOG_FILENAME;

	Debug() = delete;
	~Debug() = delete;

	// Shortens the __FILE__ macro so it only includes a couple parent folders.
	static std::string getShorterPath(const char *__file__);

	// Writes a debug message to the console with the file path and line number.
	static void write(Debug::MessageType type, const std::string &filePath,
		int lineNumber, const std::string &message);
public:
	// Use DebugMention() instead. Helper method for mentioning something about program state.
	static void mention(const char *__file__, int lineNumber, const std::string &message);

	// Use DebugWarning() instead. Helper method for warning the user about something.
	static void warning(const char *__file__, int lineNumber, const std::string &message);

	// Use DebugCrash() instead. Helper method for crashing the program with a reason.
	static void crash(const char *__file__, int lineNumber, const std::string &message);

	// Use DebugAssert() instead. Helper method for verifying that a condition is true, and
	// crashing the program if it is false.
	static void check(bool condition, const char *__file__, int lineNumber, 
		const std::string &message);

#define DebugMention(message) Debug::mention(__FILE__, __LINE__, std::string(message))
#define DebugWarning(message) Debug::warning(__FILE__, __LINE__, std::string(message))
#define DebugCrash(message) Debug::crash(__FILE__, __LINE__, std::string(message))
#define DebugAssert(condition, message) Debug::check(condition, __FILE__, __LINE__, std::string(message))
#define DebugNotImplemented() Debug::crash(__FILE__, __LINE__, "Not implemented.")
};

#endif
