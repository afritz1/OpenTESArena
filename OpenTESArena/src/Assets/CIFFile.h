#ifndef CIF_FILE_H
#define CIF_FILE_H

#include <string>

// A CIF file has one or more images, and each image has some frames associated
// with it. Examples of CIF images are character faces, cursors, and weapon 
// animations.

class CIFFile
{
public:
	CIFFile(const std::string &filename);
	~CIFFile();
};

#endif
