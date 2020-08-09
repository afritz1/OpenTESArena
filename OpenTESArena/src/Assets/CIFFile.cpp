#include <algorithm>
#include <string>
#include <unordered_map>

#include "CIFFile.h"
#include "Compression.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"
#include "components/vfs/manager.hpp"

namespace
{
	// These CIF files are headerless with a hardcoded frame count and pair
	// of dimensions (they seem to all be tile-based).
	const std::unordered_map<std::string, std::pair<int, Int2>> RawCifOverride =
	{
		{ "BRASS.CIF", { 9, Int2(8, 8) } },
		{ "BRASS2.CIF", { 9, Int2(8, 8) } },
		{ "MARBLE.CIF", { 9, Int2(3, 3) } },
		{ "MARBLE2.CIF", { 9, Int2(3, 3) } },
		{ "PARCH.CIF", { 9, Int2(20, 20) } },
		{ "SCROLL.CIF", { 9, Int2(20, 20) } }
	};
}

bool CIFFile::init(const char *filename)
{
	// Some filenames (i.e., Arrows.cif) have different casing between the floppy version and
	// CD version, so this needs to use the case-insensitive open() method for correct behavior
	// on Unix-based systems.
	Buffer<std::byte> src;
	if (!VFS::Manager::get().readCaseInsensitive(filename, &src))
	{
		DebugLogError("Could not read \"" + std::string(filename) + "\".");
		return false;
	}

	const uint8_t *srcPtr = reinterpret_cast<const uint8_t*>(src.get());
	const uint8_t *srcEnd = reinterpret_cast<const uint8_t*>(src.end());

	// X and Y offset might be useful for weapon positions on the screen.
	uint16_t xOffset, yOffset, width, height, flags, len;

	// Read header data if it is not a raw file.
	const auto rawOverrideIter = RawCifOverride.find(filename);
	const bool isRaw = rawOverrideIter != RawCifOverride.end();
	if (isRaw)
	{
		xOffset = 0;
		yOffset = 0;

		const Int2 &dims = rawOverrideIter->second.second;
		width = dims.x;
		height = dims.y;

		flags = 0;
		len = width * height;
	}
	else
	{
		xOffset = Bytes::getLE16(srcPtr);
		yOffset = Bytes::getLE16(srcPtr + 2);
		width = Bytes::getLE16(srcPtr + 4);
		height = Bytes::getLE16(srcPtr + 6);
		flags = Bytes::getLE16(srcPtr + 8);
		len = Bytes::getLE16(srcPtr + 10);
	}

	constexpr int headerSize = 12;

	if ((flags & 0x00FF) == 0x0002)
	{
		// Type 2 .CIF.
		int offset = 0;

		while ((srcPtr + offset) < srcEnd)
		{
			const uint8_t *header = srcPtr + offset;
			xOffset = Bytes::getLE16(header);
			yOffset = Bytes::getLE16(header + 2);
			width = Bytes::getLE16(header + 4);
			height = Bytes::getLE16(header + 6);
			flags = Bytes::getLE16(header + 8);
			len = Bytes::getLE16(header + 10);

			std::vector<uint8_t> decomp(width * height);
			Compression::decodeRLE(header + 12, width * height, decomp.data(),
				static_cast<int>(decomp.size()));

			this->images.push_back(Buffer2D<uint8_t>(width, height));
			this->offsets.push_back(Int2(xOffset, yOffset));

			const uint8_t *srcPixels = decomp.data();
			uint8_t *dstPixels = this->images.back().get();
			std::copy(srcPixels, srcPixels + (width * height), dstPixels);

			offset += headerSize + len;
		}
	}
	else if ((flags & 0x00FF) == 0x0004)
	{
		// Type 4 .CIF.
		int offset = 0;

		while ((srcPtr + offset) < srcEnd)
		{
			const uint8_t *header = srcPtr + offset;
			xOffset = Bytes::getLE16(header);
			yOffset = Bytes::getLE16(header + 2);
			width = Bytes::getLE16(header + 4);
			height = Bytes::getLE16(header + 6);
			flags = Bytes::getLE16(header + 8);
			len = Bytes::getLE16(header + 10);

			std::vector<uint8_t> decomp(width * height);
			Compression::decodeType04(header + 12, header + 12 + len, decomp);

			this->images.push_back(Buffer2D<uint8_t>(width, height));
			this->offsets.push_back(Int2(xOffset, yOffset));

			const uint8_t *srcPixels = decomp.data();
			uint8_t *dstPixels = this->images.back().get();
			std::copy(srcPixels, srcPixels + (width * height), dstPixels);

			offset += headerSize + len;
		}
	}
	else if ((flags & 0x00FF) == 0x0008)
	{
		// Type 8 .CIF.
		int offset = 0;

		while ((srcPtr + offset) < srcEnd)
		{
			const uint8_t *header = srcPtr + offset;
			xOffset = Bytes::getLE16(header);
			yOffset = Bytes::getLE16(header + 2);
			width = Bytes::getLE16(header + 4);
			height = Bytes::getLE16(header + 6);
			flags = Bytes::getLE16(header + 8);
			len = Bytes::getLE16(header + 10);

			std::vector<uint8_t> decomp(width * height);

			// Contains a 2 byte decompressed length after the header, so skip that 
			// (should be equivalent to width * height).
			Compression::decodeType08(header + 12 + 2, header + 12 + len, decomp);

			this->images.push_back(Buffer2D<uint8_t>(width, height));
			this->offsets.push_back(Int2(xOffset, yOffset));

			const uint8_t *srcPixels = decomp.data();
			uint8_t *dstPixels = this->images.back().get();
			std::copy(srcPixels, srcPixels + (width * height), dstPixels);

			// Skip to the next image header.
			offset += headerSize + len;
		}
	}
	else if (isRaw)
	{
		// Uncompressed raw .CIF.
		const int imageCount = rawOverrideIter->second.first;

		for (int i = 0; i < imageCount; i++)
		{
			this->images.push_back(Buffer2D<uint8_t>(width, height));
			this->offsets.push_back(Int2(xOffset, yOffset));

			const uint8_t *srcPixels = srcPtr + (i * len);
			uint8_t *dstPixels = this->images.back().get();
			std::copy(srcPixels, srcPixels + len, dstPixels);
		}
	}
	else if ((flags & 0x00FF) == 0)
	{
		// Uncompressed .CIF with headers.
		int offset = 0;

		// Read uncompressed images until the end of the file.
		while ((srcPtr + offset) < srcEnd)
		{
			const uint8_t *header = srcPtr + offset;
			xOffset = Bytes::getLE16(header);
			yOffset = Bytes::getLE16(header + 2);
			width = Bytes::getLE16(header + 4);
			height = Bytes::getLE16(header + 6);
			flags = Bytes::getLE16(header + 8);
			len = Bytes::getLE16(header + 10);

			this->images.push_back(Buffer2D<uint8_t>(width, height));
			this->offsets.push_back(Int2(xOffset, yOffset));

			const uint8_t *srcPixels = header + headerSize;
			uint8_t *dstPixels = this->images.back().get();
			std::copy(srcPixels, srcPixels + len, dstPixels);

			// Skip to the next image header.
			offset += headerSize + len;
		}
	}
	else
	{
		DebugLogError("Unrecognized flags " + std::to_string(flags) + ".");
		return false;
	}

	return true;
}

int CIFFile::getImageCount() const
{
	return static_cast<int>(this->images.size());
}

int CIFFile::getXOffset(int index) const
{
	DebugAssertIndex(this->offsets, index);
	return this->offsets[index].x;
}

int CIFFile::getYOffset(int index) const
{
	DebugAssertIndex(this->offsets, index);
	return this->offsets[index].y;
}

int CIFFile::getWidth(int index) const
{
	DebugAssertIndex(this->images, index);
	return this->images[index].getWidth();
}

int CIFFile::getHeight(int index) const
{
	DebugAssertIndex(this->images, index);
	return this->images[index].getHeight();
}

const uint8_t *CIFFile::getPixels(int index) const
{
	DebugAssertIndex(this->images, index);
	return this->images[index].get();
}
