#ifndef FLC_FILE_H
#define FLC_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Media/Palette.h"

// An FLC file is a video file. The CEL files can be used with this class since they 
// are identical to FLC, though with an extra chunk of header data. As a baseline, 
// the file should be converted to a vector of independent frames (in case it uses 
// a delta format for frame differences).

// I'm fairly certain now after looking into it, that the Arena developers used the
// "Autodesk Animator" program to make these FLC and CEL animations.

// Some interesting trivia I found in some FLC files:
// - END02.FLC was initially created on Friday, Oct. 15th, 1993, and last updated
//   on the Wednesday after that.
// - KING.FLC was initially created on Tuesday, Oct. 19th, 1993.
// - VISION.FLC was initially created a month before that, on Monday, Sept. 13th 1993.

// This website has some good information on the FLIC format:
// - http://www.compuphase.com/flic.htm (fortunately it's fairly well documented)

class FLCFile
{
private:
	// One unique_ptr for each frame.
	std::vector<std::unique_ptr<uint32_t>> pixels;
	int width;
	int height;

	void readPaletteData(const uint8_t *data, Palette &dstPalette);
public:
	FLCFile(const std::string &filename);
	~FLCFile();

	// Gets the number of frames in the FLC file.
	int getFrameCount() const;

	// Gets the width of each frame in the FLC file.
	int getWidth() const;

	// Gets the height of each frame in the FLC file.
	int getHeight() const;

	// Gets the pixel data for a frame in the FLC file.
	uint32_t *getPixels(int index) const;
};

#endif
