#include <cassert>
#include <unordered_map>

#include "IMGFile.h"

#include "Compression.h"
#include "../Math/Int2.h"
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

	uint16_t xoff, yoff, width, height, flags, srclen;

	auto rawoverride = RawImgOverride.find(filename);
	if (rawoverride != RawImgOverride.end())
	{
		xoff = 0;
		yoff = 0;
		width = rawoverride->second.getX();
		height = rawoverride->second.getY();
		flags = 0;
		srclen = width * height;
	}
	else
	{
		std::array<uint8_t, 12> imghdr;
		stream->read(reinterpret_cast<char*>(imghdr.data()), imghdr.size());
		Debug::check(stream->gcount() == static_cast<std::streamsize>(imghdr.size()),
			"IMGFile", "Could not read \"" + filename + "\" header.");

		xoff = Bytes::getLE16(imghdr.data());
		yoff = Bytes::getLE16(imghdr.data() + 2);
		width = Bytes::getLE16(imghdr.data() + 4);
		height = Bytes::getLE16(imghdr.data() + 6);
		flags = Bytes::getLE16(imghdr.data() + 8);
		srclen = Bytes::getLE16(imghdr.data() + 10);
	}

	std::vector<uint8_t> srcdata(srclen);
	stream->read(reinterpret_cast<char*>(srcdata.data()), srcdata.size());

	// Commented this because wall textures are not in the "raw" list and do not have 
	// a header, therefore causing the byte count to not match 4096 bytes.
	/*Debug::check(stream->gcount() == static_cast<std::streamsize>(srcdata.size()),
		"IMGFile", "Could not read \"" + filename + "\" data.");*/

	Palette custompal;
	const bool useBuiltInPalette = palette == nullptr;
	const bool hasBuiltInPalette = (flags & 0x0100) > 0;

	if (hasBuiltInPalette)
	{
		std::array<uint8_t, 768> rawpal;

		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());

		// Commented because some wall textures are incorrectly matching the flags & 0x0100.
		// Find a way to load wall textures without this problem!
		/*Debug::check(stream->gcount() == static_cast<std::streamsize>(rawpal.size()),
			"IMGFile", "Could not read \"" + filename + "\" palette.");*/

		auto iter = rawpal.begin();

		/* Unlike COL files, embedded palettes are stored with components in
		* the range of 0...63 rather than 0...255 (this was because old VGA
		* hardware only had 6-bit DACs, giving a maximum intensity value of
		* 63, while newer hardware had 8-bit DACs for up to 255.
		*/
		uint8_t r = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		uint8_t g = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		uint8_t b = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		custompal[0] = Color(r, g, b, 0);

		/* Remaining are solid, so give them 255 alpha. */
		std::generate(custompal.begin() + 1, custompal.end(),
			[&iter]() -> Color
		{
			uint8_t r = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			uint8_t g = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			uint8_t b = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			return Color(r, g, b, 255);
		});
	}
	else
	{
		// Don't try to use a built-in palette is there isn't one.
		Debug::check(!useBuiltInPalette, "IMGFile",
			"\"" + filename + "\" does not have a built-in palette.");
	}

	const Palette &paletteRef = (hasBuiltInPalette && useBuiltInPalette) ?
		custompal : (*palette);

	if ((flags & 0x00FF) == 0x0000)
	{
		// Uncompressed IMG.
		assert(srcdata.size() == (width * height));

		// Create 32-bit image.
		this->w = width;
		this->h = height;
		this->pixels = std::unique_ptr<uint32_t>(new uint32_t[this->w * this->h]);

		std::transform(srcdata.begin(), srcdata.end(), this->pixels.get(),
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});
	}
	else if ((flags & 0x00FF) == 0x0004)
	{
		// Type 04 compression.
		std::vector<uint8_t> decomp(width * height);
		Compression::decodeType04(srcdata.begin(), srcdata.end(), decomp);

		// Create 32-bit image.
		this->w = width;
		this->h = height;
		this->pixels = std::unique_ptr<uint32_t>(new uint32_t[this->w * this->h]);

		std::transform(decomp.begin(), decomp.end(), this->pixels.get(),
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});
	}
	else if ((flags & 0x00FF) == 0x0008)
	{
		uint16_t decomplen = Bytes::getLE16(srcdata.data());
		assert(decomplen == (width * height));

		// Type 08 compression.
		std::vector<uint8_t> decomp(width * height);
		Compression::decodeType08(srcdata.begin() + 2, srcdata.end(), decomp);

		// Create 32-bit image.
		this->w = width;
		this->h = height;
		this->pixels = std::unique_ptr<uint32_t>(new uint32_t[this->w * this->h]);

		std::transform(decomp.begin(), decomp.end(), this->pixels.get(),
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});
	}
	else
	{
		// Assume wall texture.
		// - 64x64
		// - 4096 bytes
		// - Uncompressed
		// - No header info

		// This is just a hack for now. It is not completely correct (for example,
		// MURAL3.IMG, a wall texture, can't be loaded because its first few bytes
		// just *happen* to match the built-in palette flags).

		// Reorganize these if statements so both the uncompressed IMG and wall images 
		// fall under the same flag condition. Maybe check if byteCount == 4096?

		// There are no flags, so the header is essentially garbage (just whatever 
		// the texture's colors are).

		width = 64;
		height = 64;
		srclen = width * height;
		srcdata = std::vector<uint8_t>(srclen);

		// Re-read the file in one big 4096 byte chunk.
		// To do: use the original stream in this method.
		VFS::IStreamPtr myStream = VFS::Manager::get().open(filename.c_str());
		myStream->read(reinterpret_cast<char*>(srcdata.data()), srcdata.size());

		// Create 32-bit image.
		this->w = width;
		this->h = height;
		this->pixels = std::unique_ptr<uint32_t>(new uint32_t[this->w * this->h]);

		std::transform(srcdata.begin(), srcdata.end(), this->pixels.get(),
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});
	}
}

IMGFile::~IMGFile()
{

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

	const bool hasBuiltInPalette = (flags & 0x0100) == 0x0100;

	if (hasBuiltInPalette)
	{
		// The palette data is 768 bytes, starting after the pixel data ends.
		auto iter = srcData.begin() + 12 + len;

		/* Unlike COL files, embedded palettes are stored with components in
		* the range of 0...63 rather than 0...255 (this was because old VGA
		* hardware only had 6-bit DACs, giving a maximum intensity value of
		* 63, while newer hardware had 8-bit DACs for up to 255.
		*/
		uint8_t r = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		uint8_t g = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		uint8_t b = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
		dstPalette[0] = Color(r, g, b, 0);

		/* Remaining are solid, so give them 255 alpha. */
		std::generate(dstPalette.begin() + 1, dstPalette.end(),
			[&iter]() -> Color
		{
			uint8_t r = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			uint8_t g = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			uint8_t b = std::min<uint8_t>(*(iter++), 63) * 255 / 63;
			return Color(r, g, b, 255);
		});
	}
	else
	{
		// Don't try to take a built-in palette is there isn't one.
		Debug::crash("IMGFile", "\"" + filename + "\" has no built-in palette to extract.");
	}
}

int IMGFile::getWidth() const
{
	return this->w;
}

int IMGFile::getHeight() const
{
	return this->h;
}

uint32_t *IMGFile::getPixels() const
{
	return this->pixels.get();
}
