#include <unordered_map>

#include "IMGFile.h"

#include "Compression.h"
#include "../Math/Vector2.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

namespace
{
	// These IMG files are actually headerless/raw files with hardcoded dimensions.
	const std::unordered_map<std::string, Int2> RawImgOverride =
	{
		{ "ARENARW.IMG", { 16, 16 } },
		{ "CITY.IMG", { 16, 11 } },
		{ "DITHER.IMG", { 16, 50 } },
		{ "DITHER2.IMG", { 16, 50 } },
		{ "DUNGEON.IMG", { 14,  8 } },
		{ "DZTTAV.IMG", { 32, 34 } },
		{ "NOCAMP.IMG", { 25, 19 } },
		{ "NOSPELL.IMG", { 25, 19 } },
		{ "P1.IMG", { 320, 53 } },
		{ "POPTALK.IMG", { 320, 77 } },
		{ "S2.IMG", { 320, 36 } },
		{ "SLIDER.IMG", { 289,  7 } },
		{ "TOWN.IMG", { 9, 10 } },
		{ "UPDOWN.IMG", { 8, 16 } },
		{ "VILLAGE.IMG", { 8,  8 } }
	};
}

IMGFile::IMGFile(const std::string &filename, const Palette *palette)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "IMGFile", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	uint16_t xoff, yoff, width, height, flags, len;

	// Read header data if not raw. Wall IMGs have no header and are 4096 bytes.
	const auto rawOverride = RawImgOverride.find(filename);
	const bool isRaw = rawOverride != RawImgOverride.end();
	if (isRaw)
	{
		xoff = 0;
		yoff = 0;
		width = rawOverride->second.x;
		height = rawOverride->second.y;
		flags = 0;
		len = width * height;
	}
	else if (srcData.size() == 4096)
	{
		// Some wall IMGs have rows of black (transparent) pixels near the 
		// beginning, so the header would just be zeroes. This is a guess to 
		// try and fix that issue as well as cover all other wall IMGs.
		xoff = 0;
		yoff = 0;
		width = 64;
		height = 64;
		flags = 0;
		len = width * height;
	}
	else
	{
		// Read header data.
		xoff = Bytes::getLE16(srcData.data());
		yoff = Bytes::getLE16(srcData.data() + 2);
		width = Bytes::getLE16(srcData.data() + 4);
		height = Bytes::getLE16(srcData.data() + 6);
		flags = Bytes::getLE16(srcData.data() + 8);
		len = Bytes::getLE16(srcData.data() + 10);
	}

	const int headerSize = 12;

	// Try and read the IMG's built-in palette if the given palette is null.
	Palette builtInPalette;
	const bool useBuiltInPalette = palette == nullptr;

	if (useBuiltInPalette)
	{
		// This code might run even if the IMG doesn't have a palette, because
		// some IMGs have no header and are not "raw" (like walls, for instance).
		Debug::check((flags & 0x0100) != 0, "IMGFile",
			"\"" + filename + "\" does not have a built-in palette.");

		// Read the palette data and write it to the destination palette.
		IMGFile::readPalette(srcData.data() + headerSize + len, builtInPalette);
	}

	// Choose which palette to use.
	const Palette &paletteRef = useBuiltInPalette ? builtInPalette : (*palette);

	// Lambda for setting IMGFile members and constructing the final image.
	auto makeImage = [this, &paletteRef](int width, int height, const uint8_t *data)
	{
		this->width = width;
		this->height = height;
		this->pixels = std::unique_ptr<uint32_t[]>(new uint32_t[width * height]);

		std::transform(data, data + (width * height), this->pixels.get(),
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});
	};

	// Decide how to use the pixel data.
	if (isRaw)
	{
		// Uncompressed IMG with no header (excluding walls).
		makeImage(width, height, srcData.data());
	}
	else if ((srcData.size() == 4096) && (width == 64) && (height == 64))
	{
		// Wall texture (the flags variable is garbage).
		makeImage(64, 64, srcData.data());
	}
	else
	{
		// Decode the pixel data according to the IMG flags.
		if ((flags & 0x00FF) == 0)
		{
			// Uncompressed IMG with header.
			makeImage(width, height, srcData.data() + headerSize);
		}
		else if ((flags & 0x00FF) == 0x0004)
		{
			// Type 4 compression.
			std::vector<uint8_t> decomp(width * height);
			Compression::decodeType04(srcData.begin() + headerSize,
				srcData.begin() + headerSize + len, decomp);

			// Create 32-bit image.
			makeImage(width, height, decomp.data());
		}
		else if ((flags & 0x00FF) == 0x0008)
		{
			// Type 8 compression. Contains a 2 byte decompressed length after
			// the header, so skip that (should be equivalent to width * height).
			std::vector<uint8_t> decomp(width * height);
			Compression::decodeType08(srcData.begin() + headerSize + 2,
				srcData.begin() + headerSize + len, decomp);

			// Create 32-bit image.
			makeImage(width, height, decomp.data());
		}
		else
		{
			Debug::crash("IMGFile", "Unrecognized IMG \"" + filename + "\".");
		}
	}
}

IMGFile::~IMGFile()
{

}

void IMGFile::readPalette(const uint8_t *paletteData, Palette &dstPalette)
{
	// The palette data is 768 bytes, starting after the pixel data ends.
	// Unlike COL files, embedded palettes are stored with components in
	// the range of 0...63 rather than 0...255 (this was because old VGA
	// hardware only had 6-bit DACs, giving a maximum intensity value of
	// 63, while newer hardware had 8-bit DACs for up to 255.
	uint8_t r = std::min<uint8_t>(*(paletteData++), 63) * 255 / 63;
	uint8_t g = std::min<uint8_t>(*(paletteData++), 63) * 255 / 63;
	uint8_t b = std::min<uint8_t>(*(paletteData++), 63) * 255 / 63;
	dstPalette[0] = Color(r, g, b, 0);

	// Remaining are solid, so give them 255 alpha.
	std::generate(dstPalette.begin() + 1, dstPalette.end(),
		[&paletteData]() -> Color
	{
		uint8_t r = std::min<uint8_t>(*(paletteData++), 63) * 255 / 63;
		uint8_t g = std::min<uint8_t>(*(paletteData++), 63) * 255 / 63;
		uint8_t b = std::min<uint8_t>(*(paletteData++), 63) * 255 / 63;
		return Color(r, g, b, 255);
	});
}

void IMGFile::extractPalette(const std::string &filename, Palette &dstPalette)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "IMGFile", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// No need to check for raw override. All given filenames should point to IMGs
	// with "built-in" palettes, and none of those IMGs are in the raw override.	
	uint16_t xoff = Bytes::getLE16(srcData.data());
	uint16_t yoff = Bytes::getLE16(srcData.data() + 2);
	uint16_t width = Bytes::getLE16(srcData.data() + 4);
	uint16_t height = Bytes::getLE16(srcData.data() + 6);
	uint16_t flags = Bytes::getLE16(srcData.data() + 8);
	uint16_t len = Bytes::getLE16(srcData.data() + 10);

	const int headerSize = 12;

	// Don't try to read a built-in palette is there isn't one.
	Debug::check((flags & 0x0100) != 0, "IMGFile",
		"\"" + filename + "\" has no built-in palette to extract.");

	// Read the palette data and write it to the destination palette.
	IMGFile::readPalette(srcData.data() + headerSize + len, dstPalette);
}

int IMGFile::getWidth() const
{
	return this->width;
}

int IMGFile::getHeight() const
{
	return this->height;
}

uint32_t *IMGFile::getPixels() const
{
	return this->pixels.get();
}
