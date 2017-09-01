#ifndef INF_FILE_H
#define INF_FILE_H

#include <string>

// An .INF file contains definitions of what the IDs in a .MIF file point to. These 
// are mostly texture IDs, but also text IDs and sound IDs telling which voxels have 
// which kinds of triggers, etc..

class INFFile
{
private:
	std::string text;
public:
	INFFile(const std::string &filename);
	~INFFile();

	// Gets the .INF file text.
	const std::string &getText() const;
};

#endif
