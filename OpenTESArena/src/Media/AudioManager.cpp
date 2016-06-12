
#include "AudioManager.h"

#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <thread>
#include <atomic>
#include <deque>

#include "al.h"
#include "alc.h"

#include "MusicFormat.h"
#include "MusicName.h"
#include "MusicType.h"
#include "SoundFormat.h"
#include "SoundName.h"
#include "WildMidi.hpp"
#include "../Utilities/Debug.h"

namespace
{
// Each MusicType corresponds to a list of MusicNames. These lists should be
// given to some other class in the project so that the audio manager doesn't
// need to use random numbers (remove the MusicType #include at that point).
const auto MusicTypeNames = std::map<MusicType, std::vector<MusicName>>
{
	{ MusicType::ArabCityEnter, { MusicName::ArabCityEnter } },
	{ MusicType::ArabTownEnter, { MusicName::ArabTownEnter } },
	{ MusicType::ArabVillageEnter, { MusicName::ArabVillageEnter } },
	{ MusicType::CityEnter, { MusicName::CityEnter } },
	{ MusicType::Credits, { MusicName::Credits } },
	{ MusicType::Dungeon, { MusicName::Dungeon1, MusicName::Dungeon2, MusicName::Dungeon3,
	MusicName::Dungeon4 } },
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

// Each MusicName has a corresponding filename.
const auto MusicFilenames = std::map<MusicName, std::string>
{
	{ MusicName::ArabCityEnter, "arab_city_enter" },
	{ MusicName::ArabTownEnter, "arab_town_enter" },
	{ MusicName::ArabVillageEnter, "arab_village_enter" },
	{ MusicName::CityEnter, "city_enter" },
	{ MusicName::Credits, "credits" },
	{ MusicName::Dungeon1, "dungeon_1" },
	{ MusicName::Dungeon2, "dungeon_2" },
	{ MusicName::Dungeon3, "dungeon_3" },
	{ MusicName::Dungeon4, "dungeon_4" },
	{ MusicName::Equipment, "equipment" },
	{ MusicName::Evil, "evil" },
	{ MusicName::EvilIntro, "evil_intro" },
	{ MusicName::Magic, "magic" },
	{ MusicName::Night, "night" },
	{ MusicName::Overcast, "overcast" },
	{ MusicName::OverSnow, "oversnow" },
	{ MusicName::Palace, "palace" },
	{ MusicName::PercIntro, "perc_intro" },
	{ MusicName::Raining, "raining" },
	{ MusicName::Sheet, "sheet" },
	{ MusicName::Sneaking, "sneaking" },
	{ MusicName::Snowing, "snowing" },
	{ MusicName::Square, "square" },
	{ MusicName::SunnyDay, "sunny_day" },
	{ MusicName::Swimming, "swimming" },
	{ MusicName::Tavern, "tavern" },
	{ MusicName::Temple, "temple" },
	{ MusicName::TownEnter, "town_enter" },
	{ MusicName::VillageEnter, "village_enter" },
	{ MusicName::Vision, "vision" },
	{ MusicName::WinGame, "win_game" }
};

// Each SoundName has a corresponding filename.
const auto SoundFilenames = std::map<SoundName, std::string>
{
	// Ambient
	{ SoundName::Back1, "ambient/back1" },
	{ SoundName::Birds, "ambient/birds" },
	{ SoundName::Birds2, "ambient/birds2" },
	{ SoundName::Clicks, "ambient/clicks" },
	{ SoundName::DeepChoi, "ambient/deep_choi" },
	{ SoundName::Drip1, "ambient/drip1" },
	{ SoundName::Drip2, "ambient/drip2" },
	{ SoundName::Drums, "ambient/drums" },
	{ SoundName::Eerie, "ambient/eerie" },
	{ SoundName::HiChoi, "ambient/hi_choi" },
	{ SoundName::HumEerie, "ambient/hum_eerie" },
	{ SoundName::Scream1, "ambient/scream1" },
	{ SoundName::Scream2, "ambient/scream2" },
	{ SoundName::Thunder, "ambient/thunder" },
	{ SoundName::Wind, "ambient/wind" },

	// Combat
	{ SoundName::ArrowFire, "combat/arrow_fire" },
	{ SoundName::ArrowHit, "combat/arrow_hit" },
	{ SoundName::Bash, "combat/bash" },
	{ SoundName::BodyFall, "combat/body_fall" },
	{ SoundName::Clank, "combat/clank" },
	{ SoundName::EnemyHit, "combat/enemy_hit" },
	{ SoundName::FemaleDie, "combat/female_die" },
	{ SoundName::MaleDie, "combat/male_die" },
	{ SoundName::NHit, "combat/n_hit" },
	{ SoundName::PlayerHit, "combat/player_hit" },
	{ SoundName::Swish, "combat/swish" },

	// Crime
	{ SoundName::Halt, "crime/halt" },
	{ SoundName::StopThief, "crime/stop_thief" },

	// Doors
	{ SoundName::CloseDoor, "doors/close_door" },
	{ SoundName::Grind, "doors/grind" },
	{ SoundName::Lock, "doors/lock" },
	{ SoundName::OpenAlt, "doors/open_alt" },
	{ SoundName::OpenDoor, "doors/open_door" },
	{ SoundName::Portcullis, "doors/portcullis" },

	// Entities
	{ SoundName::Rat, "entities/animals/rat" },
	{ SoundName::SnowWolf, "entities/animals/snow_wolf" },
	{ SoundName::Spider, "entities/animals/spider" },
	{ SoundName::Troll, "entities/animals/troll" },
	{ SoundName::Wolf, "entities/animals/wolf" },

	{ SoundName::Goblin, "entities/creatures/goblin" },
	{ SoundName::LizardMan, "entities/creatures/lizard_man" },
	{ SoundName::LizardManWalk, "entities/creatures/lizard_man_walk" },
	{ SoundName::Medusa, "entities/creatures/medusa" },
	{ SoundName::Minotaur, "entities/creatures/minotaur" },
	{ SoundName::Orc, "entities/creatures/orc" },

	{ SoundName::IceGolem, "entities/golems/ice_golem" },
	{ SoundName::IronGolem, "entities/golems/iron_golem" },
	{ SoundName::IronGolemWalk, "entities/golems/iron_golem_walk" },
	{ SoundName::StoneGolem, "entities/golems/stone_golem" },

	{ SoundName::FireDaemon, "entities/netherworld/fire_daemon" },
	{ SoundName::HellHound, "entities/netherworld/hell_hound" },
	{ SoundName::HellHoundWalk, "entities/netherworld/hell_hound_walk" },
	{ SoundName::Homonculus, "entities/netherworld/homonculus" },

	{ SoundName::Ghost, "entities/undead/ghost" },
	{ SoundName::Ghoul, "entities/undead/ghoul" },
	{ SoundName::Lich, "entities/undead/lich" },
	{ SoundName::Skeleton, "entities/undead/skeleton" },
	{ SoundName::Vampire, "entities/undead/vampire" },
	{ SoundName::Wraith, "entities/undead/wraith" },
	{ SoundName::Zombie, "entities/undead/zombie" },

	// Fanfare
	{ SoundName::Fanfare1, "fanfare/fanfare_1" },
	{ SoundName::Fanfare2, "fanfare/fanfare_2" },

	// Interface
	{ SoundName::Burst, "interface/burst" },

	// Movement
	{ SoundName::DirtLeft, "movement/dirt_left" },
	{ SoundName::DirtRight, "movement/dirt_right" },
	{ SoundName::MudLeft, "movement/mud_left" },
	{ SoundName::MudRight, "movement/mud_right" },
	{ SoundName::SnowLeft, "movement/snow_left" },
	{ SoundName::SnowRight, "movement/snow_right" },
	{ SoundName::Splash, "movement/splash" },
	{ SoundName::Swim, "movement/swim" },

	// Speech
	{ SoundName::EmperorThanks, "speech/emperor_thanks" },
	{ SoundName::EmperorReward, "speech/emperor_reward" },
	{ SoundName::SilmaneIntro, "speech/silmane_intro" },
	{ SoundName::SilmanePlayerDeath, "speech/silmane_player_death" },
	{ SoundName::SilmaneJourneyBegin, "speech/silmane_journey_begin" },
	{ SoundName::SilmaneFirstSleepQuest, "speech/silmane_first_sleep_quest" },
	{ SoundName::SilmaneFirstPieceObtained, "speech/silmane_first_piece_obtained" },
	{ SoundName::SilmaneSecondPieceObtained, "speech/silmane_second_piece_obtained" },
	{ SoundName::SilmaneThirdPieceObtained, "speech/silmane_third_piece_obtained" },
	{ SoundName::SilmaneFourthPieceObtained, "speech/silmane_fourth_piece_obtained" },
	{ SoundName::SilmaneFifthPieceObtained, "speech/silmane_fifth_piece_obtained" },
	{ SoundName::SilmaneSixthPieceObtained, "speech/silmane_sixth_piece_obtained" },
	{ SoundName::SilmaneSeventhPieceObtained, "speech/silmane_seventh_piece_obtained" },
	{ SoundName::SilmaneFinalPieceObtained, "speech/silmane_final_piece_obtained" },
	{ SoundName::TharnPlayerDeath, "speech/tharn_player_death" },
	{ SoundName::TharnFirstPieceObtained, "speech/tharn_first_piece_obtained" },
	{ SoundName::TharnSecondPieceObtained, "speech/tharn_second_piece_obtained" },
	{ SoundName::TharnThirdPieceObtained, "speech/tharn_third_piece_obtained" },
	{ SoundName::TharnFourthPieceObtained, "speech/tharn_fourth_piece_obtained" },
	{ SoundName::TharnFifthPieceObtained, "speech/tharn_fifth_piece_obtained" },
	{ SoundName::TharnSixthPieceObtained, "speech/tharn_sixth_piece_obtained" },
	{ SoundName::TharnSeventhPieceObtained, "speech/tharn_seventh_piece_obtained" },
	{ SoundName::TharnFinalPieceObtained, "speech/tharn_final_piece_obtained" },
	{ SoundName::TharnFinalBattle, "speech/tharn_final_battle" },
	{ SoundName::TharnAngryJewel, "speech/tharn_angry_jewel" },

	// Spells
	{ SoundName::Explode, "spells/explode" },
	{ SoundName::SlowBall, "spells/slow_ball" }
};

const auto MusicFormatExtensions = std::map<MusicFormat, std::string>
{
	{ MusicFormat::MIDI, ".mid" },
	{ MusicFormat::MP3, ".mp3" },
	{ MusicFormat::Ogg, ".ogg" }
};

const auto SoundFormatExtensions = std::map<SoundFormat, std::string>
{
	{ SoundFormat::Ogg, ".ogg" },
	{ SoundFormat::WAV, ".wav" }
};
}


class OpenALStream;

class AudioManagerImpl {
public:
    static const std::string MUSIC_PATH;
    static const std::string SOUNDS_PATH;

