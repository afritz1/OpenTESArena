#include <algorithm>
#include <cstdint>

#include "COLFile.h"

#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

void COLFile::toPalette(const std::string &filename, Palette &dstPalette)
{
	bool failed = false;
	std::array<uint8_t, 776> rawpal;
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());

	if (!stream)
	{
		Debug::mention("COLFile", "Failed to open palette \"" + filename + "\".");
		failed = true;
	}
	else
	{
		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		if (stream->gcount() != static_cast<std::streamsize>(rawpal.size()))
		{
			Debug::mention("COLFile", "Failed to read palette \"" +
				filename + "\", got " + std::to_string(stream->gcount()) + " bytes.");
			failed = true;
		}
	}
	if (!failed)
	{
		uint32_t len = Bytes::getLE32(rawpal.data());
		uint32_t ver = Bytes::getLE32(rawpal.data() + 4);
		if (len != 776)
		{
			Debug::mention("COLFile", "Invalid length for palette \"" +
				filename + "\" (" + std::to_string(len) + " bytes).");
			failed = true;
		}
		else if (ver != 0xB123)
		{
			Debug::mention("COLFile", "Invalid version for palette \"" +
				filename + "\", 0x" + String::toHexString(ver) + ".");
			failed = true;
		}
	}

	if (!failed)
	{
		auto iter = rawpal.begin() + 8;

		// First palette entry is transparent in 8-bit modes, so give it 0 alpha.
		uint8_t r = *(iter++);
		uint8_t g = *(iter++);
		uint8_t b = *(iter++);
		dstPalette[0] = Color(r, g, b, 0);

		// Remaining are solid, so give them 255 alpha.
		std::generate(dstPalette.begin() + 1, dstPalette.end(),
			[&iter]() -> Color
		{
			uint8_t r = *(iter++);
			uint8_t g = *(iter++);
			uint8_t b = *(iter++);
			return Color(r, g, b, 255);
		});
	}
	if (failed)
	{
		// Generate a monochrome palette. Entry 0 is filled with 0 already, so skip it.
		uint8_t count = 0;
		std::generate(dstPalette.begin() + 1, dstPalette.end(),
			[&count]() -> Color
		{
			uint8_t c = ++count;
			return Color(c, c, c, 255);
		});
	}
}
