#ifndef FLC_FILE_H
#define FLC_FILE_H

#include <string>

// An FLC file is a video similar to a GIF. The CEL files can be used with this 
// class since they are identical to FLC, though with an extra chunk of header data.

// As a baseline, it should be converted to a vector of independent frames (in case
// it uses a delta format for frame differences).

// This website has some good information on the FLIC format:
// - http://www.compuphase.com/flic.htm

class FLCFile
{
public:
	FLCFile(const std::string &filename);
	~FLCFile();
};

#endif
