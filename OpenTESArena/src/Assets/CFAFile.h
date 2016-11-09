#ifndef CFA_FILE_H
#define CFA_FILE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../Media/Palette.h"

// A CFA file is for creatures and spell animations.

class CFAFile
{
private:
	std::vector<std::unique_ptr<uint32_t>> pixels;
	int width, height;

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
	CFAFile(const std::string &filename, const Palette &palette);
	~CFAFile();

	// Gets the number of images in the CFA file.
	int getImageCount() const;

	// Gets the width of an image in the CFA file.
	int getWidth() const;

	// Gets the height of an image in the CFA file.
	int getHeight() const;

	// Gets a pointer to the pixels for an image in the CFA file.
	uint32_t *getPixels(int index) const;
};

#endif
