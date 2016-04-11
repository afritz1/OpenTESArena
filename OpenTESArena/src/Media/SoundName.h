#ifndef SOUND_NAME_H
#define SOUND_NAME_H

// A unique identifier for each sound file, including speech.

// Since there are only a couple characters with unique walking sounds, those
// entities can use their sound, but all others will use the generic walking sounds.

enum class SoundName
{
	// Ambient
	Back1,
	Birds,
	Birds2,
	Clicks,
	DeepChoi,
	Drip1,
	Drip2,
	Drums,
	Eerie,
	HiChoi,
	HumEerie,
	Scream1,
	Scream2,
	Thunder,
	Wind,

	// Combat
	ArrowFire,
	ArrowHit,
	Bash,
	BodyFall,
	Clank,
	EnemyHit,
	FemaleDie,
	MaleDie,
	NHit,
	PlayerHit,
	Swish,

	// Crime
	Halt,
	StopThief,

	// Doors
	CloseDoor,
	Grind,
	Lock,
	OpenAlt,
	OpenDoor,
	Portcullis,

	// Entities
	Rat,
	SnowWolf,
	Spider,
	Troll,
	Wolf,

	Goblin,
	LizardMan,
	LizardManWalk,
	Medusa,
	Minotaur,
	Orc,

	IceGolem,
	IronGolem,
	IronGolemWalk,
	StoneGolem,

	FireDaemon,
	HellHound,
	HellHoundWalk,
	Homonculus,

	Ghost,
	Ghoul,
	Lich,
	Skeleton,
	Vampire,
	Wraith,
	Zombie,

	// Fanfare
	Fanfare1,
	Fanfare2,

	// Interface
	Burst,

	// Movement
	DirtLeft,
	DirtRight,
	MudLeft,
	MudRight,
	SnowLeft,
	SnowRight,
	Splash,
	Swim,

	// Speech
	EmperorThanks,
	EmperorReward,
	SilmaneIntro,
	SilmanePlayerDeath,
	SilmaneJourneyBegin,
	SilmaneFirstSleepQuest,
	SilmaneFirstPieceObtained,
	SilmaneSecondPieceObtained,
	SilmaneThirdPieceObtained,
	SilmaneFourthPieceObtained,
	SilmaneFifthPieceObtained,
	SilmaneSixthPieceObtained,
	SilmaneSeventhPieceObtained,
	SilmaneFinalPieceObtained,
	TharnPlayerDeath,
	TharnFirstPieceObtained,
	TharnSecondPieceObtained,
	TharnThirdPieceObtained,
	TharnFourthPieceObtained,
	TharnFifthPieceObtained,
	TharnSixthPieceObtained,
	TharnSeventhPieceObtained,
	TharnFinalPieceObtained,
	TharnFinalBattle,
	TharnAngryJewel,

	// Spells
	Explode,
	SlowBall
};

#endif
