#ifndef INTERIOR_UTILS_H
#define INTERIOR_UTILS_H

#include <optional>

#include "VoxelDefinition.h"
#include "../Assets/ArenaTypes.h"

enum class InteriorType;

namespace InteriorUtils
{
	// Helper function while transitioning to using InteriorType completely.
	std::optional<InteriorType> menuTypeToInteriorType(ArenaTypes::MenuType menuType);

	bool isPrefabInterior(InteriorType interiorType);
	bool isProceduralInterior(InteriorType interiorType);
}

#endif
