#pragma once

#include <array>

namespace DOSUtils
{
	// DOS filename buffer size (8 characters + 1 period + 3 extension + 1 null).
	constexpr int FILENAME_BUFFER_SIZE = 13;

	using FilenameBuffer = std::array<char, FILENAME_BUFFER_SIZE>;
}
