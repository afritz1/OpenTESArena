#ifndef CFA_FILE_H
#define CFA_FILE_H

#include <string>

// A CFA file is for creature animations.

class CFAFile
{
public:
	CFAFile(const std::string &filename);
	~CFAFile();
};

#endif
