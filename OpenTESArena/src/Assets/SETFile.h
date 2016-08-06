#ifndef SET_FILE_H
#define SET_FILE_H

#include <string>

// A SET file is packed vertically with some uncompressed wall IMGs. Every SET 
// file has a width of 64 and a height a multiple of 64 based on the number of 
// images contained.

class SETFile
{
public:
	SETFile(const std::string &filename);
	~SETFile();	
};

#endif
