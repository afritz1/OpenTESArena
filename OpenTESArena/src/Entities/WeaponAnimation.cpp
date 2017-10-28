#include <cassert>
#include <map>

#include "WeaponAnimation.h"
#include "../Items/WeaponType.h"
#include "../Utilities/Debug.h"

namespace
{
	// Mappings of weapon types to .CIF filenames.
	const std::map<WeaponType, std::string> WeaponTypeFilenames =
	{
		{ WeaponType::BattleAxe, "AXE" },
		{ WeaponType::Broadsword, "SWORD" },
		{ WeaponType::Claymore, "SWORD" },
		{ WeaponType::Dagger, "SWORD" },
		{ WeaponType::DaiKatana, "SWORD" },
		{ WeaponType::Fists, "HAND" },
		{ WeaponType::Flail, "STAR" },
		{ WeaponType::Katana, "SWORD" },
		{ WeaponType::LongBow, "" }, // Bows have no animations.
		{ WeaponType::Longsword, "SWORD" },
		{ WeaponType::Mace, "MACE" },
		{ WeaponType::Saber, "SWORD" },
		{ WeaponType::ShortBow, "" }, // Bows have no animations.
		{ WeaponType::Shortsword, "SWORD" },
		{ WeaponType::Staff, "STAFF" },
		{ WeaponType::Tanto, "SWORD" },
		{ WeaponType::Wakizashi, "SWORD" },
		{ WeaponType::WarAxe, "AXE" },
		{ WeaponType::Warhammer, "HAMMER" }
	};

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

WeaponAnimation::WeaponAnimation(WeaponType weaponType)
{
	// Bows are not allowed because they have no animation.
	DebugAssert((weaponType != WeaponType::LongBow) && (weaponType != WeaponType::ShortBow),
		"Bows do not have weapon animations.");

	this->state = WeaponAnimation::State::Sheathed;
	this->weaponType = weaponType;
	this->currentTime = 0.0;
	this->timePerFrame = WeaponAnimation::DEFAULT_TIME_PER_FRAME;
	this->rangeIndex = 0;
}

WeaponAnimation::~WeaponAnimation()
{

}

const std::vector<int> &WeaponAnimation::getCurrentRange() const
{
	const std::vector<int> &indices = (this->weaponType == WeaponType::Fists) ?
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
	const std::string &filename = WeaponTypeFilenames.at(this->weaponType);
	return filename;
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
