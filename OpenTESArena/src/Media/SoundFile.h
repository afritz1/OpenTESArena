#ifndef SOUND_FILE_H
#define SOUND_FILE_H

#include <string>

// Namespace for accessing Arena sound filenames.

enum class SoundName;

namespace SoundFile
{
	const std::string &fromName(SoundName soundName);
}

#endif
