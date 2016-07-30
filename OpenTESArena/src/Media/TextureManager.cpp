#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

#include "SDL.h"
#include "SDL_image.h"

#include "TextureManager.h"

#include "Color.h"
#include "PaletteName.h"
#include "TextureFile.h"
#include "../Interface/Surface.h"
#include "../Math/Int2.h"
#include "../Rendering/Renderer.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

namespace
{
	const std::map<PaletteName, std::string> PaletteFilenames =
	{
		{ PaletteName::Default, "PAL.COL" },
		{ PaletteName::CharSheet, "CHARSHT.COL" },
		{ PaletteName::Daytime, "DAYTIME.COL" },
		{ PaletteName::Dreary, "DREARY.COL" }
	};

	// These IMG files are actually headerless/raw files with hardcoded dimensions.
	const std::map<std::string, Int2> RawImgOverride =
	{
		{ "ARENARW.IMG", { 16, 16} },
		{ "CITY.IMG",    { 16, 11} },
		{ "DITHER.IMG",  { 16, 50} },
		{ "DITHER2.IMG", { 16, 50} },
		{ "DUNGEON.IMG", { 14,  8} },
		{ "DZTTAV.IMG",  { 32, 34} },
		{ "NOCAMP.IMG",  { 25, 19} },
		{ "NOSPELL.IMG", { 25, 19} },
		{ "P1.IMG",      {320, 53} },
		{ "POPTALK.IMG", {320, 77} },
		{ "S2.IMG",      {320, 36} },
		{ "SLIDER.IMG",  {289,  7} },
		{ "TOWN.IMG",    {  9, 10} },
		{ "UPDOWN.IMG",  {  8, 16} },
		{ "VILLAGE.IMG", {  8,  8} }
	};

	template<typename T>
	void decode04Type(T src, T srcend, std::vector<uint8_t> &out)
	{
		auto dst = out.begin();

		std::array<uint8_t, 4096> history;
		std::fill(history.begin(), history.end(), 0x20);
		int historypos = 0;

		// This appears to be some form of LZ compression. It starts with a 1-byte-
		// wide bitmask, where each bit declares if the next pixel comes directly
		// from the input, or refers back to a previous run of output pixels that
		// get duplicated. After each bit in the mask is used, another byte is read
		// for another bitmask and the cycle repeats until the end of input.
		int bitcount = 0;
		int mask = 0;
		while (src != srcend)
		{
			if (!bitcount)
			{
				bitcount = 8;
				mask = *(src++);
			}
			else
				mask >>= 1;

			if ((mask & 1))
			{
				if (src == srcend)
					throw std::runtime_error("Unexpected end of image.");
				if (dst == out.end())
					throw std::runtime_error("Decoded image overflow.");
				history[historypos++ & 0x0FFF] = *src;
				*(dst++) = *(src++);
			}
			else
			{
				if (std::distance(src, srcend) < 2)
					throw std::runtime_error("Unexpected end of image.");
				uint8_t byte1 = *(src++);
				uint8_t byte2 = *(src++);
				int tocopy = (byte2 & 0x0F) + 3;
				int copypos = (((byte2 & 0xF0) << 4) | byte1) + 18;

				if (std::distance(dst, out.end()) < tocopy)
					throw std::runtime_error("Decoded image overflow.");

				for (int i = 0; i < tocopy; ++i)
				{
					*dst = history[copypos++ & 0x0FFF];
					history[historypos++ & 0x0FFF] = *(dst++);
				}
			}
			--bitcount;
		}

		std::fill(dst, out.end(), 0);
	}

