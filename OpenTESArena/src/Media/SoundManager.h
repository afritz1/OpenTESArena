#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <map>
#include <string>
#include <vector>

// If there can't be separate instances of FMOD running then just fuse the MusicManager
// and SoundManager into an AudioManager.

enum class SoundFormat;
enum class SoundName;

struct FMOD_CHANNEL;
struct FMOD_SOUND;
struct FMOD_SYSTEM;

class SoundManager
{
private:
	static const std::string PATH;

	FMOD_SYSTEM *system;
	FMOD_CHANNEL *channel; // Should this be a std::vector?
	std::map<SoundName, FMOD_SOUND*> sounds;

	void checkSuccess(bool success, const std::string &message) const;
	bool soundIsLoaded(FMOD_SOUND *sound) const;
	void releaseSound(FMOD_SOUND *sound);
public:
	// All sounds should be loaded on start-up and kept for the lifetime of the program.
	// This is to keep from having interrupts while playing.
	SoundManager();
	~SoundManager();

	static const double MIN_VOLUME;
	static const double MAX_VOLUME;

	// All sounds have the same volume, for simplicity.
	double getVolume() const;

	// All sounds play only once.
	void play(SoundName sound);

	// Percent is [0.0, 1.0].
	void setVolume(double percent);
};

#endif
