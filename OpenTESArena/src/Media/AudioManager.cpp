#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "al.h"
#include "alc.h"

#include "AudioManager.h"

#include "MusicName.h"
#include "MusicType.h"
#include "SoundName.h"
#include "WildMidi.hpp"
#include "../Game/Options.h"
#include "../Utilities/Debug.h"

namespace
{
	// Each MusicType corresponds to a list of MusicNames. These lists should be
	// given to some other class in the project so that the audio manager doesn't
	// need to use random numbers (remove the MusicType #include at that point).
	const std::map<MusicType, std::vector<MusicName>> MusicTypeNames =
	{
		{ MusicType::ArabCityEnter, { MusicName::ArabCityEnter } },
		{ MusicType::ArabTownEnter, { MusicName::ArabTownEnter } },
		{ MusicType::ArabVillageEnter, { MusicName::ArabVillageEnter } },
		{ MusicType::CityEnter, { MusicName::CityEnter } },
		{ MusicType::Credits, { MusicName::Credits } },
		{ MusicType::Dungeon, { MusicName::Dungeon1, MusicName::Dungeon2, MusicName::Dungeon3, MusicName::Dungeon4 } }, 
		{ MusicType::Equipment, { MusicName::Equipment } },
		{ MusicType::Evil, { MusicName::Evil } },
		{ MusicType::EvilIntro, { MusicName::EvilIntro } },
		{ MusicType::Magic, { MusicName::Magic } },
		{ MusicType::Night, { MusicName::Night } },
		{ MusicType::Overcast, { MusicName::Overcast } },
		{ MusicType::Palace, { MusicName::Palace } },
		{ MusicType::PercIntro, { MusicName::PercIntro } },
		{ MusicType::Raining, { MusicName::Raining } },
		{ MusicType::Sheet, { MusicName::Sheet } },
		{ MusicType::Sneaking, { MusicName::Sneaking } },
		{ MusicType::Snowing, { MusicName::Snowing, MusicName::OverSnow } },
		{ MusicType::Sunny, { MusicName::SunnyDay } },
		{ MusicType::Swimming, { MusicName::Swimming } },
		{ MusicType::Tavern, { MusicName::Tavern, MusicName::Square } },
		{ MusicType::Temple, { MusicName::Temple } },
		{ MusicType::TownEnter, { MusicName::TownEnter } },
		{ MusicType::VillageEnter, { MusicName::VillageEnter } },
		{ MusicType::Vision, { MusicName::Vision } },
		{ MusicType::WinGame, { MusicName::WinGame } }
	};

	// Each MusicName has a corresponding filename. Interestingly, it seems Arena
	// has separate XFM files for FM synth output devices (OPL, as on Adlib and
	// Sound Blaster before the AWE32), while the corresponding XMI files are for
	// MT-32, MPU-401, and other General MIDI devices.
	const std::map<MusicName, std::string> MusicFilenames =
	{
		{ MusicName::ArabCityEnter, "ARABCITY.XMI" },
		{ MusicName::ArabTownEnter, "ARABTOWN.XMI" },
		{ MusicName::ArabVillageEnter, "ARAB_VLG.XMI" },
		{ MusicName::CityEnter, "CITY.XMI" },
		{ MusicName::Combat, "COMBAT.XMI" },
		{ MusicName::Credits, "CREDITS.XMI" },
		{ MusicName::Dungeon1, "DUNGEON1.XMI" },
		{ MusicName::Dungeon2, "DUNGEON2.XMI" },
		{ MusicName::Dungeon3, "DUNGEON3.XMI" },
		{ MusicName::Dungeon4, "DUNGEON4.XMI" },
		{ MusicName::Equipment, "EQUIPMNT.XMI" },
		{ MusicName::Evil, "EVIL.XMI" },
		{ MusicName::EvilIntro, "EVLINTRO.XMI" },
		{ MusicName::Magic, "MAGIC_2.XMI" },
		{ MusicName::Night, "NIGHT.XMI" },
		{ MusicName::Overcast, "OVERCAST.XMI" },
		{ MusicName::OverSnow, "OVERSNOW.XMI" },
		{ MusicName::Palace, "PALACE.XMI" },
		{ MusicName::PercIntro, "PERCNTRO.XMI" },
		{ MusicName::Raining, "RAINING.XMI" },
		{ MusicName::Sheet, "SHEET.XMI" },
		{ MusicName::Sneaking, "SNEAKING.XMI" },
		{ MusicName::Snowing, "SNOWING.XMI" },
		{ MusicName::Square, "SQUARE.XMI" },
		{ MusicName::SunnyDay, "SUNNYDAY.XMI" },
		{ MusicName::Swimming, "SWIMMING.XMI" },
		{ MusicName::Tavern, "TAVERN.XMI" },
		{ MusicName::Temple, "TEMPLE.XMI" },
		{ MusicName::TownEnter, "TOWN.XMI" },
		{ MusicName::VillageEnter, "VILLAGE.XMI" },
		{ MusicName::Vision, "VISION.XMI" },
		{ MusicName::WinGame, "WINGAME.XMI" }
	};

