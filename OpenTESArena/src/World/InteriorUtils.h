#ifndef INTERIOR_UTILS_H
#define INTERIOR_UTILS_H

#include <optional>

#include "VoxelDefinition.h"

enum class InteriorType;

namespace InteriorUtils
{
	// Helper function while transitioning to using InteriorType completely.
	std::optional<InteriorType> menuTypeToInteriorType(VoxelDefinition::WallData::MenuType menuType);
}

#endif
