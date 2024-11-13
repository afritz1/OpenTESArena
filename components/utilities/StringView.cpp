#include <cctype>

#include "StringView.h"

bool StringView::equals(std::string_view a, std::string_view b)
{
	return a == b;
}

bool StringView::caseInsensitiveEquals(std::string_view a, std::string_view b)
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

std::string_view StringView::substr(std::string_view str, size_t offset, size_t count)
{
	return str.substr(offset, count);
}

Buffer<std::string_view> StringView::split(std::string_view str, char separator)
{
	// Always have at least one view.
	const int viewCount = 1 + static_cast<int>(std::count(str.begin(), str.end(), separator));
	Buffer<std::string_view> buffer(viewCount);
	buffer[0] = std::string_view(str.data(), 0);

	int writeIndex = 0;
	for (size_t i = 0; i < str.size(); i++)
	{
		const char c = str[i];
		if (c == separator)
		{
			// Start a new string.
			writeIndex++;
			buffer[writeIndex] = std::string_view(str.data() + i + 1, 0);
		}
		else
		{
			// Put the character on the end of the current string.
			std::string_view &currentView = buffer.get(writeIndex);
			currentView = std::string_view(currentView.data(), currentView.size() + 1);
		}
	}

	return buffer;
}

Buffer<std::string_view> StringView::split(std::string_view str)
{
	return StringView::split(str, String::SPACE);
}

std::string_view StringView::trimFront(std::string_view str)
{
	std::string_view trimmed(str);
	while ((trimmed.size() > 0) && std::isspace(trimmed.front()))
	{
		trimmed.remove_prefix(1);
	}

	return trimmed;
}

std::string_view StringView::trimBack(std::string_view str)
{
	std::string_view trimmed(str);
	while ((trimmed.size() > 0) && std::isspace(trimmed.back()))
	{
		trimmed.remove_suffix(1);
	}

	return trimmed;
}

std::string_view StringView::getExtension(std::string_view str)
{
	const size_t dotPos = str.rfind(String::FILE_EXTENSION_SEPARATOR);
	const bool hasDot = (dotPos < str.size()) && (dotPos != std::string_view::npos);
	return hasDot ? std::string_view(
		str.data() + dotPos + 1, str.size() - dotPos - 1) : std::string_view();
}
