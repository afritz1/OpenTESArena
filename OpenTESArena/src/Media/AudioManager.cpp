#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>

#include "al.h"
#include "alc.h"

#include "AudioManager.h"

#include "MusicFormat.h"
#include "MusicName.h"
#include "MusicType.h"
#include "SoundFormat.h"
#include "SoundName.h"
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

class AudioManagerImpl 
{
public:
    static const std::string MUSIC_PATH;
    static const std::string SOUNDS_PATH;

    std::map<std::string, std::uint32_t> objects;
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

const double AudioManager::MIN_VOLUME = 0.0;
const double AudioManager::MAX_VOLUME = 1.0;

AudioManagerImpl::AudioManagerImpl()
{

}

AudioManagerImpl::~AudioManagerImpl()
{
    ALCcontext *context = alcGetCurrentContext();

	if (!context)
	{
		return;
	}

    ALCdevice *device = alcGetContextsDevice(context);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
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
	static_cast<void>(musicName);
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
	
}

void AudioManagerImpl::stopSound()
{
	
}

void AudioManagerImpl::setMusicVolume(double percent)
{
	static_cast<void>(percent);
}

void AudioManagerImpl::setSoundVolume(double percent)
{
	static_cast<void>(percent);
}

// Audio Manager

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
