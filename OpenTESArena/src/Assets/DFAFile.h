#ifndef DFA_FILE_H
#define DFA_FILE_H

#include <string>

// A DFA file contains animation frames for static entities, like the bartender
// and various kinds of torches.

class DFAFile
{
public:
	DFAFile(const std::string &filename);
	~DFAFile();
};

#endif