	template<typename T>
	void decode08Type(T src, T srcend, std::vector<uint8_t> &out)
	{
		static const std::array<uint8_t, 256> highOffsetBits{
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
			0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
			0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
			0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
			0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
			0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
			0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
			0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
			0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
			0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
			0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
			0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
		};
		static const std::array<uint8_t, 256> lowOffsetBitCount{
			0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
			0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
			0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
			0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
			0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
			0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
			0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
			0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
			0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
			0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
			0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
			0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
			0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
		};

		std::array<uint8_t, 4096> history;
		std::fill(history.begin(), history.end(), 0x20);
		int historypos = 0;

		std::array<uint16_t, 941> NodeIdxMap;
		std::iota(NodeIdxMap.begin(), NodeIdxMap.begin() + 626, 0);
		std::for_each(NodeIdxMap.begin(), NodeIdxMap.begin() + 626,
			[](uint16_t &val) { val = (val >> 1) + 314; }
		);
		NodeIdxMap[626] = 0;
		std::iota(NodeIdxMap.begin() + 627, NodeIdxMap.end(), 0);

		std::array<uint16_t, 627> NodeTree;
		std::iota(NodeTree.begin(), NodeTree.begin() + 314, 627);
		std::iota(NodeTree.begin() + 314, NodeTree.end(), 0);
		std::for_each(NodeTree.begin() + 314, NodeTree.end(),
			[](uint16_t &val) { val *= 2; }
		);

		std::array<uint16_t, 627> NodeFreq;
		std::fill(NodeFreq.begin(), NodeFreq.begin() + 314, 1);
		{
			auto iter = NodeFreq.begin();
			std::for_each(NodeFreq.begin() + 314, NodeFreq.begin() + 627,
				[&iter](uint16_t &val)
			{
				val = *(iter++);
				val += *(iter++);
			}
			);
		}

		uint16_t bitmask = 0;
		uint8_t validbits = 0;

		// This feels like some form of adaptive Huffman coding, with a form of LZ
		// compression. DEFLATE?
		auto dst = out.begin();
		while (dst != out.end())
		{
			// Starting with the root, append bits from the input while traversing
			// the tree until a leaf node is found (indicated by being >=627).
			uint16_t node = NodeTree[626];
			while (node < 627)
			{
				while (validbits < 9)
				{
					if (src != srcend)
						bitmask |= *(src++) << (8 - validbits);
					validbits += 8;
				}
				node = NodeTree.at(node + ((bitmask >> 15) & 1));
				bitmask <<= 1;
				--validbits;
			}

			// Increment the use count (frequency) of this node, and ensure the
			// tree remains sorted.
			uint16_t freqidx = NodeIdxMap.at(node);
			do {
				NodeFreq.at(freqidx) += 1;
				uint16_t freq = NodeFreq[freqidx];
				uint16_t nextidx = freqidx + 1;
				if (nextidx < NodeFreq.size() && NodeFreq[nextidx] < freq)
				{
					// Find the next frequency count that's not greater than the new frequency.
					do {
						++nextidx;
					} while (nextidx < NodeFreq.size() && NodeFreq[nextidx] < freq);
					--nextidx;

					// Swap 'em, placing the new frequency just before the next
					// greater one. Since the freq only incremented by 1, this
					// won't put it out of order.
					NodeFreq[freqidx] = NodeFreq[nextidx];
					NodeFreq[nextidx] = freq;

					std::iter_swap(NodeTree.begin() + freqidx, NodeTree.begin() + nextidx);

					// Update the index mappings
					uint16_t mapidx = NodeTree[nextidx];
					NodeIdxMap.at(mapidx) = nextidx;
					if (mapidx < 627)
						NodeIdxMap[mapidx + 1] = nextidx;

					mapidx = NodeTree[freqidx];
					NodeIdxMap.at(mapidx) = freqidx;
					if (mapidx < 627)
						NodeIdxMap[mapidx + 1] = freqidx;
					freqidx = nextidx;
				}
				// Recurse up the tree
				freqidx = NodeIdxMap[freqidx];
			} while (freqidx != 0);

			// Get the value from the node. If it's less than 256, it's a direct pixel value.
			uint16_t codeword = node - 627;
			if (codeword < 256)
			{
				uint8_t codewordByte = static_cast<uint8_t>(codeword);
				history[historypos++ & 0x0FFF] = codewordByte;
				*(dst++) = codewordByte;
			}
			else
			{
				// Otherwise, get the next 8 bits from input to construct the
				// offset to previous pixels to repeat, with the count being
				// derived from the node's value.
				while (validbits < 9)
				{
					if (src != srcend)
						bitmask |= *(src++) << (8 - validbits);
					validbits += 8;
				}
				uint8_t tableidx = bitmask >> 8;
				bitmask <<= 8;
				validbits -= 8;

				uint16_t offsetHigh = highOffsetBits[tableidx] << 6;
				uint16_t bitcount = lowOffsetBitCount[tableidx] - 2;
				uint16_t offsetLow = tableidx;
				for (uint16_t i = 0;i < bitcount;++i)
				{
					while (validbits < 9)
					{
						if (src != srcend)
							bitmask |= *(src++) << (8 - validbits);
						validbits += 8;
					}
					offsetLow = (offsetLow << 1) | ((bitmask >> 15) & 1);
					bitmask <<= 1;
					--validbits;
				}

				uint16_t copypos = historypos - (offsetHigh | (offsetLow & 0x003F)) - 1;
				uint16_t tocopy = codeword - 256 + 3;
				for (uint16_t i = 0;i < tocopy;++i)
				{
					*dst = history[copypos++ & 0x0FFF];
					history[historypos++ & 0x0FFF] = *(dst++);
				}
			}
		}
	}

