#include <array>

#include "DoorSoundLibrary.h"
#include "../Assets/ExeData.h"

#include "components/debug/Debug.h"

namespace
{
	constexpr std::array<ArenaTypes::DoorType, 4> DoorTypes =
	{
		ArenaTypes::DoorType::Swinging,
		ArenaTypes::DoorType::Sliding,
		ArenaTypes::DoorType::Raising,
		ArenaTypes::DoorType::Splitting
	};

	std::optional<int> TryGetOpenSoundIndex(ArenaTypes::DoorType type)
	{
		if (type == ArenaTypes::DoorType::Swinging)
		{
			return 6;
		}
		else if (type == ArenaTypes::DoorType::Sliding)
		{
			return 14;
		}
		else if (type == ArenaTypes::DoorType::Raising)
		{
			return 15;
		}
		else
		{
			return std::nullopt;
		}
	}

	std::optional<int> TryGetCloseSoundIndex(ArenaTypes::DoorType type)
	{
		if (type == ArenaTypes::DoorType::Swinging)
		{
			return 5;
		}
		else if (type == ArenaTypes::DoorType::Sliding)
		{
			return 14;
		}
		else if (type == ArenaTypes::DoorType::Raising)
		{
			return 15;
		}
		else
		{
			return std::nullopt;
		}
	}

	std::optional<DoorSoundDefinition::CloseType> TryGetCloseSoundType(ArenaTypes::DoorType type)
	{
		if (type == ArenaTypes::DoorType::Swinging)
		{
			return DoorSoundDefinition::CloseType::OnClosed;
		}
		else if (type == ArenaTypes::DoorType::Sliding)
		{
			return DoorSoundDefinition::CloseType::OnClosing;
		}
		else if (type == ArenaTypes::DoorType::Raising)
		{
			return DoorSoundDefinition::CloseType::OnClosing;
		}
		else
		{
			return std::nullopt;
		}
	}
}

void DoorSoundLibrary::init()
{
	// Generate open and close definitions for each door type. Unfortunately this library can't
	// store sound filenames because the mappings are defined per-level in the game (therefore,
	// maybe door sounds should be defined in LevelInfoDefinition instead of this library).
	for (const ArenaTypes::DoorType doorType : DoorTypes)
	{
		const std::optional<int> openSoundIndex = TryGetOpenSoundIndex(doorType);
		if (openSoundIndex.has_value())
		{
			DoorSoundDefinition openSoundDef;
			openSoundDef.initOpen(doorType, *openSoundIndex);
			this->defs.emplace_back(std::move(openSoundDef));
		}

		const std::optional<int> closeSoundIndex = TryGetCloseSoundIndex(doorType);
		const std::optional<DoorSoundDefinition::CloseType> closeSoundType = TryGetCloseSoundType(doorType);
		if (closeSoundIndex.has_value() && closeSoundType.has_value())
		{
			DoorSoundDefinition closeSoundDef;
			closeSoundDef.initClose(doorType, *closeSoundType, *closeSoundIndex);
			this->defs.emplace_back(std::move(closeSoundDef));
		}
	}
}

int DoorSoundLibrary::getDefCount() const
{
	return static_cast<int>(this->defs.size());
}

const DoorSoundDefinition &DoorSoundLibrary::getDef(int index) const
{
	DebugAssertIndex(this->defs, index);
	return this->defs[index];
}

std::optional<int> DoorSoundLibrary::tryGetDefIndex(ArenaTypes::DoorType doorType,
	DoorSoundDefinition::Type type) const
{
	for (int i = 0; i < static_cast<int>(this->defs.size()); i++)
	{
		const DoorSoundDefinition &def = this->defs[i];
		if ((def.getDoorType() == doorType) && (def.getType() == type))
		{
			return i;
		}
	}

	return std::nullopt;
}