    /* Currently active song and playback stream. */
    MidiSongPtr mCurrentSong;
    std::unique_ptr<OpenALStream> mSongStream;

    /* A deque of available sources to play sounds and streams with. */
    std::deque<ALuint> mFreeSources;

    MusicFormat musicFormat;
    SoundFormat soundFormat;

    AudioManagerImpl();
    ~AudioManagerImpl();

    void init(MusicFormat musicFormat, SoundFormat soundFormat,
        double musicVolume, double soundVolume, int maxChannels);

    void loadMusic(const std::string &filename);
    void loadSound(const std::string &filename);

    bool musicIsPlaying() const;

    // All music will continue to loop until changed by an outside force.
    void playMusic(MusicName musicName);
    void playSound(SoundName soundName);

    void toggleMusic();
    void stopMusic();
    void stopSound();

    // Percent is [0.0, 1.0].
    void setMusicVolume(double percent);
    void setSoundVolume(double percent);
};

const std::string AudioManagerImpl::MUSIC_PATH = "data/music/";
const std::string AudioManagerImpl::SOUNDS_PATH = "data/sounds/";


class OpenALStream {
    AudioManagerImpl *mManager;
    MidiSong *mSong;

    /* Background thread and control. */
    std::atomic<bool> mQuit;
    std::thread mThread;

