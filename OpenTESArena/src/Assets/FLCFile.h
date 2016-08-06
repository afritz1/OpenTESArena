#ifndef FLC_FILE_H
#define FLC_FILE_H

#include <string>

// An FLC file is a video similar to a GIF. The CEL files can be used with this 
// class since they are identical to FLC.

// As a baseline, it should be converted to a vector of individual frames.

class FLCFile
{
public:
	FLCFile(const std::string &filename);
	~FLCFile();
};

#endif