	// Each SoundName has a corresponding filename. A number of them have
	// their name mixed up with another in the original files. I'm not sure
	// if the entity "Walk" sounds are used (maybe just the iron golem's).
	// - Unused/duplicate sounds: MOON.VOC, SNARL2.VOC, UMPH.VOC, WHINE.VOC.
	const std::map<SoundName, std::string> SoundFilenames =
	{
		// Ambient
		{ SoundName::Back1, "BACK1.VOC" },
		{ SoundName::Birds, "BIRDS.VOC" },
		{ SoundName::Birds2, "BIRDS2.VOC" },
		{ SoundName::Clicks, "CLICKS.VOC" },
		{ SoundName::DeepChoir, "DEEPCHOI.VOC" },
		{ SoundName::Drip1, "DRIP1.VOC" },
		{ SoundName::Drip2, "DRIP2.VOC" },
		{ SoundName::Drums, "DRUMS.VOC" },
		{ SoundName::Eerie, "EERIE.VOC" },
		{ SoundName::HiChoir, "HICHOIR.VOC" },
		{ SoundName::HumEerie, "HUMEERIE.VOC" },
		{ SoundName::Scream1, "SCREAM1.VOC" },
		{ SoundName::Scream2, "SCREAM2.VOC" },
		{ SoundName::Thunder, "THUNDER.VOC" },
		{ SoundName::Wind, "WIND.VOC" },

		// Combat
		{ SoundName::ArrowFire, "ARROWFR.VOC" },
		{ SoundName::ArrowHit, "ARROWHT.VOC" },
		{ SoundName::Bash, "BASH.VOC" },
		{ SoundName::BodyFall, "BODYFALL.VOC" },
		{ SoundName::Clank, "CLANK.VOC" },
		{ SoundName::EnemyHit, "EHIT.VOC" },
		{ SoundName::FemaleDie, "FDIE.VOC" },
		{ SoundName::MaleDie, "MDIE.VOC" },
		{ SoundName::NHit, "NHIT.VOC" },
		{ SoundName::PlayerHit, "UHIT.VOC" },
		{ SoundName::Swish, "SWISH.VOC" },

		// Crime
		{ SoundName::Halt, "HALT.VOC" },
		{ SoundName::StopThief, "STPTHIEF.VOC" },

		// Doors
		{ SoundName::CloseDoor, "CLOSDOOR.VOC" },
		{ SoundName::Grind, "GRIND.VOC" },
		{ SoundName::Lock, "LOCK.VOC" },
		{ SoundName::OpenAlt, "OPENALT.VOC" },
		{ SoundName::OpenDoor, "OPENDOOR.VOC" },
		{ SoundName::Portcullis, "PORTC.VOC" },

		// Entities
		{ SoundName::Rat, "RATS.VOC" },
		{ SoundName::SnowWolf, "WOLF.VOC" },
		{ SoundName::Spider, "SKELETON.VOC" },
		{ SoundName::Troll, "TROLL.VOC" },
		{ SoundName::Wolf, "WOLF.VOC" },

		{ SoundName::Goblin, "LICH.VOC" },
		{ SoundName::LizardMan, "MONSTER.VOC" },
		{ SoundName::LizardManWalk, "LIZARDST.VOC" },
		{ SoundName::Medusa, "SQUISH1.VOC" },
		{ SoundName::Minotaur, "GROWL2.VOC" },
		{ SoundName::Orc, "ORC.VOC" },

		{ SoundName::IceGolem, "ICEGOLEM.VOC" },
		{ SoundName::IronGolem, "GROWL1.VOC" },
		{ SoundName::IronGolemWalk, "IRONGOLS.VOC" },
		{ SoundName::StoneGolem, "STONEGOL.VOC" },

		{ SoundName::FireDaemon, "FIREDAEM.VOC" },
		{ SoundName::HellHound, "GROWL.VOC" },
		{ SoundName::HellHoundWalk, "HELLHOUN.VOC" },
		{ SoundName::Homonculus, "HOMO.VOC" },

		{ SoundName::Ghost, "GHOST.VOC" },
		{ SoundName::Ghoul, "GHOUL.VOC" },
		{ SoundName::Lich, "MINOTAUR.VOC" },
		{ SoundName::Skeleton, "MEDUSA.VOC" },
		{ SoundName::Vampire, "VAMPIRE.VOC" },
		{ SoundName::Wraith, "WRAITH.VOC" },
		{ SoundName::Zombie, "ZOMBIE.VOC" },

		// Fanfare
		{ SoundName::Fanfare1, "FANFARE1.VOC" },
		{ SoundName::Fanfare2, "FANFARE2.VOC" },

		// Movement
		{ SoundName::DirtLeft, "DIRTL.VOC" },
		{ SoundName::DirtRight, "DIRTR.VOC" },
		{ SoundName::MudLeft, "MUDSTE.VOC" },
		{ SoundName::MudRight, "MUDSTEP.VOC" },
		{ SoundName::SnowLeft, "SNOWL.VOC" },
		{ SoundName::SnowRight, "SNOWR.VOC" },
		{ SoundName::Splash, "SPLASH.VOC" },
		{ SoundName::Swim, "SWIMMING.VOC" },

		// Spells
		{ SoundName::Burst, "BURST5.VOC" },
		{ SoundName::Explode, "EXPLODE.VOC" },
		{ SoundName::SlowBall, "SLOWBALL.VOC" }
	};
}

