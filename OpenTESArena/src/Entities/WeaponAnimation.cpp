#include <algorithm>
#include <array>
#include <cassert>
#include <unordered_map>

#include "WeaponAnimation.h"
#include "../Assets/ExeData.h"
#include "../Utilities/Debug.h"
#include "../Utilities/String.h"

namespace std
{
	// Hash specialization, required until GCC 6.1.
	template <>
	struct hash<WeaponAnimation::State>
	{
		size_t operator()(const WeaponAnimation::State &x) const
		{
			return static_cast<size_t>(x);
		}
	};
}

namespace
{
	// Mappings of melee weapon animation states to ranges of frame indices.
	const std::unordered_map<WeaponAnimation::State, std::vector<int>> MeleeAnimationRanges =
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
	const std::unordered_map<WeaponAnimation::State, std::vector<int>> FistsAnimationRanges =
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

	// Mappings of bow animation states to ranges of frame indices. Sheathing and unsheathing
	// are instantaneous, so they are not stored here.
	const std::unordered_map<WeaponAnimation::State, std::vector<int>> BowAnimationRanges =
	{
		{ WeaponAnimation::State::Sheathed, { } },
		{ WeaponAnimation::State::Idle, { 0 } },
		{ WeaponAnimation::State::Firing,{ 1 } }
	};
}

const double WeaponAnimation::DEFAULT_TIME_PER_FRAME = 1.0 / 16.0;
const int WeaponAnimation::FISTS_ID = -1;

WeaponAnimation::WeaponAnimation(int weaponID, const ExeData &exeData)
{
	this->state = WeaponAnimation::State::Sheathed;
	this->weaponID = weaponID;
	this->animationFilename = [weaponID, &exeData]()
	{
		// Get the filename associated with the weapon ID. These indices point into the
		// filenames list.
		const std::array<int, 18> WeaponFilenameIndices =
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
			WeaponFilenameIndices.at(weaponID) : fistsFilenameIndex;

		const auto &animationList = exeData.equipment.weaponAnimationFilenames;
		const std::string &filename = animationList.at(index);
		return String::toUppercase(filename);
	}();

	this->currentTime = 0.0;
	this->rangeIndex = 0;
}

double WeaponAnimation::getTimePerFrame() const
{
	if (this->isRanged())
	{
		// The ranged animation should never be in a sheathing or unsheathing state
		// because both are instant (technically their times would be 0.0, but it's
		// implemented differently -- see setState()).
		assert(this->state != WeaponAnimation::State::Unsheathing);
		assert(this->state != WeaponAnimation::State::Sheathing);

		return WeaponAnimation::DEFAULT_TIME_PER_FRAME *
			((this->state == WeaponAnimation::State::Firing) ? 7.0 : 1.0);
	}
	else
	{
		// Melee weapons and fists.
		return WeaponAnimation::DEFAULT_TIME_PER_FRAME;
	}
}

const std::vector<int> &WeaponAnimation::getCurrentRange() const
{
	// Find the range mapped to the weapon and the current animation state.
	if (this->weaponID == WeaponAnimation::FISTS_ID)
	{
		return FistsAnimationRanges.at(this->state);
	}
	else if (this->isRanged())
	{
		return BowAnimationRanges.at(this->state);
	}
	else
	{
		return MeleeAnimationRanges.at(this->state);
	}
}

bool WeaponAnimation::isRanged() const
{
	return (this->weaponID == 16) || (this->weaponID == 17);
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
	// Check that the given state is valid for the weapon animation.
	if (this->isRanged())
	{
		// Ranged weapons use a strict subset of the animation states.
		const std::array<WeaponAnimation::State, 5> AllowedRangedStates =
		{
			WeaponAnimation::State::Sheathed,
			WeaponAnimation::State::Unsheathing,
			WeaponAnimation::State::Idle,
			WeaponAnimation::State::Firing,
			WeaponAnimation::State::Sheathing
		};

		assert(std::find(AllowedRangedStates.begin(),
			AllowedRangedStates.end(), state) != AllowedRangedStates.end());
	}
	else
	{
		// Melee weapons cannot use the firing state.
		assert(state != WeaponAnimation::State::Firing);
	}	

	// Switch to the beginning of the new range of indices. The combination of
	// the state and range index will return a frame index. Do not retrieve the
	// frame index when in the sheathed state.
	this->state = [this, state]()
	{
		// If the animation is ranged, skip states that would otherwise be instant.
		if (this->isRanged())
		{
			if (state == WeaponAnimation::State::Unsheathing)
			{
				// Skip to idle.
				return WeaponAnimation::State::Idle;
			}
			else if (state == WeaponAnimation::State::Sheathing)
			{
				// Skip to sheathed.
				return WeaponAnimation::State::Sheathed;
			}
			else
			{
				return state;
			}
		}
		else
		{
			// Melee animations do not skip any states.
			return state;
		}
	}();

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
		while (this->currentTime >= this->getTimePerFrame())
		{
			this->currentTime -= this->getTimePerFrame();
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
					// Switching from unsheathing to idle, or from swing/fire to idle.
					this->state = WeaponAnimation::State::Idle;
				}
			}
		}
	}
}
