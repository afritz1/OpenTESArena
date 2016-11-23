#ifndef MUSIC_FILE_H
#define MUSIC_FILE_H

#include <string>

// Static class for accessing Arena music filenames.

enum class MusicName;

class MusicFile
{
	MusicFile() = delete;
	MusicFile(const MusicFile&) = delete;
	~MusicFile() = delete;
public:
	static const std::string &fromName(MusicName musicName);
};

#endif
