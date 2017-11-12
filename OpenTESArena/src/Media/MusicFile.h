#ifndef MUSIC_FILE_H
#define MUSIC_FILE_H

#include <string>

// Static class for accessing Arena music filenames.

enum class MusicName;

class MusicFile
{
private:
	MusicFile() = delete;
	~MusicFile() = delete;
public:
	static const std::string &fromName(MusicName musicName);
};

#endif
