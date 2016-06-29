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
#include "TextureFile.h"
#include "../Interface/Surface.h"
#include "../Math/Int2.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

#include "components/vfs/manager.hpp"

namespace
{
	// The format extension for all tool-extracted textures is PNG. Remove this
	// once the program is able to load original Arena assets exclusively.
	const std::string TextureExtension = ".png";

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
        static const std::array<uint8_t,256> highOffsetBits{
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
        static const std::array<uint8_t,256> lowOffsetBitCount{
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

        std::array<uint16_t,941> NodeIdxMap;
        std::iota(NodeIdxMap.begin(), NodeIdxMap.begin()+626, 0);
        std::for_each(NodeIdxMap.begin(), NodeIdxMap.begin()+626,
            [](uint16_t &val) { val = (val>>1) + 314; }
        );
        NodeIdxMap[626] = 0;
        std::iota(NodeIdxMap.begin()+627, NodeIdxMap.end(), 0);

        std::array<uint16_t,627> NodeTree;
        std::iota(NodeTree.begin(), NodeTree.begin()+314, 627);
        std::iota(NodeTree.begin()+314, NodeTree.end(), 0);
        std::for_each(NodeTree.begin()+314, NodeTree.end(),
            [](uint16_t &val) { val *= 2; }
        );

        std::array<uint16_t,627> NodeFreq;
        std::fill(NodeFreq.begin(), NodeFreq.begin()+314, 1);
        {
            auto iter = NodeFreq.begin();
            std::for_each(NodeFreq.begin()+314, NodeFreq.begin()+627,
                [&iter](uint16_t &val)
                {
                    val  = *(iter++);
                    val += *(iter++);
                }
            );
        }

        uint16_t bitmask = 0;
        uint8_t validbits = 0;

        // This feels like some form of adaptive Huffman coding, with a form of LZ
        // compression. DEFLATE?
        auto dst = out.begin();
        while(dst != out.end())
        {
            // Starting with the root, append bits from the input while traversing
            // the tree until a leaf node is found (indicated by being >=627)
            uint16_t node = NodeTree[626];
            while(node < 627)
            {
                while(validbits < 9)
                {
                    if(src != srcend)
                        bitmask |= *(src++) << (8-validbits);
                    validbits += 8;
                }
                node = NodeTree.at(node + ((bitmask>>15)&1));
                bitmask <<= 1;
                --validbits;
            }

            // Increment the use count (frequency) of this node, and ensure the
            // tree remains sorted
            uint16_t freqidx = NodeIdxMap.at(node);
            do {
                NodeFreq.at(freqidx) += 1;
                uint16_t freq = NodeFreq[freqidx];
                uint16_t nextidx = freqidx + 1;
                if(nextidx < NodeFreq.size() && NodeFreq[nextidx] < freq)
                {
                    // Find the next frequency count that's not greater than the new frequency
                    do {
                        ++nextidx;
                    } while(nextidx < NodeFreq.size() && NodeFreq[nextidx] < freq);
                    --nextidx;

                    // Swap 'em, placing the new frequency just before the next
                    // greater one. Since the freq only incremented by 1, this
                    // won't put it out of order.
                    NodeFreq[freqidx] = NodeFreq[nextidx];
                    NodeFreq[nextidx] = freq;

                    std::iter_swap(NodeTree.begin()+freqidx, NodeTree.begin()+nextidx);

                    // Update the index mappings
                    uint16_t mapidx = NodeTree[nextidx];
                    NodeIdxMap.at(mapidx) = nextidx;
                    if(mapidx < 627)
                        NodeIdxMap[mapidx + 1] = nextidx;

                    mapidx = NodeTree[freqidx];
                    NodeIdxMap.at(mapidx) = freqidx;
                    if(mapidx < 627)
                        NodeIdxMap[mapidx + 1] = freqidx;
                    freqidx = nextidx;
                }
                // Recurse up the tree
                freqidx = NodeIdxMap[freqidx];
            } while(freqidx != 0);

            // Get the value from the node. If it's less than 256, it's a direct pixel value.
            uint16_t codeword = node - 627;
            if(codeword < 256)
            {
                history[historypos++ & 0x0FFF] = codeword;
                *(dst++) = codeword;
            }
            else
            {
                // Otherwise, get the next 8 bits from input to construct the
                // offset to previous pixels to repeat, with the count being
                // derived from the node's value.
                while(validbits < 9)
                {
                    if(src != srcend)
                        bitmask |= *(src++) << (8-validbits);
                    validbits += 8;
                }
                uint8_t tableidx = bitmask >> 8;
                bitmask <<= 8;
                validbits -= 8;

                uint16_t offsetHigh = highOffsetBits[tableidx] << 6;
                uint16_t bitcount = lowOffsetBitCount[tableidx] - 2;
                uint16_t offsetLow = tableidx;
                for(uint16_t i = 0;i < bitcount;++i)
                {
                    while(validbits < 9)
                    {
                        if(src != srcend)
                            bitmask |= *(src++) << (8-validbits);
                        validbits += 8;
                    }
                    offsetLow = (offsetLow<<1) | ((bitmask>>15)&1);
                    bitmask <<= 1;
                    --validbits;
                }

                uint16_t copypos = historypos - (offsetHigh | (offsetLow&0x003F)) - 1;
                uint16_t tocopy = codeword-256 + 3;
                for(uint16_t i = 0;i < tocopy;++i)
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

// This path might be obsolete soon.
const std::string TextureManager::PATH = "data/textures/";

TextureManager::TextureManager(const SDL_Renderer *renderer, const SDL_PixelFormat *format)
{
	Debug::mention("Texture Manager", "Initializing.");

	assert(format != nullptr);

	this->surfaces = std::map<std::string, Surface>();
	this->textures = std::map<std::string, SDL_Texture*>();
	this->renderer = renderer;
	this->format = format;

	// Load default palette.
	this->setPalette("PAL.COL");

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
		SDL_DestroyTexture(pair.second);
	}

	IMG_Quit();
}

SDL_Surface *TextureManager::loadPNG(const std::string &fullPath)
{
	// Load the SDL_Surface from file.
	auto *unOptSurface = IMG_Load(fullPath.c_str());
	Debug::check(unOptSurface != nullptr, "Texture Manager",
		"Could not open texture \"" + fullPath + "\".");

	// Try to optimize the SDL_Surface.
	auto *optSurface = SDL_ConvertSurface(unOptSurface, this->format, 0);
	SDL_FreeSurface(unOptSurface);
	Debug::check(optSurface != nullptr, "Texture Manager",
		"Could not optimize texture \"" + fullPath + "\".");

	return optSurface;
}

SDL_Surface *TextureManager::loadIMG(const std::string &fullPath)
{
	VFS::IStreamPtr stream = VFS::Manager::get().open(fullPath.c_str());
	Debug::check(stream != nullptr, "Texture Manager",
		"Could not open texture \"" + fullPath + "\".");

    uint16_t xoff, yoff, width, height, flags, srclen;

    auto rawoverride = RawImgOverride.find(fullPath);
    if(rawoverride != RawImgOverride.end())
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
            "Texture Manager", "Could not read texture \"" + fullPath + "\" header.");

        xoff = getLE16(imghdr.data());
        yoff = getLE16(imghdr.data() + 2);
        width = getLE16(imghdr.data() + 4);
        height = getLE16(imghdr.data() + 6);
        flags = getLE16(imghdr.data() + 8);
        srclen = getLE16(imghdr.data() + 10);
    }

	std::vector<uint8_t> srcdata(srclen);
	stream->read(reinterpret_cast<char*>(srcdata.data()), srcdata.size());
	Debug::check(stream->gcount() == static_cast<std::streamsize>(srcdata.size()),
		"Texture Manager", "Could not read texture \"" + fullPath + "\" data.");

	Palette custompal;
	if (flags & 0x0100)
	{
		std::array<uint8_t, 768> rawpal;

		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		Debug::check(stream->gcount() == static_cast<std::streamsize>(rawpal.size()),
			"Texture Manager", "Could not read texture \"" + fullPath + "\" palette.");

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

	const Palette &paletteRef = (flags & 0x0100) ? custompal : this->palette;
	if ((flags & 0x00ff) == 0x0000)
	{
		// Uncompressed IMG.
		assert(srcdata.size() == width * height);

		// Create temporary ARGB surface.
		SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 
			Surface::DEFAULT_BPP, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
		if (SDL_LockSurface(surface) == 0)
		{
			uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
			std::transform(srcdata.begin(), srcdata.end(), pixels,
				[&paletteRef](uint8_t col) -> uint32_t
			{
				return paletteRef[col].toARGB();
			});
			SDL_UnlockSurface(surface);
		}

		auto *optSurface = SDL_ConvertSurface(surface, this->format, 0);
		SDL_FreeSurface(surface);

		return optSurface;
	}
	if ((flags & 0x00ff) == 0x0004)
	{
		// Type 4 compression.
		std::vector<uint8_t> decomp(width * height);
		decode04Type(srcdata.begin(), srcdata.end(), decomp);

		// Create temporary ARGB surface
		SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height, 
			Surface::DEFAULT_BPP, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
		if (SDL_LockSurface(surface) == 0)
		{
			uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
			std::transform(decomp.begin(), decomp.end(), pixels,
				[&paletteRef](uint8_t col) -> uint32_t
			{
				return paletteRef[col].toARGB();
			});
			SDL_UnlockSurface(surface);
		}

		auto *optSurface = SDL_ConvertSurface(surface, this->format, 0);
		SDL_FreeSurface(surface);

		return optSurface;
	}
    if ((flags & 0x00ff) == 0x0008)
    {
        uint16_t decomplen = getLE16(srcdata.data());
        assert(decomplen == width*height);

        // Type 8 compression.
        std::vector<uint8_t> decomp(width * height);
        decode08Type(srcdata.begin()+2, srcdata.end(), decomp);

        // Create temporary ARGB surface
        SDL_Surface *surface = SDL_CreateRGBSurface(0, width, height,
            Surface::DEFAULT_BPP, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
        if (SDL_LockSurface(surface) == 0)
        {
            uint32_t *pixels = static_cast<uint32_t*>(surface->pixels);
            std::transform(decomp.begin(), decomp.end(), pixels,
                [&paletteRef](uint8_t col) -> uint32_t
            {
                return paletteRef[col].toARGB();
            }
            );
            SDL_UnlockSurface(surface);
        }

        auto *optSurface = SDL_ConvertSurface(surface, this->format, 0);
        SDL_FreeSurface(surface);

        return optSurface;
    }

	Debug::crash("Texture Manager", "Unhandled IMG flags, 0x" + String::toHexString(flags) + ".");
	return nullptr;
}

const SDL_PixelFormat *TextureManager::getFormat() const
{
	return this->format;
}

const Surface &TextureManager::getSurface(const std::string &filename)
{
    auto iter = this->surfaces.find(filename);
	if (iter != this->surfaces.end())
	{
		// Get the existing surface.
		return iter->second;
	}

	size_t dot = filename.rfind('.');
	bool isIMG = (dot < filename.length()) && 
		filename.compare(dot, filename.length() - dot, ".IMG") == 0;
	bool isMNU = (dot < filename.length()) && 
		filename.compare(dot, filename.length() - dot, ".MNU") == 0;
	if ((dot != std::string::npos) && (isIMG || isMNU))
	{
		auto *optSurface = this->loadIMG(filename);

		// Create surface from SDL_Surface. No need to optimize it again.
		Surface surface(optSurface);
		
		// Add the new surface and return it.
		iter = this->surfaces.insert(std::make_pair(filename, surface)).first;
		SDL_FreeSurface(optSurface);
		return iter->second;
	}
	else
	{
		// Load optimized SDL_Surface from file.
		std::string fullPath(TextureManager::PATH + filename + TextureExtension);
		auto *optSurface = this->loadPNG(fullPath);

        // Create surface from SDL_Surface. No need to optimize it again.
        Surface surface(optSurface);

		// Add the new surface and return it.
		auto iter = this->surfaces.insert(std::make_pair(filename, surface)).first;
		SDL_FreeSurface(optSurface);
		return iter->second;
	}
}

const SDL_Texture *TextureManager::getTexture(const std::string &filename)
{
	if (this->textures.find(filename) != this->textures.end())
	{
		const SDL_Texture *texture = this->textures.at(filename);
		return texture;
	}
	else
	{
		// Make a texture from the surface. It's okay if the surface isn't used except
		// for, say, texture dimensions (instead of doing SDL_QueryTexture()).
		const Surface &surface = this->getSurface(filename);
		SDL_Texture *texture = SDL_CreateTextureFromSurface(
			const_cast<SDL_Renderer*>(this->renderer), surface.getSurface());

		// Add the new texture and return it.
		auto iter = this->textures.insert(std::make_pair(filename, texture)).first;
		return iter->second;
	}	
}

void TextureManager::setPalette(const std::string &paletteName)
{
	bool failed = false;
	std::array<uint8_t, 776> rawpal;
	VFS::IStreamPtr stream = VFS::Manager::get().open(paletteName.c_str());
	if (!stream)
	{
		Debug::mention("Texture Manager", "Failed to open palette \"" + paletteName + "\".");
		failed = true;
	}
	else
	{
		stream->read(reinterpret_cast<char*>(rawpal.data()), rawpal.size());
		if (stream->gcount() != static_cast<std::streamsize>(rawpal.size()))
		{
			Debug::mention("Texture Manager", "Failed to read palette \"" + paletteName + 
				"\", got " + std::to_string(stream->gcount()) + " bytes.");
			failed = true;
		}
	}
	if (!failed)
	{
		uint32_t len = getLE32(rawpal.data());
		uint32_t ver = getLE32(rawpal.data() + 4);
		if (len != 776)
		{
			Debug::mention("Texture Manager", "Invalid length for palette \"" + paletteName +
				"\" (" + std::to_string(len) + " bytes).");
			failed = true;
		}
		else if (ver != 0xb123)
		{
			Debug::mention("Texture Manager", "Invalid version for palette \"" + paletteName +
				"\", 0x" + String::toHexString(ver) + ".");
			failed = true;
		}
	}
	if (!failed)
	{
		auto iter = rawpal.begin() + 8;

		/* First palette entry is transparent in 8-bit modes, so give it 0
		* alpha. */
		uint8_t r = *(iter++);
		uint8_t g = *(iter++);
		uint8_t b = *(iter++);
		this->palette[0] = Color(r, g, b, 0);

		/* Remaining are solid, so give them 255 alpha. */
		std::generate(this->palette.begin() + 1, this->palette.end(),
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
		std::generate(this->palette.begin() + 1, this->palette.end(),
			[&count]() -> Color
		{
			uint8_t c = ++count;
			return Color(c, c, c, 255);
		});
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
			this->getSurface(filename);
		}
	}
}

void TextureManager::reloadTextures(SDL_Renderer *renderer)
{
	// This assignment isn't completely necessary. The renderer shouldn't need to 
	// be destroyed and reinitialized on window resize events.
	this->renderer = renderer;

	for (auto &pair : this->textures)
	{
		SDL_DestroyTexture(pair.second);
		const Surface &surface = this->getSurface(pair.first);
		this->textures.at(pair.first) = SDL_CreateTextureFromSurface(
			renderer, surface.getSurface());
	}
}