	// These might be useful as public misc utility functions.

	uint16_t getLE16(const uint8_t *buf)
	{
		return buf[0] | (buf[1] << 8);
	}

	uint32_t getLE32(const uint8_t *buf)
	{
		return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	}

}

// This path should be removed once using original Arena files exclusively.
const std::string TextureManager::PATH = "data/textures/";

TextureManager::TextureManager(Renderer &renderer)
	: renderer(renderer)
{
	Debug::mention("Texture Manager", "Initializing.");

	this->palettes = std::map<PaletteName, Palette>();
	this->surfaces = std::unordered_map<std::string, std::map<PaletteName, Surface>>();
	this->textures = std::unordered_map<std::string, std::map<PaletteName, SDL_Texture*>>();
	this->surfaceSets = std::unordered_map<std::string, 
		std::map<PaletteName, std::vector<Surface>>>();
	this->textureSets = std::unordered_map<std::string,
		std::map<PaletteName, std::vector<SDL_Texture*>>>();

	// Load default palette.
	this->setPalette(PaletteName::Default);

	// Intialize PNG file loading.
	int imgFlags = IMG_INIT_PNG;
	Debug::check((IMG_Init(imgFlags) & imgFlags) != SDL_FALSE, "Texture Manager",
		"Couldn't initialize texture loader, " + std::string(IMG_GetError()));
}

TextureManager::~TextureManager()
{
	// Release the SDL_Textures.
	// The SDL_Renderer destroys these itself with SDL_DestroyRenderer(), too.
	for (auto &pair : this->textures)
	{
		for (auto &innerPair : pair.second)
		{
			SDL_DestroyTexture(innerPair.second);
		}
	}

	for (auto &pair : this->textureSets)
	{
		for (auto &innerPair : pair.second)
		{
			for (auto *texture : innerPair.second)
			{
				SDL_DestroyTexture(texture);
			}
		}
	}

	IMG_Quit();
}

TextureManager &TextureManager::operator=(TextureManager &&textureManager)
{
	this->palettes = std::move(textureManager.palettes);
	this->surfaces = std::move(textureManager.surfaces);
	this->textures = std::move(textureManager.textures);
	this->surfaceSets = std::move(textureManager.surfaceSets);
	this->textureSets = std::move(textureManager.textureSets);
	this->renderer = textureManager.renderer;
	this->activePalette = textureManager.activePalette;

	return *this;
}

std::vector<SDL_Surface*> TextureManager::loadCFA(const std::string &filename, 
	PaletteName paletteName)
{
	// Enemy sprite animations (goblin, orc, skeleton, ...), as well as
	// spell animations, are CFA.

	Debug::crash("Texture Manager", "loadCFA not implemented.");
	return std::vector<SDL_Surface*>();
}

std::vector<SDL_Surface*> TextureManager::loadCIF(const std::string &filename, 
	PaletteName paletteName)
{
	// CIF is for player weapon animations, some arrow cursors, and character heads.

	Debug::crash("Texture Manager", "loadCIF not implemented.");
	return std::vector<SDL_Surface*>();
}

std::vector<SDL_Surface*> TextureManager::loadDFA(const std::string &filename, 
	PaletteName paletteName)
{
	// Lots of sprite animations (bartender, tavern folk, volcanoes), and 
	// some torches, fountains, etc. are DFA.

	Debug::crash("Texture Manager", "loadDFA not implemented.");
	return std::vector<SDL_Surface*>();
}

std::vector<SDL_Surface*> TextureManager::loadFLC(const std::string &filename)
{
	// FLC and CEL files are movies. I'm pretty sure they're identical formats,
	// and they are related to GIF.

	Debug::crash("Texture Manager", "loadFLC not implemented.");
	return std::vector<SDL_Surface*>();
}

