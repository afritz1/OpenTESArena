#ifndef AUTOMAP_UI_MODEL_H
#define AUTOMAP_UI_MODEL_H

#include "../Math/Vector2.h"
#include "../World/Coord.h"

namespace AutomapUiModel
{
	// How fast the automap moves when scrolling.
	constexpr double ScrollSpeed = 100.0;

	// Calculates automap screen offset in pixels for rendering.
	Double2 makeAutomapOffset(const VoxelInt2 &playerVoxel);

	// Helper function for obtaining relative wild origin in new coordinate system.
	NewInt2 makeRelativeWildOrigin(const NewInt2 &voxel, SNInt gridWidth, WEInt gridDepth);
}

#endif
