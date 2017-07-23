#ifndef VOC_FILE_H
#define VOC_FILE_H

#include <string>

// A .VOC file contains audio data in the Creative Voice format. It's used with any 
// sounds in Arena, and in the CD version it's also used with voices in cinematics.

// All of Arena's .VOC files are mono 8-bit unsigned PCM.

class VOCFile
{
private:

public:
	VOCFile(const std::string &filename);
	~VOCFile();

	// To do: expose audio data as 16-bit signed PCM to OpenAL Soft?
};

#endif
