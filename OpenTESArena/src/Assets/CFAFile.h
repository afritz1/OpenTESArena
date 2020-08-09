#ifndef CFA_FILE_H
#define CFA_FILE_H

#include <cstdint>

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"

// A .CFA file is for creatures and spell animations.

class CFAFile
{
private:
	Buffer<Buffer2D<uint8_t>> images;
	int width, height, xOffset, yOffset;

	// CFA files have their palette indices compressed into fewer bits depending
	// on the total number of colors in the file. These demuxing functions
	// uncompress those bits into bytes. Adapted from WinArena.
	static void demux1(const uint8_t *src, uint8_t *dst);
	static void demux2(const uint8_t *src, uint8_t *dst);
	static void demux3(const uint8_t *src, uint8_t *dst);
	static void demux4(const uint8_t *src, uint8_t *dst);
	static void demux5(const uint8_t *src, uint8_t *dst);
	static void demux6(const uint8_t *src, uint8_t *dst);
	static void demux7(const uint8_t *src, uint8_t *dst);
public:
	bool init(const char *filename);

	// Gets the number of images.
	int getImageCount() const;

	// Gets the width of all images.
	int getWidth() const;

	// Gets the height of all images.
	int getHeight() const;

	// Gets the X offset of all images.
	int getXOffset() const;

	// Gets the Y offset of all images.
	int getYOffset() const;

	// Gets a pointer to an image's 8-bit pixels.
	const uint8_t *getPixels(int index) const;
};

#endif
