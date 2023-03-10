#ifndef VOC_FILE_H
#define VOC_FILE_H

#include <cstdint>
#include <vector>

#include "components/utilities/BufferView.h"

// A .VOC file contains audio data in the Creative Voice format. It's used with any 
// sounds in Arena, and in the CD version it's also used with voices in cinematics.

// All of Arena's .VOC files are mono 8-bit unsigned PCM.

class VOCFile
{
private:
	std::vector<uint8_t> audioData;
	int sampleRate;
public:
	bool init(const char *filename);

	// Gets the sample rate of the .VOC file (usually between 4000 and 11111).
	int getSampleRate() const;

	// Gets the 8-bit unsigned PCM audio data.
	BufferView<const uint8_t> getAudioData() const;
};

#endif
