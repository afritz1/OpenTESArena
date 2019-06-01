#include <algorithm>
#include <array>
#include <cstdint>

#include "COLFile.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

COLFile::COLFile(const std::string &filename)
{
	bool failed = false;
	std::array<uint8_t, 776> rawPal;
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename);

	if (stream == nullptr)
	{
		DebugLogWarning("Failed to open palette \"" + filename + "\".");
		failed = true;
	}
	else
	{
		stream->read(reinterpret_cast<char*>(rawPal.data()), rawPal.size());
		if (stream->gcount() != static_cast<std::streamsize>(rawPal.size()))
		{
			DebugLogWarning("Failed to read palette \"" + filename + "\", got " +
				std::to_string(stream->gcount()) + " bytes.");
			failed = true;
		}
	}

	if (!failed)
	{
		const uint32_t len = Bytes::getLE32(rawPal.data());
		const uint32_t ver = Bytes::getLE32(rawPal.data() + 4);
		if (len != 776)
		{
			DebugLogWarning("Invalid length for palette \"" + filename + "\" (" +
				std::to_string(len) + " bytes).");
			failed = true;
		}
		else if (ver != 0xB123)
		{
			DebugLogWarning("Invalid version for palette \"" + filename + "\", 0x" +
				String::toHexString(ver) + ".");
			failed = true;
		}
	}

	if (!failed)
	{
		auto iter = rawPal.begin() + 8;

		// First palette entry is transparent in 8-bit modes, so give it 0 alpha.
		uint8_t r = *(iter++);
		uint8_t g = *(iter++);
		uint8_t b = *(iter++);
		this->palette.get()[0] = Color(r, g, b, 0);

		// Remaining are solid, so give them 255 alpha.
		std::generate(this->palette.get().begin() + 1, this->palette.get().end(),
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
		std::generate(this->palette.get().begin() + 1, this->palette.get().end(),
			[&count]() -> Color
		{
			uint8_t c = ++count;
			return Color(c, c, c, 255);
		});
	}
}

const Palette &COLFile::getPalette() const
{
	return this->palette;
}
