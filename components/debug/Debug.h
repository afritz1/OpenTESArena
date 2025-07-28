#ifndef DEBUG_H
#define DEBUG_H

#include <cstdio>
#include <string>
#include <type_traits>

// Various macros/asserts that may be accompanied with messages and file logging.
namespace Debug
{
	static const std::string MessagePrefixLog = "";
	static const std::string MessagePrefixWarning = "Warning: ";
	static const std::string MessagePrefixError = "Error: ";

	bool init(const char *logDirectory);
	void shutdown();

	// Shortens __FILE__ for readability.
	std::string getShorterPath(const char *__file__);

	[[noreturn]] void exitApplication();

	void write(const std::string &filePath, int lineNumber, const std::string &messagePrefix, const std::string &message);

	template<typename... Args>
	static std::string stringFormat(const char *format, Args... args)
	{
		const int charCount = std::snprintf(nullptr, 0, format, args...);
		std::string buffer(charCount + 1, '\0');
		std::snprintf(buffer.data(), buffer.size(), format, args...);
		return buffer;
	}

	template<typename... Args>
	static void internalLogFormat(void (*logFunc)(const char*, int, const std::string&), const char *__file__, int lineNumber, const char *message, Args... args)
	{
		const std::string formattedString = Debug::stringFormat(message, args...);
		logFunc(__file__, lineNumber, formattedString.c_str());
	}

	static void log(const char *__file__, int lineNumber, const std::string &message)
	{
		Debug::write(Debug::getShorterPath(__file__), lineNumber, Debug::MessagePrefixLog, message);
	}

	template<typename... Args>
	static void logFormat(const char *__file__, int lineNumber, const char *message, Args... args)
	{
		Debug::internalLogFormat(Debug::log, __file__, lineNumber, message, args...);
	}

	static void logWarning(const char *__file__, int lineNumber, const std::string &message)
	{
		Debug::write(Debug::getShorterPath(__file__), lineNumber, Debug::MessagePrefixWarning, message);
	}

	template<typename... Args>
	static void logWarningFormat(const char *__file__, int lineNumber, const char *message, Args... args)
	{
		Debug::internalLogFormat(Debug::logWarning, __file__, lineNumber, message, args...);
	}

	static void logError(const char *__file__, int lineNumber, const std::string &message)
	{
		Debug::write(Debug::getShorterPath(__file__), lineNumber, Debug::MessagePrefixError, message);
	}

	template<typename... Args>
	static void logErrorFormat(const char *__file__, int lineNumber, const char *message, Args... args)
	{
		Debug::internalLogFormat(Debug::logError, __file__, lineNumber, message, args...);
	}

	[[noreturn]] static void crash(const char *__file__, int lineNumber, const std::string &message)
	{
		Debug::logError(__file__, lineNumber, message);
		Debug::exitApplication();
	}

	template<typename... Args>
	[[noreturn]] static void crashFormat(const char *__file__, int lineNumber, const char *message, Args... args)
	{
		Debug::internalLogFormat(Debug::logError, __file__, lineNumber, message, args...);
		Debug::exitApplication();
	}
};

#define DebugLog(message) Debug::log(__FILE__, __LINE__, message)
#define DebugLogFormat(message, ...) Debug::logFormat(__FILE__, __LINE__, message, ##__VA_ARGS__)
#define DebugLogWarning(message) Debug::logWarning(__FILE__, __LINE__, message)
#define DebugLogWarningFormat(message, ...) Debug::logWarningFormat(__FILE__, __LINE__, message, ##__VA_ARGS__)
#define DebugLogError(message) Debug::logError(__FILE__, __LINE__, message)
#define DebugLogErrorFormat(message, ...) Debug::logErrorFormat(__FILE__, __LINE__, message, ##__VA_ARGS__)
#define DebugCrash(message) Debug::crash(__FILE__, __LINE__, message)
#define DebugCrashFormat(message, ...) Debug::crashFormat(__FILE__, __LINE__, message, ##__VA_ARGS__)

#define DebugAssertMsg(condition, message) \
	do { if (!(condition)) DebugCrash("Assertion failed: " + std::string(message)); } while (false)
#define DebugAssertMsgFormat(condition, message, ...) \
	do { if (!(condition)) { const std::string formattedString = Debug::stringFormat(message, ##__VA_ARGS__); Debug::logErrorFormat(__FILE__, __LINE__, "Assertion failed: %s", formattedString.c_str()); Debug::exitApplication(); } } while (false)
#define DebugAssert(condition) \
	do { if (!(condition)) DebugCrashFormat("Assertion failed: %s", #condition); } while (false)

#define DebugUnhandledReturnMsg(returnType, message) \
	do { DebugAssertMsg(false, "Unhandled return: " + std::string(message)); return returnType(); } while (false)
#define DebugUnhandledReturn(returnType) \
	do { DebugAssertMsg(false, "Unhandled return."); return returnType(); } while (false)

#define DebugNotImplementedMsg(message) \
	DebugCrash("Not implemented: " + std::string(message))
#define DebugNotImplemented() \
	DebugCrash("Not implemented.")

#define DebugIsValidIndex(container, index) \
	(std::is_integral_v<std::remove_reference_t<decltype(index)>> && ((index) >= 0) && ((index) < std::size(container)))
#define DebugAssertIndex(container, index) \
	do { if (!DebugIsValidIndex(container, index)) DebugCrashFormat("Index '%d' out of bounds.", index); } while (false)
#define DebugMakeIndex(container, index) \
	([&]() { const auto val = (index); DebugAssertIndex(container, val); return val; }())

#endif
