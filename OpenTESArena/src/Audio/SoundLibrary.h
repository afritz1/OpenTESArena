#ifndef SOUND_LIBRARY_H
#define SOUND_LIBRARY_H

#include <string>
#include <vector>

#include "components/utilities/Singleton.h"

// Stores all sounds available from game data.
class SoundLibrary : public Singleton<SoundLibrary>
{
private:
	std::vector<std::string> filenames;
public:
	void init();

	int getFilenameCount() const;
	const char *getFilename(int index) const;
};

#endif
