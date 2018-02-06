#include <array>
#include <cassert>
#include <map>

#include "WeaponAnimation.h"
#include "../Assets/ExeStrings.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

namespace
{
	// Mappings of weapon animation states to ranges of frame indices, excluding fists.
	const std::map<WeaponAnimation::State, std::vector<int>> WeaponAnimationRanges =
	{
		{ WeaponAnimation::State::Sheathed, { } },
		{ WeaponAnimation::State::Unsheathing, { 30, 31, 32 } },
		{ WeaponAnimation::State::Idle, { 32 } },
		{ WeaponAnimation::State::Forward, { 25, 26, 27, 28, 29 } },
		{ WeaponAnimation::State::Down, { 0, 1, 2, 3, 4 } },
		{ WeaponAnimation::State::Right, { 15, 16, 17, 18, 19 } },
		{ WeaponAnimation::State::Left, { 10, 11, 12, 13, 14 } },
		{ WeaponAnimation::State::DownRight, { 20, 21, 22, 23, 24 } },
		{ WeaponAnimation::State::DownLeft, { 5, 6, 7, 8, 9 } },
		{ WeaponAnimation::State::Sheathing, { 32, 31, 30 } }
	};

	// Mappings of fists animation states to ranges of frame indices.
	const std::map<WeaponAnimation::State, std::vector<int>> FistsAnimationRanges =
	{
		{ WeaponAnimation::State::Sheathed, { } },
		{ WeaponAnimation::State::Unsheathing, { 10, 11, 12 } },
		{ WeaponAnimation::State::Idle, { 12 } },
		{ WeaponAnimation::State::Forward, { 5, 6, 7, 8, 9 } },
		{ WeaponAnimation::State::Down, { 0, 1, 2, 3, 4 } },
		{ WeaponAnimation::State::Right, { 5, 6, 7, 8, 9 } },
		{ WeaponAnimation::State::Left, { 0, 1, 2, 3, 4 } },
		{ WeaponAnimation::State::DownRight, { 5, 6, 7, 8, 9 } },
		{ WeaponAnimation::State::DownLeft, { 0, 1, 2, 3, 4 } },
		{ WeaponAnimation::State::Sheathing, { 12, 11, 10 } }
	};
}

const double WeaponAnimation::DEFAULT_TIME_PER_FRAME = 1.0 / 16.0;
const int WeaponAnimation::FISTS_ID = -1;

WeaponAnimation::WeaponAnimation(int weaponID, const ExeStrings &exeStrings)
{
	// Bows are not allowed yet.
	const bool weaponIsRanged = (weaponID == 16) || (weaponID == 17);
	DebugAssert(!weaponIsRanged, "Bow animations not implemented.");

	this->state = WeaponAnimation::State::Sheathed;
	this->weaponID = weaponID;
	this->animationFilename = [weaponID, &exeStrings]()
	{
		// Get the filename associated with the weapon ID. These indices point into the
		// filenames list.
		const std::array<int, 18> WeaponFilenameMappings =
		{
			0, // Staff
			1, // Dagger
			1, // Shortsword
			1, // Broadsword
			1, // Saber
			1, // Longsword
			1, // Claymore
			1, // Tanto
			1, // Wakizashi
			1, // Katana
			1, // Dai-katana
			2, // Mace
			3, // Flail
			4, // War hammer
			5, // War axe
			5, // Battle axe
			6, // Short bow
			6 // Long bow
		};

		const int fistsFilenameIndex = 7;
		const int index = (weaponID != WeaponAnimation::FISTS_ID) ?
			WeaponFilenameMappings.at(weaponID) : fistsFilenameIndex;

		const std::vector<std::string> &animationList =
			exeStrings.getList(ExeStringKey::WeaponAnimationFilenames);
		const std::string &filename = animationList.at(index);
		return String::toUppercase(filename);
	}();

	this->currentTime = 0.0;
	this->timePerFrame = WeaponAnimation::DEFAULT_TIME_PER_FRAME;
	this->rangeIndex = 0;
}

WeaponAnimation::~WeaponAnimation()
{

}

const std::vector<int> &WeaponAnimation::getCurrentRange() const
{
	const std::vector<int> &indices = (this->weaponID == WeaponAnimation::FISTS_ID) ?
		FistsAnimationRanges.at(this->state) : WeaponAnimationRanges.at(this->state);
	return indices;
}

bool WeaponAnimation::isSheathed() const
{
	return this->state == WeaponAnimation::State::Sheathed;
}

bool WeaponAnimation::isIdle() const
{
	return this->state == WeaponAnimation::State::Idle;
}

const std::string &WeaponAnimation::getAnimationFilename() const
{
	return this->animationFilename;
}

int WeaponAnimation::getFrameIndex() const
{
	// The sheathe animation's frame index should not be used.
	assert(!this->isSheathed());

	const std::vector<int> &indices = this->getCurrentRange();
	return indices.at(this->rangeIndex);
}

void WeaponAnimation::setState(WeaponAnimation::State state)
{
	// Switch to the beginning of the new range of indices. The combination of
	// the state and range index will return a frame index. Do not retrieve the
	// frame index when in the sheathed state.
	this->state = state;
	this->rangeIndex = 0;
}

void WeaponAnimation::tick(double dt)
{
	// Tick if not idle and not sheathed.
	if ((this->state != WeaponAnimation::State::Idle) &&
		(this->state != WeaponAnimation::State::Sheathed))
	{
		this->currentTime += dt;

		// Update the index if current time has passed the time per frame.
		while (this->currentTime >= this->timePerFrame)
		{
			this->currentTime -= this->timePerFrame;
			this->rangeIndex++;

			// Get the current range of frame indices.
			const std::vector<int> &indices = this->getCurrentRange();

			// If the index is outside the range, decide which state is next.
			if (this->rangeIndex >= indices.size())
			{
				// Start at the beginning of the new range. The range index is not
				// used in the sheathed state.
				this->rangeIndex = 0;

				if (this->state == WeaponAnimation::State::Sheathing)
				{
					// Switching from sheathing to sheathed.
					this->state = WeaponAnimation::State::Sheathed;
				}
				else
				{
					// Switching from unsheathing to idle, or from swing to idle.
					this->state = WeaponAnimation::State::Idle;
				}
			}
		}
	}
}