std::unique_ptr<MidiDevice> MidiDevice::sInstance;

class OpenALStream;

class AudioManagerImpl {
public:
	float mMusicVolume;
	float mSfxVolume;

	/* Currently active song and playback stream. */
	MidiSongPtr mCurrentSong;
	std::unique_ptr<OpenALStream> mSongStream;

	/* A deque of available sources to play sounds and streams with. */
	std::deque<ALuint> mFreeSources;

	AudioManagerImpl();
	~AudioManagerImpl();

	void init(const Options &options);

	bool musicIsPlaying() const;

	// All music should loop until changed. Some, like when entering a city, are an 
	// exception to this.
	void playMusic(MusicName musicName);
	void playSound(SoundName soundName);

	void toggleMusic();
	void stopMusic();
	void stopSound();

	// Percent is [0.0, 1.0].
	void setMusicVolume(double percent);
	void setSoundVolume(double percent);
};


class OpenALStream {
	AudioManagerImpl *mManager;
	MidiSong *mSong;

	/* Background thread and control. */
	std::atomic<bool> mQuit;
	std::thread mThread;

	/* Playback source and buffer queue. */
	static const int32_t sBufferFrames = 16384;
	ALuint mSource;
	std::array<ALuint, 4> mBuffers;
	ALuint mBufferIdx;

	/* Stream format. */
	ALenum mFormat;
	ALuint mSampleRate;
	ALuint mFrameSize;

	/* Read samples from the song and fill the given OpenAL buffer ID (buffer
	 * vector is for temporary storage). Returns true if the buffer was filled.
	 */
	bool fillBuffer(ALuint bufid, std::vector<char> &buffer)
	{
		size_t totalSize = 0;
		while (totalSize < buffer.size())
		{
			size_t toget = (buffer.size() - totalSize) / mFrameSize;
			size_t got = mSong->read(buffer.data() + totalSize, toget);
			if (got < toget)
			{
				/* End of song, rewind to loop. */
				if (!mSong->seek(0))
					break;
			}
			totalSize += got*mFrameSize;
		}
		if (totalSize == 0)
			return false;

		std::fill(buffer.begin() + totalSize, buffer.end(), 0);
		alBufferData(bufid, mFormat, buffer.data(),
			static_cast<ALsizei>(buffer.size()), mSampleRate);
		return true;
	}

	/* Fill buffers to fill up the source queue. Returns the number of buffers
	 * queued.
	 */
	ALint fillBufferQueue(std::vector<char> &buffer)
	{
		ALint queued;
		alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
		while (queued < mBuffers.size())
		{
			ALuint bufid = mBuffers[mBufferIdx];
			if (!fillBuffer(bufid, buffer))
				break;
			mBufferIdx = (mBufferIdx + 1) % mBuffers.size();
			alSourceQueueBuffers(mSource, 1, &bufid);
			++queued;
		}
		return queued;
	}

