#include <algorithm>
#include <cassert>
#include <iostream>

#include "FMOD\fmod.h"

#include "AudioManager.h"

#include "MusicFormat.h"
#include "MusicName.h"
#include "MusicType.h"
#include "SoundFormat.h"
#include "SoundName.h"
#include "../Utilities/Debug.h"

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

const std::string AudioManager::MUSIC_PATH = "data/music/";
const std::string AudioManager::SOUNDS_PATH = "data/sounds/";
const double AudioManager::MIN_VOLUME = 0.0;
const double AudioManager::MAX_VOLUME = 1.0;

AudioManager::AudioManager(MusicFormat musicFormat, SoundFormat soundFormat, 
	int maxChannels)
{
	this->system = nullptr;
	this->musicChannel = nullptr;
	this->soundChannel = nullptr;
	this->objects = std::map<std::string, FMOD_SOUND*>();

	// Create the system.
	FMOD_RESULT result = FMOD_System_Create(&this->system);
	Debug::check(result == FMOD_OK, "Audio Manager", "FMOD_System_Create");

	// Initialize the system.
	result = FMOD_System_Init(this->system, maxChannels, FMOD_INIT_NORMAL, nullptr);
	Debug::check(result == FMOD_OK, "Audio Manager", "FMOD_System_Init");

	// Set formats.
	this->musicFormat = musicFormat;
	this->soundFormat = soundFormat;

	// The channels are null until used with "FMOD_System_PlaySound()". The volume 
	// can't be set until that function is called, either.
	assert(this->system != nullptr);
	assert(this->objects.size() == 0);
	assert(this->musicFormat == musicFormat);
	assert(this->soundFormat == soundFormat);
}

AudioManager::~AudioManager()
{
	// Release all stored objects.
	FMOD_RESULT result;
	for (auto &pair : this->objects)
	{
		result = FMOD_Sound_Release(pair.second);
		Debug::check(result == FMOD_OK, "Audio Manager", "FMOD_Sound_Release");
	}

	result = FMOD_System_Close(this->system);
	Debug::check(result == FMOD_OK, "Audio Manager", "FMOD_System_Close");
}

bool AudioManager::isLoaded(FMOD_SOUND *object) const
{
	return object != nullptr;
}

double AudioManager::getMusicVolume() const
{
	float volume;
	FMOD_RESULT result = FMOD_Channel_GetVolume(this->musicChannel, &volume);
	Debug::check(result == FMOD_OK, "Audio Manager", "getMusicVolume");

	assert(volume >= 0.0f);
	assert(volume <= 1.0f);

	return static_cast<double>(volume);
}

double AudioManager::getSoundVolume() const
{
	float volume;
	FMOD_RESULT result = FMOD_Channel_GetVolume(this->soundChannel, &volume);
	Debug::check(result == FMOD_OK, "Audio Manager", "getSoundVolume");

	assert(volume >= 0.0f);
	assert(volume <= 1.0f);

	return static_cast<double>(volume);
}

bool AudioManager::musicIsPlaying() const
{
	FMOD_BOOL playing;
	FMOD_RESULT result = FMOD_Channel_IsPlaying(this->musicChannel, &playing);
	Debug::check(result == FMOD_OK, "Audio Manager", "musicIsPlaying");
	return playing > 0;
}

void AudioManager::loadMusic(const std::string &filename)
{
	// Make a blank mapping to write into.
	this->objects.insert(std::pair<std::string, FMOD_SOUND*>(filename, nullptr));

	auto fullPath = AudioManager::MUSIC_PATH + filename +
		MusicFormatExtensions.at(this->musicFormat);

	// Load the music file into the blank mapping.
	FMOD_RESULT result = FMOD_System_CreateStream(this->system, fullPath.c_str(),
		FMOD_SOFTWARE, nullptr, &this->objects.at(filename));
	Debug::check(result == FMOD_OK, "Audio Manager",
		"loadMusic FMOD_System_CreateStream " + filename);
}

