#ifndef AUTOMAP_UI_MODEL_H
#define AUTOMAP_UI_MODEL_H

#include "../Math/Vector2.h"
#include "../World/Coord.h"

namespace AutomapUiModel
{
	// Calculates automap screen offset in pixels for rendering.
	Double2 makeAutomapOffset(const VoxelInt2 &playerVoxel);

	// Helper function for obtaining relative wild origin in new coordinate system.
	WorldInt2 makeRelativeWildOrigin(const WorldInt2 &voxel, SNInt gridWidth, WEInt gridDepth);
}

#endif