    /* Playback source and buffer queue. */
    static const int sBufferFrames = 16384;
    ALuint mSource;
    std::array<ALuint,4> mBuffers;
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
        while(totalSize < buffer.size())
        {
            size_t toget = (buffer.size()-totalSize) / mFrameSize;
            size_t got = mSong->read(buffer.data()+totalSize, toget);
            if(got < toget)
            {
                /* End of song, rewind to loop. */
                if(mSong->seek(0))
                    got += mSong->read(buffer.data()+totalSize+got, toget-got);
            }
            totalSize += got*mFrameSize;
        }
        if(totalSize == 0)
            return false;

        std::fill(buffer.begin()+totalSize, buffer.end(), 0);
        alBufferData(bufid, mFormat, buffer.data(), buffer.size(), mSampleRate);
        return true;
    }

    /* Fill buffers to fill up the source queue. Returns the number of buffers
     * queued.
     */
    ALint fillBufferQueue(std::vector<char> &buffer)
    {
        ALint queued;
        alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
        while(queued < mBuffers.size())
        {
            ALuint bufid = mBuffers[mBufferIdx];
            if(!fillBuffer(bufid, buffer))
                break;
            mBufferIdx = (mBufferIdx+1) % mBuffers.size();
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

        while(!mQuit.load())
        {
            /* First, make sure the buffer queue is filled. */
            fillBufferQueue(buffer);

            ALint state;
            alGetSourcei(mSource, AL_SOURCE_STATE, &state);
            if(state != AL_PLAYING && state != AL_PAUSED)
            {
                /* If the source is not playing or paused, it either underrun
                 * or hasn't started at all yet. So remove any buffers that
                 * have been played (will be 0 when first starting).
                 */
                ALint processed;
                alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
                while(processed > 0)
                {
                    ALuint bufid;
                    alSourceUnqueueBuffers(mSource, 1, &bufid);
                    --processed;
                }

                /* Make sure the buffer queue is still filled, in case another
                 * buffer had finished before checking the state and after the
                 * last fill. If the queue is empty, playback is over.
                 */
                if(fillBufferQueue(buffer) == 0)
                {
                    mQuit.store(true);
                    return;
                }

                /* Now start the sound source. */
                alSourcePlay(mSource);
            }

            ALint processed;
            alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
            if(processed == 0)
            {
                /* Wait until a buffer in the queue has been processed. */
                do {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    if(mQuit.load()) break;
                    alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
                } while(processed == 0);
            }
            /* Remove processed buffers, then restart the loop to keep the
             * queue filled.
             */
            while(processed > 0)
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
        , mBuffers{0}, mBufferIdx(0), mSampleRate(0)
    { }
    ~OpenALStream()
    {
        if(mThread.get_id() != std::thread::id())
        {
            /* Tell the thread to quit and wait for it to stop. */
            mQuit.store(true);
            mThread.join();
        }
        if(mSource)
        {
            /* Stop the source, remove the buffers, then put it back so it can
             * be used again.
             */
            alSourceRewind(mSource);
            alSourcei(mSource, AL_BUFFER, 0);
            mManager->mFreeSources.push_front(mSource);
        }
        /* Delete the buffers used for the queue. */
        alDeleteBuffers(mBuffers.size(), mBuffers.data());
    }

    void play()
    {
        /* If the source is already playing (thread exists and isn't stopped),
         * don't do anything.
         */
        if(mThread.get_id() != std::thread::id())
        {
            if(!mQuit.load())
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
        if(mThread.get_id() != std::thread::id())
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

    bool init(ALuint source)
    {
        assert(mSource != 0);

        /* Clear existing errors */
        alGetError();

        alGenBuffers(mBuffers.size(), mBuffers.data());
        if(alGetError() != AL_NO_ERROR)
        {
            std::fill(mBuffers.begin(), mBuffers.end(), 0);
            return false;
        }

        /* Set the default properties for localized playback */
        alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
        alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
        alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSourcef(source, AL_GAIN, 1.0f);
        alSourcef(source, AL_PITCH, 1.0f);
        alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);
        alSourcef(source, AL_SEC_OFFSET, 0.0f);
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcei(source, AL_LOOPING, AL_FALSE);
        if(alGetError() != AL_NO_ERROR)
            return false;

        int srate;
        mSong->getFormat(&srate);

        /* Currently hard-coded to 16-bit stereo. */
        mFormat = AL_FORMAT_STEREO16;
        mFrameSize = 4;
        mSampleRate = srate;

        mSource = source;
        return true;
    }
};


AudioManagerImpl::AudioManagerImpl()
{

}

AudioManagerImpl::~AudioManagerImpl()
{
    stopMusic();

    ALCcontext *context = alcGetCurrentContext();
    if(!context) return;


    for(ALuint source : mFreeSources)
        alDeleteSources(1, &source);
    mFreeSources.clear();

    ALCdevice *device = alcGetContextsDevice(context);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

#ifdef HAVE_WILDMIDI
    WildMidiDevice::shutdown();
#endif
}

void AudioManagerImpl::init(MusicFormat musicFormat, SoundFormat soundFormat,
    double musicVolume, double soundVolume, int maxChannels)
{
    Debug::mention("Audio Manager", "Initializing.");

    // Start initializing the OpenAL device.
    ALCdevice *device = alcOpenDevice(nullptr);
    Debug::check(device != nullptr, "Audio Manager", "alcOpenDevice");

    // Create an OpenAL context.
    ALCcontext *context = alcCreateContext(device, nullptr);
    Debug::check(context != nullptr, "Audio Manager", "alcCreateContext");

    ALCboolean success = alcMakeContextCurrent(context);
    Debug::check(success == AL_TRUE, "Audio Manager", "alcMakeContextCurrent");

    // Set formats.
    this->musicFormat = musicFormat;
    this->soundFormat = soundFormat;

    this->setMusicVolume(musicVolume);
    this->setSoundVolume(soundVolume);

    for(size_t i = 0;i < maxChannels;++i)
    {
        ALuint source;
        alGenSources(1, &source);
        if(alGetError() != AL_NO_ERROR)
            break;
        mFreeSources.push_back(source);
    }
}

bool AudioManagerImpl::musicIsPlaying() const
{
	return false;
}

void AudioManagerImpl::loadMusic(const std::string &filename)
{
	static_cast<void>(filename);
}

void AudioManagerImpl::loadSound(const std::string &filename)
{
	static_cast<void>(filename);
}

void AudioManagerImpl::playMusic(MusicName musicName)
{
    stopMusic();

    auto music = MusicFilenames.find(musicName);
    if(music == MusicFilenames.end())
    {
        Debug::mention("Audio Manager", "Failed to lookup music ID "+
                                        std::to_string((int)musicName));
        mCurrentSong = nullptr;
    }
    else if(!mFreeSources.empty())
    {
#ifdef HAVE_WILDMIDI
        mCurrentSong = WildMidiDevice::get().open(music->second);
#endif
        if(!mCurrentSong)
        {
            Debug::mention("Audio Manager", "Failed to play "+music->second);
            return;
        }

        mSongStream.reset(new OpenALStream(this, mCurrentSong.get()));
        if(mSongStream->init(mFreeSources.front()))
        {
            mFreeSources.pop_front();
            mSongStream->play();
            Debug::mention("Audio Manager", "Playing music "+music->second);
        }
        else
            Debug::mention("Audio Manager", "Failed to init song stream");
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
    if(mSongStream)
        mSongStream->stop();
    mSongStream = nullptr;
    mCurrentSong = nullptr;
}

void AudioManagerImpl::stopSound()
{
	
}

void AudioManagerImpl::setMusicVolume(double percent)
{
    if(mSongStream)
        mSongStream->setVolume(percent);
}

void AudioManagerImpl::setSoundVolume(double percent)
{
	static_cast<void>(percent);
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

void AudioManager::init(MusicFormat musicFormat, SoundFormat soundFormat,
	double musicVolume, double soundVolume, int maxChannels)
{
    pImpl->init(musicFormat, soundFormat, musicVolume, soundVolume, maxChannels);
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
