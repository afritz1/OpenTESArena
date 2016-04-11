#ifndef MUSIC_MANAGER_H
#define MUSIC_MANAGER_H

#include <map>
#include <string>

enum class MusicFormat;
enum class MusicName;
enum class MusicType;

struct FMOD_SYSTEM;
struct FMOD_SOUND;
struct FMOD_CHANNEL;

class MusicManager
{
private:
	static const std::string PATH;

	FMOD_SYSTEM *system;
	FMOD_CHANNEL *channel;
	std::map<MusicName, FMOD_SOUND*> musics;

	void checkSuccess(bool success, const std::string &message) const;
	bool musicIsLoaded(FMOD_SOUND *music) const;
	void releaseMusic(FMOD_SOUND *music);
public:
	// All musics should be loaded on start-up and kept for the lifetime of the program.
	// This is to keep from having interrupts while playing.
	MusicManager(MusicFormat format);
	~MusicManager();

	static const double MIN_VOLUME;
	static const double MAX_VOLUME;
	
	double getVolume() const;
	bool isPlaying() const;

	// All music will continue to loop until changed by an outside force.
	void play(MusicName musicName);

	// This picks a random MusicName behind the scenes.
	void play(MusicType musicType);

	void togglePause();

	// Percent is [0.0, 1.0].
	void setVolume(double percent);
};

#endif