	/* A method run in a backround thread, to keep filling the queue with new
	 * audio over time.
	 */
	void backgroundProc()
	{
		/* Temporary storage to read samples into, before passing to OpenAL.
		 * Kept here to avoid reallocating it during playback.
		 */
		std::vector<char> buffer(sBufferFrames * mFrameSize);

		while (!mQuit.load())
		{
			/* First, make sure the buffer queue is filled. */
			fillBufferQueue(buffer);

			ALint state;
			alGetSourcei(mSource, AL_SOURCE_STATE, &state);
			if (state != AL_PLAYING && state != AL_PAUSED)
			{
				/* If the source is not playing or paused, it either underrun
				 * or hasn't started at all yet. So remove any buffers that
				 * have been played (will be 0 when first starting).
				 */
				ALint processed;
				alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
				while (processed > 0)
				{
					ALuint bufid;
					alSourceUnqueueBuffers(mSource, 1, &bufid);
					--processed;
				}

				/* Make sure the buffer queue is still filled, in case another
				 * buffer had finished before checking the state and after the
				 * last fill. If the queue is empty, playback is over.
				 */
				if (fillBufferQueue(buffer) == 0)
				{
					mQuit.store(true);
					return;
				}

				/* Now start the sound source. */
				alSourcePlay(mSource);
			}

			ALint processed;
			alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
			if (processed == 0)
			{
				/* Wait until a buffer in the queue has been processed. */
				do {
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					if (mQuit.load()) break;
					alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
				} while (processed == 0);
			}
			/* Remove processed buffers, then restart the loop to keep the
			 * queue filled.
			 */
			while (processed > 0)
			{
				ALuint bufid;
				alSourceUnqueueBuffers(mSource, 1, &bufid);
				--processed;
			}
		}
	}

public:
	OpenALStream(AudioManagerImpl *manager, MidiSong *song)
		: mManager(manager), mSong(song), mQuit(false), mSource(0)
		, mBufferIdx(0), mSampleRate(0)
	{
		// Using std::fill for mBuffers since VS2013 doesn't support mBuffers{0}.
		std::fill(mBuffers.begin(), mBuffers.end(), 0);
	}

	~OpenALStream()
	{
		if (mThread.get_id() != std::thread::id())
		{
			/* Tell the thread to quit and wait for it to stop. */
			mQuit.store(true);
			mThread.join();
		}
		if (mSource)
		{
			/* Stop the source, remove the buffers, then put it back so it can
			 * be used again.
			 */
			alSourceRewind(mSource);
			alSourcei(mSource, AL_BUFFER, 0);
			mManager->mFreeSources.push_front(mSource);
		}
		/* Delete the buffers used for the queue. */
		alDeleteBuffers(static_cast<ALsizei>(mBuffers.size()), mBuffers.data());
	}

	void play()
	{
		/* If the source is already playing (thread exists and isn't stopped),
		 * don't do anything.
		 */
		if (mThread.get_id() != std::thread::id())
		{
			if (!mQuit.load())
				return;
			mThread.join();
		}

		/* Reset the source and clear any buffers that may be on it. */
		alSourceRewind(mSource);
		alSourcei(mSource, AL_BUFFER, 0);
		mBufferIdx = 0;
		mQuit.store(false);

		/* Start the background thread processing. */
		mThread = std::thread(std::mem_fn(&OpenALStream::backgroundProc), this);
	}

	void stop()
	{
		if (mThread.get_id() != std::thread::id())
		{
			mQuit.store(true);
			mThread.join();
		}

		alSourceRewind(mSource);
		alSourcei(mSource, AL_BUFFER, 0);
		mBufferIdx = 0;
	}

	void setVolume(float volume)
	{
		assert(mSource != 0);
		alSourcef(mSource, AL_GAIN, volume);
	}

	bool init(ALuint source, float volume)
	{
		assert(mSource == 0);

		/* Clear existing errors */
		alGetError();

		alGenBuffers(static_cast<ALsizei>(mBuffers.size()), mBuffers.data());
		if (alGetError() != AL_NO_ERROR)
		{
			std::fill(mBuffers.begin(), mBuffers.end(), 0);
			return false;
		}

		/* Set the default properties for localized playback */
		alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
		alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSourcef(source, AL_GAIN, volume);
		alSourcef(source, AL_PITCH, 1.0f);
		alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);
		alSourcef(source, AL_SEC_OFFSET, 0.0f);
		alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
		alSourcei(source, AL_LOOPING, AL_FALSE);
		if (alGetError() != AL_NO_ERROR)
			return false;

		int32_t srate;
		mSong->getFormat(&srate);

		/* Currently hard-coded to 16-bit stereo. */
		mFormat = AL_FORMAT_STEREO16;
		mFrameSize = 4;
		mSampleRate = srate;

		mSource = source;
		return true;
	}
};

// Audio Manager Impl

AudioManagerImpl::AudioManagerImpl()
	: mMusicVolume(1.0f), mSfxVolume(1.0f)
{

}

