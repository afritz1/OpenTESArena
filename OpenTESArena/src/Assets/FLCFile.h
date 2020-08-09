#ifndef FLC_FILE_H
#define FLC_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Media/Palette.h"

#include "components/utilities/Buffer2D.h"

// An .FLC file is a video file. .CEL files are nearly identical to .FLCs, though with 
// an extra chunk of header data (which can probably be skipped).

// I'm fairly certain now after looking into it, that the Arena developers used
// Autodesk Animator to make these .FLC and .CEL animations.

// Some interesting trivia I found in some .FLC files:
// - END02.FLC was initially created on Friday, Oct. 15th, 1993, and last updated
//   on the Wednesday after that.
// - KING.FLC was initially created on Tuesday, Oct. 19th, 1993.
// - VISION.FLC was initially created a month before that, on Monday, Sept. 13th 1993.

// These websites have some information on the FLIC format:
// - http://www.compuphase.com/flic.htm
// - http://www.fileformat.info/format/fli/egff.htm

class FLCFile
{
private:
	// One buffer for each frame. Each integer points into that frame's palette.
	std::vector<std::pair<int, Buffer2D<uint8_t>>> images;
	std::vector<Palette> palettes;
	double frameDuration;
	int width;
	int height;

	// Reads a palette chunk and writes out the results to the reference parameter.
	static bool readPalette(const uint8_t *chunkData, Palette *dst);

	// Decodes a fullscreen FLC chunk by updating the initial frame indices and
	// returning a complete frame.
	Buffer2D<uint8_t> decodeFullFrame(const uint8_t *chunkData, int chunkSize,
		Buffer2D<uint8_t> &initialFrame);

	// Decodes a delta FLC chunk by partially updating the initial frame indices and
	// returning a complete frame.
	Buffer2D<uint8_t> decodeDeltaFrame(const uint8_t *chunkData, int chunkSize,
		Buffer2D<uint8_t> &initialFrame);
public:
	bool init(const char *filename);

	// Gets the number of frames.
	int getFrameCount() const;

	// Gets the duration of each frame in seconds.
	double getFrameDuration() const;

	// Gets the width of each frame.
	int getWidth() const;

	// Gets the height of each frame.
	int getHeight() const;

	// Gets the palette associated with the given frame index.
	const Palette &getFramePalette(int index) const;

	// Gets the pixel data for some frame.
	const uint8_t *getPixels(int index) const;
};

#endif
