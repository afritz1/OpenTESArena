#include <cctype>

#include "StringView.h"

bool StringView::caseInsensitiveEquals(const std::string_view &a, const std::string_view &b)
{
	if (a.size() != b.size())
	{
		return false;
	}

	for (size_t i = 0; i < a.size(); i++)
	{
		if (std::tolower(a[i]) != std::tolower(b[i]))
		{
			return false;
		}
	}

	return true;
}

std::string_view StringView::substr(const std::string_view &str, size_t offset, size_t count)
{
	return str.substr(offset, count);
}

std::vector<std::string_view> StringView::split(const std::string_view &str, char separator)
{
	std::vector<std::string_view> strings;

	// Add an empty string view to start off. If the given string view is empty, then a
	// vector with one empty string view is returned.
	strings.push_back(std::string_view(str.data(), 0));

	for (size_t i = 0; i < str.size(); i++)
	{
		const char c = str[i];

		if (c == separator)
		{
			// Start a new string.
			strings.push_back(std::string_view(str.data() + i + 1, 0));
		}
		else
		{
			// Put the character on the end of the current string.
			std::string_view old = strings.back();
			strings.back() = std::string_view(old.data(), old.size() + 1);
		}
	}

	return strings;
}

std::vector<std::string_view> StringView::split(const std::string_view &str)
{
	return StringView::split(str, String::SPACE);
}

std::string_view StringView::trimFront(const std::string_view &str)
{
	std::string_view trimmed(str);
	while ((trimmed.size() > 0) && std::isspace(trimmed.front()))
	{
		trimmed.remove_prefix(1);
	}

	return trimmed;
}

std::string_view StringView::trimBack(const std::string_view &str)
{
	std::string_view trimmed(str);
	while ((trimmed.size() > 0) && std::isspace(trimmed.back()))
	{
		trimmed.remove_suffix(1);
	}

	return trimmed;
}

std::string_view StringView::getExtension(const std::string_view &str)
{
	const size_t dotPos = str.rfind(String::FILE_EXTENSION_SEPARATOR);
	const bool hasDot = (dotPos < str.size()) && (dotPos != std::string_view::npos);
	return hasDot ? std::string_view(
		str.data() + dotPos + 1, str.size() - dotPos - 1) : std::string_view();
}