SDL_Surface *TextureManager::loadIMG(const std::string &filename, PaletteName paletteName)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());
	Debug::check(stream != nullptr, "Texture Manager",
		"Could not open texture \"" + filename + "\".");

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
			"Texture Manager", "Could not read texture \"" + filename + "\" header.");

		xoff = getLE16(imghdr.data());
		yoff = getLE16(imghdr.data() + 2);
		width = getLE16(imghdr.data() + 4);
		height = getLE16(imghdr.data() + 6);
		flags = getLE16(imghdr.data() + 8);
		srclen = getLE16(imghdr.data() + 10);
	}

	std::vector<uint8_t> srcdata(srclen);
	stream->read(reinterpret_cast<char*>(srcdata.data()), srcdata.size());

	// Commented this because wall textures are not in the "raw" list and do not have 
	// a header, therefore causing the byte count to not match 4096 bytes.
	/*Debug::check(stream->gcount() == static_cast<std::streamsize>(srcdata.size()),
		"Texture Manager", "Could not read texture \"" + filename + "\" data.");*/

	Palette custompal;
	bool hasBuiltInPalette = (flags & 0x0100) > 0;

	if (hasBuiltInPalette)
	{
		std::array<uint8_t, 768> rawpal;

		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		
		// Commented because some wall textures are incorrectly matching the flags & 0x0100.
		// Find a way to load wall textures without this problem!
		/*Debug::check(stream->gcount() == static_cast<std::streamsize>(rawpal.size()),
			"Texture Manager", "Could not read texture \"" + filename + "\" palette.");*/

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
		Debug::check(paletteName != PaletteName::BuiltIn, "Texture Manager",
			"File \"" + filename + "\" does not have a built-in palette.");
	}

	const Palette &paletteRef = (hasBuiltInPalette && (paletteName == PaletteName::BuiltIn)) ?
		custompal : this->palettes.at(paletteName);

	if ((flags & 0x00FF) == 0x0000)
	{
		// Uncompressed IMG.
		assert(srcdata.size() == (width * height));

		// Create temporary ARGB surface.
		SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height,
			Surface::DEFAULT_BPP, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		std::transform(srcdata.begin(), srcdata.end(), pixels,
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});

		auto *optSurface = SDL_ConvertSurface(surface, this->renderer.getFormat(), 0);
		SDL_FreeSurface(surface);

		return optSurface;
	}
	else if ((flags & 0x00FF) == 0x0004)
	{
		// Type 4 compression.
		std::vector<uint8_t> decomp(width * height);
		decode04Type(srcdata.begin(), srcdata.end(), decomp);

		// Create temporary ARGB surface.
		SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height,
			Surface::DEFAULT_BPP, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		std::transform(decomp.begin(), decomp.end(), pixels,
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});

		auto *optSurface = SDL_ConvertSurface(surface, this->renderer.getFormat(), 0);
		SDL_FreeSurface(surface);

		return optSurface;
	}
	else if ((flags & 0x00FF) == 0x0008)
	{
		uint16_t decomplen = getLE16(srcdata.data());
		assert(decomplen == (width * height));

		// Type 8 compression.
		std::vector<uint8_t> decomp(width * height);
		decode08Type(srcdata.begin() + 2, srcdata.end(), decomp);

		// Create temporary ARGB surface.
		SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height,
			Surface::DEFAULT_BPP, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		std::transform(decomp.begin(), decomp.end(), pixels,
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});

		auto *optSurface = SDL_ConvertSurface(surface, this->renderer.getFormat(), 0);
		SDL_FreeSurface(surface);

		return optSurface;
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
		// fall under the same flag condition.

		// There are no flags, so the header is essentially garbage (just whatever 
		// the texture's colors are).

		// There should be a "loadSET()" method. It would be easy to make. It just
		// involves either two or three (maybe four?) 64x64 wall textures packed 
		// together vertically.

		width = 64;
		height = 64;
		srclen = width * height;
		srcdata = std::vector<uint8_t>(srclen);

		// Re-read the file in one big 4096 byte chunk.
		// To do: use the original stream in this method.
		VFS::IStreamPtr myStream = VFS::Manager::get().open(filename.c_str());
		myStream->read(reinterpret_cast<char*>(srcdata.data()), srcdata.size());

		// Create temporary ARGB surface.
		SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height,
			Surface::DEFAULT_BPP, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

		uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
		std::transform(srcdata.begin(), srcdata.end(), pixels,
			[&paletteRef](uint8_t col) -> uint32_t
		{
			return paletteRef[col].toARGB();
		});

		auto *optSurface = SDL_ConvertSurface(surface, this->renderer.getFormat(), 0);
		SDL_FreeSurface(surface);

		return optSurface;
	}
}

