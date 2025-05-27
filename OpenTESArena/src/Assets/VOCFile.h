#ifndef VOC_FILE_H
#define VOC_FILE_H

#include <cstdint>
#include <vector>

#include "components/utilities/BufferView.h"

// Creative Voice audio file. All sounds and voices in Arena are mono 8-bit unsigned PCM.
class VOCFile
{
private:
	std::vector<uint8_t> audioData;
	int sampleRate;
public:
	bool init(const char *filename);

	// Usually between 4000 and 11111.
	int getSampleRate() const;

	// Gets the 8-bit unsigned PCM samples.
	BufferView<uint8_t> getAudioData();
	BufferView<const uint8_t> getAudioData() const;
};

#endif
