#include "ArenaChasmUtils.h"

#include "components/debug/Debug.h"

bool ArenaChasmUtils::allowsSwimming(ArenaTypes::ChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaTypes::ChasmType::Dry:
		return false;
	case ArenaTypes::ChasmType::Wet:
	case ArenaTypes::ChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}

bool ArenaChasmUtils::isDamaging(ArenaTypes::ChasmType chasmType)
{
	switch (chasmType)
	{
	case ArenaTypes::ChasmType::Dry:
	case ArenaTypes::ChasmType::Wet:
		return false;
	case ArenaTypes::ChasmType::Lava:
		return true;
	default:
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(chasmType)));
	}
}