SDL_Surface *TextureManager::loadPNG(const std::string &fullPath)
{
	// Load the SDL_Surface from file.
	auto *unOptSurface = IMG_Load(fullPath.c_str());
	Debug::check(unOptSurface != nullptr, "Texture Manager",
		"Could not open texture \"" + fullPath + "\".");

	// Try to optimize the SDL_Surface.
	auto *optSurface = SDL_ConvertSurface(unOptSurface, this->renderer.getFormat(), 0);
	SDL_FreeSurface(unOptSurface);
	Debug::check(optSurface != nullptr, "Texture Manager",
		"Could not optimize texture \"" + fullPath + "\".");

	return optSurface;
}

std::vector<SDL_Surface*> TextureManager::loadSET(const std::string &filename, 
	PaletteName paletteName)
{
	// A SET is just some IMGs packed together vertically, so split them here
	// and return them separately in a vector. They're basically all uncompressed
	// wall textures grouped by type.

	// The height of the image should be a multiple of 64, so divide by 64 to get
	// the number of IMGs in the SET. Alternatively, since all wall textures are
	// 4096 bytes, just divide the SET byte size by 4096 for the IMG count.

	Debug::crash("Texture Manager", "loadSET not implemented.");
	return std::vector<SDL_Surface*>();
}

