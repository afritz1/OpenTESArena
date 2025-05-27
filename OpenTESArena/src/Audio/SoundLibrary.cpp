#include <algorithm>

#include "SoundLibrary.h"
#include "../Assets/ArenaSoundName.h"

#include "components/debug/Debug.h"

void SoundLibrary::init()
{
	// @todo: remove ArenaSoundName and load from SoundDefinitions.txt instead, which would allow new VOC files
	const char *soundFilenames[] =
	{
		// Combat.
		ArenaSoundName::ArrowFire,
		ArenaSoundName::ArrowHit,
		ArenaSoundName::Bash,
		ArenaSoundName::BodyFall,
		ArenaSoundName::Clank,
		ArenaSoundName::EnemyHit,
		ArenaSoundName::FemaleDie,
		ArenaSoundName::MaleDie,
		ArenaSoundName::NHit,
		ArenaSoundName::PlayerHit,
		ArenaSoundName::Swish,

		// Crime.
		ArenaSoundName::Halt,
		ArenaSoundName::StopThief,

		// Fanfare.
		ArenaSoundName::Fanfare1,
		ArenaSoundName::Fanfare2,

		// Movement.
		ArenaSoundName::DirtLeft,
		ArenaSoundName::DirtRight,
		ArenaSoundName::MudLeft,
		ArenaSoundName::MudRight,
		ArenaSoundName::SnowLeft,
		ArenaSoundName::SnowRight,
		ArenaSoundName::Splash,
		ArenaSoundName::Swim,

		// Spells.
		ArenaSoundName::Burst,
		ArenaSoundName::Explode,
		ArenaSoundName::SlowBall,

		// Weather.
		ArenaSoundName::Thunder
	};

	for (const char *filename : soundFilenames)
	{
		this->filenames.emplace_back(std::string(filename));
	}

	std::sort(this->filenames.begin(), this->filenames.end());
}

int SoundLibrary::getFilenameCount() const
{
	return static_cast<int>(this->filenames.size());
}

const char *SoundLibrary::getFilename(int index) const
{
	DebugAssertIndex(this->filenames, index);
	return this->filenames[index].c_str();
}
