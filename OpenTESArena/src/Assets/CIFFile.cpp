#include <algorithm>
#include <unordered_map>

#include "CIFFile.h"

#include "../Math/Int2.h"
#include "../Utilities/Bytes.h"
#include "../Utilities/Debug.h"

#include "components/vfs/manager.hpp"

namespace
{
	// These CIF files are headerless with hardcoded dimensions.
	const std::unordered_map<std::string, Int2> RawCifOverride =
	{
		{ "BRASS.CIF", { 8, 8 } },
		{ "BRASS2.CIF", { 8, 8 } },
		{ "MARBLE.CIF", { 3, 3 } },
		{ "MARBLE2.CIF", { 3, 3 } },
		{ "PARCH.CIF", { 20, 20 } },
		{ "SCROLL.CIF", { 20, 20 } }
	};

	// The method for finding how many images are in a CIF is currently unknown,
	// so they are hardcoded from experimental data for now. If the image count
	// is not found, assume it is 1.
	/*const std::unordered_map<std::string, int> CifImageCounts =
	{
		{ "ARROWS.CIF", 9 },
		{ "AXE.CIF", 33 },
		{ "CHAIN.CIF", 33 },
		{ "HAMMER.CIF", 33 },
		{ "HAND.CIF", 13 },
		{ "KEYS.CIF", 12 },
		{ "MACE.CIF", 33 },
		{ "PLATE.CIF", 33 },
		{ "STAFF.CIF", 33 },
		{ "STAR.CIF", 33 },
		{ "SWORD.CIF", 33 }
	};*/
}

CIFFile::CIFFile(const std::string &filename, const Palette &palette)
	: pixels(), dimensions()
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "CIFFile", "Could not open \"" + filename + "\".");

	stream->seekg(0, std::ios::end);
	const auto fileSize = stream->tellg();
	stream->seekg(0, std::ios::beg);

	std::vector<uint8_t> srcData(fileSize);
	stream->read(reinterpret_cast<char*>(srcData.data()), srcData.size());

	// X and Y offset might be useful for weapon positions on the screen.
	uint16_t xoff, yoff, width, height, flags, len;

	// Read header data if it is not a raw file.
	const auto rawOverride = RawCifOverride.find(filename);
	const bool isRaw = rawOverride != RawCifOverride.end();
	if (isRaw)
	{
		xoff = 0;
		yoff = 0;
		width = rawOverride->second.getX();
		height = rawOverride->second.getY();
		flags = 0;
		len = width * height;
	}
	else
	{
		xoff = Bytes::getLE16(srcData.data());
		yoff = Bytes::getLE16(srcData.data() + 2);
		width = Bytes::getLE16(srcData.data() + 4);
		height = Bytes::getLE16(srcData.data() + 6);
		flags = Bytes::getLE16(srcData.data() + 8);
		len = Bytes::getLE16(srcData.data() + 10);
	}

	const int headerSize = 12;

	if (flags == 0x0802)
	{		
		Debug::crash("CIFFile", "Type 2 not implemented.");
	}
	else if (flags == 0x0804)
	{
		Debug::crash("CIFFile", "Type 4 not implemented.");
	}
	else if (isRaw)
	{
		// Uncompressed raw CIF.
		this->pixels.push_back(std::unique_ptr<uint32_t>(new uint32_t[width * height]));
		this->dimensions.push_back(Int2(width, height));

		const uint8_t *imagePixels = srcData.data();
		uint32_t *dstPixels = this->pixels.at(this->pixels.size() - 1).get();

		std::transform(imagePixels, imagePixels + len, dstPixels,
			[&palette](uint8_t col) -> uint32_t
		{
			return palette[col].toARGB();
		});
	}
	else if (flags == 0x0000)
	{
		// Uncompressed CIF with headers.
		int offset = 0;

		// Read uncompressed images until the end of the file.
		while ((srcData.begin() + offset) < srcData.end())
		{
			const uint8_t *header = srcData.data() + offset;
			xoff = Bytes::getLE16(header);
			yoff = Bytes::getLE16(header + 2);
			width = Bytes::getLE16(header + 4);
			height = Bytes::getLE16(header + 6);
			flags = Bytes::getLE16(header + 8);
			len = Bytes::getLE16(header + 10);

			this->pixels.push_back(std::unique_ptr<uint32_t>(new uint32_t[width * height]));
			this->dimensions.push_back(Int2(width, height));

			const uint8_t *imagePixels = header + headerSize;
			uint32_t *dstPixels = this->pixels.at(this->pixels.size() - 1).get();

			std::transform(imagePixels, imagePixels + len, dstPixels,
				[&palette](uint8_t col) -> uint32_t
			{
				return palette[col].toARGB();
			});

			// Skip to the next image header.
			offset += (headerSize + len);
		}
	}
	else
	{
		Debug::crash("CIFFile", "Unrecognized flags " + std::to_string(flags) + ".");
	}
}

CIFFile::~CIFFile()
{

}

int CIFFile::getImageCount() const
{
	return static_cast<int>(this->pixels.size());
}

int CIFFile::getWidth(int index) const
{
	return this->dimensions.at(index).getX();
}

int CIFFile::getHeight(int index) const
{
	return this->dimensions.at(index).getY();
}

uint32_t *CIFFile::getPixels(int index) const
{
	return this->pixels.at(index).get();
}
