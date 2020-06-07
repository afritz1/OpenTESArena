#ifndef DEBUG_H
#define DEBUG_H

#include <stdexcept>
#include <string>
#include <type_traits>

// Below are various debug methods and macros for replacing asserts or program exits 
// that might be accompanied with messages and logging.

class Debug
{
public:
	enum class MessageType
	{
		Status,
		Warning,
		Error
	};
private:
	static const std::string LOG_FILENAME;

	Debug() = delete;
	~Debug() = delete;

	// Writes a debug message to the console with the file path and line number.
	static void write(Debug::MessageType type, const std::string &filePath,
		int lineNumber, const std::string &message);
public:
	// Shortens the __FILE__ macro so it only includes a couple parent folders.
	static std::string getShorterPath(const char *__file__);

	// Use DebugLog() instead. Helper method for mentioning something about program state.
	static void log(const char *__file__, int lineNumber, const std::string &message);

	// Use DebugLogWarning() instead. Helper method for warning the user about something.
	static void logWarning(const char *__file__, int lineNumber, const std::string &message);

	// Use DebugLogError() instead. Helper method for reporting an error while still continuing.
	static void logError(const char *__file__, int lineNumber, const std::string &message);

	// Use DebugCrash() instead. Helper method for crashing the program with a reason.
	static void crash(const char *__file__, int lineNumber, const std::string &message);

	// General logging defines.
#define DebugLog(message) Debug::log(__FILE__, __LINE__, message)
#define DebugLogWarning(message) Debug::logWarning(__FILE__, __LINE__, message)
#define DebugLogError(message) Debug::logError(__FILE__, __LINE__, message)

	// Crash define, when the program simply cannot continue.
#define DebugCrash(message) Debug::crash(__FILE__, __LINE__, message)

	// Assertions.
#define DebugAssertMsg(condition, message) \
	do { if (!(condition)) DebugCrash("Assertion failed: " + std::string(message)); } while (false)
#define DebugAssert(condition) \
	do { if (!(condition)) DebugCrash("Assertion failed: \"" + std::string(#condition) + "\""); } while (false)

	// Exception generator with file and line.
#define DebugException(message) \
	std::runtime_error(std::string(message) + " (" + Debug::getShorterPath(__FILE__) + "(" + std::to_string(__LINE__) + "))")

	// Various error handlers:
	// Unhandled return.
#define DebugUnhandledReturnMsg(returnType, message) \
	do { DebugAssertMsg(false, "Unhandled return: " + std::string(message)); return returnType(); } while (false)
#define DebugUnhandledReturn(returnType) \
	do { DebugAssertMsg(false, "Unhandled return."); return returnType(); } while (false)

	// Unimplemented code.
#define DebugNotImplementedMsg(message) \
	DebugCrash("Not implemented: " + std::string(message))
#define DebugNotImplemented() \
	DebugCrash("Not implemented.")

	// Bounds-checking.
#define DebugValidIndex(container, index) \
	(std::is_integral_v<decltype(index)> && (index >= 0) && (index < std::size(container)))
#define DebugAssertIndex(container, index) \
	do { if (!DebugValidIndex(container, index)) DebugCrash("Index '" + std::to_string(index) + "' out of bounds."); } while (false)
#define DebugMakeIndex(container, index) \
	([&]() { const auto val = index; DebugAssertIndex(container, val); return val; }())
};

#endif
