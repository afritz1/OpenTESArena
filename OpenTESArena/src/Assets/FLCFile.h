#ifndef FLC_FILE_H
#define FLC_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Media/Palette.h"

// An FLC file is a video file. CEL files are nearly identical to FLCs, though with 
// an extra chunk of header data (which can probably be skipped).

// I'm fairly certain now after looking into it, that the Arena developers used the
// "Autodesk Animator" program to make these FLC and CEL animations.

// Some interesting trivia I found in some FLC files:
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
	// One unique_ptr for each frame.
	std::vector<std::unique_ptr<uint32_t>> pixels;
	double frameDuration;
	int width;
	int height;

	// Reads a palette chunk and writes the results to the given palette reference.
	void readPaletteData(const uint8_t *chunkData, Palette &dstPalette);

	// Decodes a fullscreen FLC chunk by updating the initial frame indices and
	// returning a complete frame.
	std::unique_ptr<uint32_t> decodeFullFrame(const uint8_t *chunkData, int chunkSize,
		const Palette &palette, std::vector<uint8_t> &initialFrame);

	// Decodes a delta FLC chunk by partially updating the initial frame indices and
	// returning a complete frame.
	std::unique_ptr<uint32_t> decodeDeltaFrame(const uint8_t *chunkData, int chunkSize,
		const Palette &palette, std::vector<uint8_t> &initialFrame);
public:
	FLCFile(const std::string &filename);
	~FLCFile();

	// Gets the number of frames in the FLC file.
	int getFrameCount() const;

	// Gets the duration of each frame in seconds in the FLC file.
	double getFrameDuration() const;

	// Gets the width of each frame in the FLC file.
	int getWidth() const;

	// Gets the height of each frame in the FLC file.
	int getHeight() const;

	// Gets the pixel data for a frame in the FLC file.
	uint32_t *getPixels(int index) const;
};

#endif
