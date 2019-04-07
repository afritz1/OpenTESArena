#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <string_view>
#include <vector>

class StringView
{
private:
	StringView() = delete;
	~StringView() = delete;
public:
	// Splits a string view on the given character.
	static std::vector<std::string_view> split(const std::string_view &str, char separator);

	// Splits a string view on whitespace.
	static std::vector<std::string_view> split(const std::string_view &str);

	// Removes leading whitespace from a string view.
	static std::string_view trimFront(const std::string_view &str);

	// Removes trailing whitespace from a string view.
	static std::string_view trimBack(const std::string_view &str);

	// Gets the right-most extension from a string view, i.e., "txt".
	static std::string_view getExtension(const std::string_view &str);
};

#endif
