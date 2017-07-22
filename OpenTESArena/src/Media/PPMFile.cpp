#include <algorithm>
#include <fstream>

#include "PPMFile.h"

#include "../Utilities/Debug.h"
#include "../Utilities/File.h"
#include "../Utilities/String.h"

std::unique_ptr<uint32_t[]> PPMFile::read(const std::string &filename,
	int &width, int &height)
{
	const std::string text = File::readAllText(filename);
	const std::vector<std::string> lines = String::split(text, '\n');

	// Make sure the PPM type is P3 (ASCII format).
	std::string ppmType = String::trimLines(lines.at(0));
	DebugAssert(ppmType.compare("P3") == 0, "Unrecognized PPM type \"" + ppmType + "\".");

	// Skip the comment at index 1.

	// Get the width and height of the PPM image.
	const std::string &dimensions = lines.at(2);
	const std::vector<std::string> dimensionTokens = String::split(dimensions);
	width = std::atoi(dimensionTokens.at(0).c_str());
	height = std::atoi(dimensionTokens.at(1).c_str());

	// Get the max color intensity (probably just 255).
	const std::string &maxColor = lines.at(3);
	const int maxColorValue = std::atoi(maxColor.c_str());
	DebugAssert(maxColorValue == 255, "Unusual color max \"" +
		std::to_string(maxColorValue) + "\".");

	// Start reading RGB components for each pixel.
	std::unique_ptr<uint32_t[]> pixels(new uint32_t[width * height]);
	std::fill(pixels.get(), pixels.get() + (width * height), 0);

	for (int y = 0; y < height; ++y)
	{
		// A component here is an ASCII value between 0-255.
		const std::vector<std::string> components = String::split(lines.at(4 + y));
		const int componentCount = static_cast<int>(components.size());

		// Verify that the length is a multiple of three (for R, G, and B).
		DebugAssert((componentCount % 3) == 0, "Incorrect component count at line " +
			std::to_string(y) + " (" + std::to_string(components.size()) +
			"), should be multiple of 3.");

		uint32_t *pixelsPtr = pixels.get();
		for (int x = 0; x < (componentCount - 2); x += 3)
		{
			const uint8_t a = 255;
			const uint8_t r = std::atoi(components.at(x).c_str());
			const uint8_t g = std::atoi(components.at(x + 1).c_str());
			const uint8_t b = std::atoi(components.at(x + 2).c_str());

			// Write the pixel to the buffer in 0xAARRGGBB format.
			pixelsPtr[(x / 3) + (y * width)] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	}

	return pixels;
}

void PPMFile::write(const uint32_t *pixels, int width, int height,
	const std::string &comment, const std::string &filename)
{
	std::ofstream ofs(filename);
	const int uchar_max = 255;

	// Write PPM header.
	ofs << "P3" << "\n" << ("# " + comment) << "\n" << std::to_string(width) << " " <<
		std::to_string(height) << "\n" << std::to_string(uchar_max) << "\n";

	// Write color data out to file.
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// Assume 0x00RRGGBB color format.
			const uint32_t color = pixels[x + (y * width)];
			const uint8_t r = static_cast<uint8_t>(color >> 16);
			const uint8_t g = static_cast<uint8_t>(color >> 8);
			const uint8_t b = static_cast<uint8_t>(color);

			ofs << std::to_string(r) << " " << std::to_string(g) << " " <<
				std::to_string(b) << " ";
		}

		ofs << "\n";
	}

	ofs.close();
}
