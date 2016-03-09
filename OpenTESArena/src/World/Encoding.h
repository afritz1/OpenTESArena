#ifndef ENCODING_H
#define ENCODING_H

// I moved this over from the IO folder because there are some classes like the 
// TextureManager which do IO operations, but they aren't in the IO folder.

// This class is intended for things like compression and decompression of chunks 
// and the world. Theoretically, encoding and decoding chunks instead of voxels on 
// the large scale should provide better patterns for RLE. It's also suggested to
// work in terms of "regions", or sets of chunks, when reading and writing with the 
// hard drive.

class Encoding
{
private:

public:
	Encoding();
	~Encoding();

	// compress...
	// decompress...
};

#endif