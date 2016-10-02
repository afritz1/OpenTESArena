#ifndef PPM_FILE_H
#define PPM_FILE_H

#include <cstdint>
#include <memory>
#include <string>

// Simple static class for reading and writing type P3 PPM image files.

// PPM is an easy, uncompressed image format.

class PPMFile
{
private:
	PPMFile() = delete;
	PPMFile(const PPMFile&) = delete;
	~PPMFile() = delete;
public:
	// Reads a .ppm file.
	static std::unique_ptr<uint32_t> read(const std::string &filename,
		int &width, int &height);

	// Writes a new .ppm file.
	static void write(const uint32_t *pixels, int width, int height,
		const std::string &comment, const std::string &filename);
};

#endif