void TextureManager::initPalette(Palette &palette, PaletteName paletteName)
{
	bool failed = false;
	std::array<uint8_t, 776> rawpal;
	const std::string &filename = PaletteFilenames.at(paletteName);
	VFS::IStreamPtr stream = VFS::Manager::get().open(filename.c_str());

	if (!stream)
	{
		Debug::mention("Texture Manager",
			"Failed to open palette \"" + filename + "\".");
		failed = true;
	}
	else
	{
		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		if (stream->gcount() != static_cast<std::streamsize>(rawpal.size()))
		{
			Debug::mention("Texture Manager", "Failed to read palette \"" +
				filename + "\", got " + std::to_string(stream->gcount()) + " bytes.");
			failed = true;
		}
	}
	if (!failed)
	{
		uint32_t len = getLE32(rawpal.data());
		uint32_t ver = getLE32(rawpal.data() + 4);
		if (len != 776)
		{
			Debug::mention("Texture Manager", "Invalid length for palette \"" +
				filename + "\" (" + std::to_string(len) + " bytes).");
			failed = true;
		}
		else if (ver != 0xB123)
		{
			Debug::mention("Texture Manager", "Invalid version for palette \"" +
				filename + "\", 0x" + String::toHexString(ver) + ".");
			failed = true;
		}
	}
	if (!failed)
	{
		auto iter = rawpal.begin() + 8;

		/* First palette entry is transparent in 8-bit modes, so give it 0 alpha. */
		uint8_t r = *(iter++);
		uint8_t g = *(iter++);
		uint8_t b = *(iter++);
		palette[0] = Color(r, g, b, 0);

		/* Remaining are solid, so give them 255 alpha. */
		std::generate(palette.begin() + 1, palette.end(),
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
		std::generate(palette.begin() + 1, palette.end(),
			[&count]() -> Color
		{
			uint8_t c = ++count;
			return Color(c, c, c, 255);
		});
	}
}

const Surface &TextureManager::getSurface(const std::string &filename,
	PaletteName paletteName)
{
	// See if the image file already exists with any palette. Otherwise,
	// decide how to load it further down.
	auto surfaceIter = this->surfaces.find(filename);
	if (surfaceIter != this->surfaces.end())
	{
		// Now see if the image exists with the requested palette.
		const auto &paletteMap = surfaceIter->second;
		auto paletteIter = paletteMap.find(paletteName);

		if (paletteIter != paletteMap.end())
		{
			// The requested surface exists.
			return paletteIter->second;
		}
	}
	else
	{
		// The image hasn't been loaded with any palettes yet, so make a new entry.
		surfaceIter = this->surfaces.emplace(std::make_pair(
			filename, std::map<PaletteName, Surface>())).first;
	}

	// Check what kind of file extension is used. Every texture should have an
	// extension, so the "dot position" might be unnecessary once PNGs are no
	// longer used.
	size_t dotPos = filename.rfind('.');
	bool hasDot = (dotPos < filename.length()) && (dotPos != std::string::npos);
	bool isIMG = hasDot &&
		(filename.compare(dotPos, filename.length() - dotPos, ".IMG") == 0);
	bool isMNU = hasDot &&
		(filename.compare(dotPos, filename.length() - dotPos, ".MNU") == 0);

	SDL_Surface *optSurface = nullptr;

	if (isIMG || isMNU)
	{
		optSurface = this->loadIMG(filename, paletteName);
	}
	else
	{
		// PNG file. This "else" case should eventually throw a runtime error instead
		// once PNGs aren't used anymore.
		std::string fullPath(TextureManager::PATH + filename + ".png");
		optSurface = this->loadPNG(fullPath);
	}

	// Create surface from optimized SDL_Surface.
	Surface surface(optSurface);
	SDL_FreeSurface(optSurface);

	// Add the new surface and return it.
	auto &paletteMap = surfaceIter->second;	
	auto iter = paletteMap.emplace(std::make_pair(paletteName, surface)).first;
	return iter->second;
}

const Surface &TextureManager::getSurface(const std::string &filename)
{
	return this->getSurface(filename, this->activePalette);
}

SDL_Texture *TextureManager::getTexture(const std::string &filename,
	PaletteName paletteName)
{
	// See if the image file already exists with any palette. Otherwise,
	// decide how to load it further down.
	auto textureIter = this->textures.find(filename);
	if (textureIter != this->textures.end())
	{
		// Now see if the image exists with the requested palette.
		const auto &paletteMap = textureIter->second;
		auto paletteIter = paletteMap.find(paletteName);

		if (paletteIter != paletteMap.end())
		{
			// The requested texture exists.
			return paletteIter->second;
		}
	}
	else
	{
		// The image hasn't been loaded with any palettes yet, so make a new entry.
		textureIter = this->textures.emplace(std::make_pair(
			filename, std::map<PaletteName, SDL_Texture*>())).first;
	}

	// Make a texture from the surface. It's okay if the surface isn't used except
	// for, say, texture dimensions (instead of doing SDL_QueryTexture()).
	const Surface &surface = this->getSurface(filename, paletteName);		
	SDL_Texture *texture = this->renderer.createTextureFromSurface(surface);
		
	// Add the new texture and return it.
	auto &paletteMap = textureIter->second;
	auto iter = paletteMap.emplace(std::make_pair(paletteName, texture)).first;
	return iter->second;	
}

SDL_Texture *TextureManager::getTexture(const std::string &filename)
{
	return this->getTexture(filename, this->activePalette);
}

const std::vector<Surface> &TextureManager::getSurfaces(const std::string &filename, 
	PaletteName paletteName)
{
	// I would like this method to deal with the animations and movies, so it'll 
	// check filenames for ".CFA", ".CIF", ".DFA", ".FLC", etc..

	Debug::crash("Texture Manager", "getSurfaces() not implemented.");
	return this->surfaceSets.at("").at(PaletteName::Default); // Dummy placeholder.
}

const std::vector<Surface> &TextureManager::getSurfaces(const std::string &filename)
{
	return this->getSurfaces(filename, this->activePalette);
}

const std::vector<SDL_Texture*> &TextureManager::getTextures(const std::string &filename, 
	PaletteName paletteName)
{
	// This method will just take the surfaces from getSurfaces() and turn them into 
	// SDL_Textures if not already loaded. I'm not too worried about memory consumption 
	// and unused surfaces at this point. A fullscreen (320x200) uncompressed 32-bit 
	// image is only 256KB, and with all the movies and animations combined, that's 
	// like... 450MB? I think an "unloadTexture()" method is a bit too far ahead right now.

	Debug::crash("Texture Manager", "getTextures() not implemented.");
	return this->textureSets.at("").at(PaletteName::Default); // Dummy placeholder.
}

const std::vector<SDL_Texture*> &TextureManager::getTextures(const std::string &filename)
{
	return this->getTextures(filename, this->activePalette);
}

void TextureManager::setPalette(PaletteName paletteName)
{
	// Error if the palette name is "built-in".
	assert(paletteName != PaletteName::BuiltIn);

	this->activePalette = paletteName;

	if (this->palettes.find(paletteName) == this->palettes.end())
	{
		Palette palette;
		this->initPalette(palette, paletteName);
		this->palettes.emplace(std::make_pair(paletteName, palette));
	}
}

void TextureManager::preloadSequences()
{
	Debug::mention("Texture Manager", "Preloading sequences.");

	for (const auto name : TextureFile::getSequenceNames())
	{
		std::vector<std::string> filenames = TextureFile::fromName(name);
		for (const auto &filename : filenames)
		{
			this->getTexture(filename);
		}
	}
}
