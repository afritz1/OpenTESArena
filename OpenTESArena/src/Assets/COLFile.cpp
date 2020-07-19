#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

#include "COLFile.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/utilities/String.h"
#include "components/vfs/manager.hpp"

bool COLFile::init(const char *filename)
{
	std::array<uint8_t, 8 + (256 * 3)> rawPal;
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);
	bool success = true;

	if (stream == nullptr)
	{
		DebugLogWarning("Could not open \"" + std::string(filename) + "\".");
		success = false;
	}
	else
	{
		stream->read(reinterpret_cast<char*>(rawPal.data()), rawPal.size());
		if (stream->gcount() != static_cast<std::streamsize>(rawPal.size()))
		{
			DebugLogWarning("Could not read \"" + std::string(filename) + "\", got " +
				std::to_string(stream->gcount()) + " bytes.");
			success = false;
		}
	}

	if (success)
	{
		const uint32_t len = Bytes::getLE32(rawPal.data());
		const uint32_t ver = Bytes::getLE32(rawPal.data() + 4);
		if (len != 776)
		{
			DebugLogWarning("Invalid length for palette \"" + std::string(filename) + "\" (" +
				std::to_string(len) + " bytes).");
			success = false;
		}
		else if (ver != 0xB123)
		{
			DebugLogWarning("Invalid version for palette \"" + std::string(filename) + "\", 0x" +
				String::toHexString(ver) + ".");
			success = false;
		}
	}

	if (success)
	{
		auto iter = rawPal.begin() + 8;

		// First palette entry is transparent in 8-bit modes, so give it 0 alpha.
		uint8_t r = *(iter++);
		uint8_t g = *(iter++);
		uint8_t b = *(iter++);
		this->palette[0] = Color(r, g, b, 0);

		// Remaining are solid, so give them 255 alpha.
		std::generate(this->palette.begin() + 1, this->palette.end(),
			[&iter]() -> Color
		{
			uint8_t r = *(iter++);
			uint8_t g = *(iter++);
			uint8_t b = *(iter++);
			return Color(r, g, b, 255);
		});
	}
	else
	{
		// Generate a monochrome palette. Entry 0 is filled with 0 already, so skip it.
		uint8_t count = 0;
		std::generate(this->palette.begin() + 1, this->palette.end(),
			[&count]() -> Color
		{
			uint8_t c = ++count;
			return Color(c, c, c, 255);
		});
	}

	return true;
}

const Palette &COLFile::getPalette() const
{
	return this->palette;
}