AudioManagerImpl::~AudioManagerImpl()
{
	stopMusic();

	MidiDevice::shutdown();

	ALCcontext *context = alcGetCurrentContext();
	if (!context) return;


	for (ALuint source : mFreeSources)
		alDeleteSources(1, &source);
	mFreeSources.clear();

	ALCdevice *device = alcGetContextsDevice(context);
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

void AudioManagerImpl::init(const Options &options)
{
	Debug::mention("Audio Manager", "Initializing.");

#ifdef HAVE_WILDMIDI
	WildMidiDevice::init(options.getSoundfont());
#endif

	// Start initializing the OpenAL device.
	ALCdevice *device = alcOpenDevice(nullptr);
	Debug::check(device != nullptr, "Audio Manager", "alcOpenDevice");

	// Create an OpenAL context.
	ALCcontext *context = alcCreateContext(device, nullptr);
	Debug::check(context != nullptr, "Audio Manager", "alcCreateContext");

	ALCboolean success = alcMakeContextCurrent(context);
	Debug::check(success == AL_TRUE, "Audio Manager", "alcMakeContextCurrent");

	double musicVolume = options.getMusicVolume();
	double soundVolume = options.getSoundVolume();
	int32_t maxChannels = options.getSoundChannelCount();

	this->setMusicVolume(musicVolume);
	this->setSoundVolume(soundVolume);

	for (size_t i = 0; i < maxChannels; ++i)
	{
		ALuint source;
		alGenSources(1, &source);
		if (alGetError() != AL_NO_ERROR)
			break;
		mFreeSources.push_back(source);
	}
}

bool AudioManagerImpl::musicIsPlaying() const
{
	return false;
}

void AudioManagerImpl::playMusic(MusicName musicName)
{
	stopMusic();

	auto music = MusicFilenames.find(musicName);
	if (music == MusicFilenames.end())
	{
		Debug::mention("Audio Manager", "Failed to lookup music ID " +
			std::to_string(static_cast<int32_t>(musicName)) + ".");
		mCurrentSong = nullptr;
	}
	else if (!mFreeSources.empty())
	{
		if (MidiDevice::isInited())
			mCurrentSong = MidiDevice::get().open(music->second);
		if (!mCurrentSong)
		{
			Debug::mention("Audio Manager", "Failed to play " + music->second + ".");
			return;
		}

		mSongStream.reset(new OpenALStream(this, mCurrentSong.get()));
		if (mSongStream->init(mFreeSources.front(), mMusicVolume))
		{
			mFreeSources.pop_front();
			mSongStream->play();
			Debug::mention("Audio Manager", "Playing music " + music->second + ".");
		}
		else
			Debug::mention("Audio Manager", "Failed to init song stream.");
	}
}

void AudioManagerImpl::playSound(SoundName soundName)
{
	static_cast<void>(soundName);
}

void AudioManagerImpl::toggleMusic()
{

}

void AudioManagerImpl::stopMusic()
{
	if (mSongStream)
		mSongStream->stop();
	mSongStream = nullptr;
	mCurrentSong = nullptr;
}

void AudioManagerImpl::stopSound()
{

}

void AudioManagerImpl::setMusicVolume(double percent)
{
	if (mSongStream)
		mSongStream->setVolume(static_cast<float>(percent));
	mMusicVolume = static_cast<float>(percent);
}

void AudioManagerImpl::setSoundVolume(double percent)
{
	mSfxVolume = static_cast<float>(percent);
}

// Audio Manager

const double AudioManager::MIN_VOLUME = 0.0;
const double AudioManager::MAX_VOLUME = 1.0;

AudioManager::AudioManager()
	: pImpl(new AudioManagerImpl())
{

}

AudioManager::~AudioManager()
{

}

void AudioManager::init(const Options &options)
{
	pImpl->init(options);
}

bool AudioManager::musicIsPlaying() const
{
	return pImpl->musicIsPlaying();
}

void AudioManager::playMusic(MusicName musicName)
{
	pImpl->playMusic(musicName);
}

void AudioManager::playSound(SoundName soundName)
{
	pImpl->playSound(soundName);
}

void AudioManager::stopMusic()
{
	pImpl->stopMusic();
}

void AudioManager::stopSound()
{
	pImpl->stopSound();
}

void AudioManager::toggleMusic()
{
	pImpl->toggleMusic();
}

void AudioManager::setMusicVolume(double percent)
{
	pImpl->setMusicVolume(percent);
}

void AudioManager::setSoundVolume(double percent)
{
	pImpl->setSoundVolume(percent);
}
