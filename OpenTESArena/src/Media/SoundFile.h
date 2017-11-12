#ifndef SOUND_FILE_H
#define SOUND_FILE_H

#include <string>

// Static class for accessing Arena sound filenames.

enum class SoundName;

class SoundFile
{
private:
	SoundFile() = delete;
	~SoundFile() = delete;
public:
	static const std::string &fromName(SoundName soundName);
};

#endif
