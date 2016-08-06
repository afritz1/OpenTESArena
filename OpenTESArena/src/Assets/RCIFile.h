#ifndef RCI_FILE_H
#define RCI_FILE_H

#include <string>

// An RCI file is for screen-space animations like water and lava.

class RCIFile
{
public:
	RCIFile(const std::string &filename);
	~RCIFile();
};

#endif
