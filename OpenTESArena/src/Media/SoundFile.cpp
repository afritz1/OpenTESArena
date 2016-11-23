#include <unordered_map>

#include "SoundFile.h"

#include "SoundName.h"

namespace std
{
	// Hash specialization, since GCC doesn't support enum classes used as keys
	// in unordered_maps.
	template <>
	struct hash<SoundName>
	{
		size_t operator()(const SoundName &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

namespace
{
	// Each SoundName has a corresponding filename. A number of them have
	// their name mixed up with another in the original files. I'm not sure
	// if the entity "Walk" sounds are used (maybe just the iron golem's).
	// - Unused/duplicate sounds: MOON.VOC, SNARL2.VOC, UMPH.VOC, WHINE.VOC.
	const std::unordered_map<SoundName, std::string> SoundFilenames =
	{
		// Ambient.
		{ SoundName::Back1, "BACK1.VOC" },
		{ SoundName::Birds, "BIRDS.VOC" },
		{ SoundName::Birds2, "BIRDS2.VOC" },
		{ SoundName::Clicks, "CLICKS.VOC" },
		{ SoundName::DeepChoir, "DEEPCHOI.VOC" },
		{ SoundName::Drip1, "DRIP1.VOC" },
		{ SoundName::Drip2, "DRIP2.VOC" },
		{ SoundName::Drums, "DRUMS.VOC" },
		{ SoundName::Eerie, "EERIE.VOC" },
		{ SoundName::HighChoir, "HICHOIR.VOC" },
		{ SoundName::HumEerie, "HUMEERIE.VOC" },
		{ SoundName::Scream1, "SCREAM1.VOC" },
		{ SoundName::Scream2, "SCREAM2.VOC" },
		{ SoundName::Thunder, "THUNDER.VOC" },
		{ SoundName::Wind, "WIND.VOC" },

		// Combat.
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

		// Crime.
		{ SoundName::Halt, "HALT.VOC" },
		{ SoundName::StopThief, "STPTHIEF.VOC" },

		// Doors.
		{ SoundName::CloseDoor, "CLOSDOOR.VOC" },
		{ SoundName::Grind, "GRIND.VOC" },
		{ SoundName::Lock, "LOCK.VOC" },
		{ SoundName::OpenAlt, "OPENALT.VOC" },
		{ SoundName::OpenDoor, "OPENDOOR.VOC" },
		{ SoundName::Portcullis, "PORTC.VOC" },

		// Entities.
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

		// Fanfare.
		{ SoundName::Fanfare1, "FANFARE1.VOC" },
		{ SoundName::Fanfare2, "FANFARE2.VOC" },

		// Movement.
		{ SoundName::DirtLeft, "DIRTL.VOC" },
		{ SoundName::DirtRight, "DIRTR.VOC" },
		{ SoundName::MudLeft, "MUDSTE.VOC" },
		{ SoundName::MudRight, "MUDSTEP.VOC" },
		{ SoundName::SnowLeft, "SNOWL.VOC" },
		{ SoundName::SnowRight, "SNOWR.VOC" },
		{ SoundName::Splash, "SPLASH.VOC" },
		{ SoundName::Swim, "SWIMMING.VOC" },

		// Spells.
		{ SoundName::Burst, "BURST5.VOC" },
		{ SoundName::Explode, "EXPLODE.VOC" },
		{ SoundName::SlowBall, "SLOWBALL.VOC" }
	};
}

const std::string &SoundFile::fromName(SoundName soundName)
{
	const std::string &filename = SoundFilenames.at(soundName);
	return filename;
}