void AudioManager::loadSound(const std::string &filename)
{
	// Make a blank mapping to write into.
	this->objects.insert(std::pair<std::string, FMOD_SOUND*>(filename, nullptr));

	auto fullPath = AudioManager::SOUNDS_PATH + filename +
		SoundFormatExtensions.at(this->soundFormat);

	// Load the sound file into the blank mapping.
	FMOD_RESULT result = FMOD_System_CreateStream(this->system, fullPath.c_str(),
		FMOD_SOFTWARE, nullptr, &this->objects.at(filename));
	Debug::check(result == FMOD_OK, "Audio Manager",
		"loadSound FMOD_System_CreateStream " + filename);
}

void AudioManager::playMusic(const std::string &filename)
{
	if (this->objects.find(filename) != this->objects.end())
	{
		// Stop any currently playing music.
		this->stopMusic();

		// Play the new music.
		FMOD_RESULT result = FMOD_System_PlaySound(this->system, FMOD_CHANNEL_FREE,
			this->objects.at(filename), false, &this->musicChannel);
		Debug::check(result == FMOD_OK, "Audio Manager",
			"playMusic FMOD_System_PlaySound " + filename);

		// Set the music to loop.
		result = FMOD_Channel_SetMode(this->musicChannel, FMOD_LOOP_NORMAL);
		Debug::check(result == FMOD_OK, "Audio Manager",
			"playMusic FMOD_Channel_SetMode " + filename);
	}
	else
	{
		this->loadMusic(filename);
		assert(this->objects.find(filename) != this->objects.end());
		this->playMusic(filename);
	}
}

void AudioManager::playMusic(MusicName musicName)
{
	this->playMusic(MusicFilenames.at(musicName));
}

void AudioManager::playSound(const std::string &filename)
{
	if (this->objects.find(filename) != this->objects.end())
	{
		FMOD_RESULT result = FMOD_System_PlaySound(this->system, FMOD_CHANNEL_FREE,
			this->objects.at(filename), false, &this->soundChannel);
		Debug::check(result == FMOD_OK, "Audio Manager",
			"playSound FMOD_System_PlaySound" + filename);
	}
	else
	{
		this->loadSound(filename);
		assert(this->objects.find(filename) != this->objects.end());
		this->playSound(filename);
	}
}

void AudioManager::playSound(SoundName soundName)
{
	this->playSound(SoundFilenames.at(soundName));
}

void AudioManager::toggleMusic()
{
	FMOD_BOOL p;
	FMOD_Channel_GetPaused(this->musicChannel, &p);
	FMOD_Channel_SetPaused(this->musicChannel, !p);
}

void AudioManager::stopMusic()
{
	FMOD_Channel_Stop(this->musicChannel);
}

void AudioManager::stopSound()
{
	FMOD_Channel_Stop(this->soundChannel);
}

void AudioManager::setMusicVolume(double percent)
{
	float volume = static_cast<float>(std::max(AudioManager::MIN_VOLUME,
		std::min(AudioManager::MAX_VOLUME, percent)));

	assert(volume >= 0.0f);
	assert(volume <= 1.0f);

	FMOD_RESULT result = FMOD_Channel_SetVolume(this->musicChannel, volume);
	Debug::check(result == FMOD_OK, "Audio Manager",
		"setMusicVolume FMOD_Channel_SetVolume " + std::to_string(percent));
}

void AudioManager::setSoundVolume(double percent)
{
	float volume = static_cast<float>(std::max(AudioManager::MIN_VOLUME,
		std::min(AudioManager::MAX_VOLUME, percent)));

	assert(volume >= 0.0f);
	assert(volume <= 1.0f);

	FMOD_RESULT result = FMOD_Channel_SetVolume(this->soundChannel, volume);
	Debug::check(result == FMOD_OK, "Audio Manager",
		"setSoundVolume FMOD_Channel_SetVolume " + std::to_string(percent));
}
