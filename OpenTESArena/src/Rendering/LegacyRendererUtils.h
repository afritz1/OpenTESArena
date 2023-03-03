#ifndef LEGACY_RENDERER_UTILS_H
#define LEGACY_RENDERER_UTILS_H

#include <limits>

#include "ArenaRenderUtils.h"
#include "../Assets/ArenaTypes.h"
#include "../Math/MathUtils.h"
#include "../Math/Vector3.h"
#include "../Utilities/Palette.h"

// Temp namespace for storing old code from the 2.5D ray caster, etc., to be deleted once no longer needed.

namespace LegacyRendererUtils
{
	// Clipping planes for Z coordinates.
	constexpr double NEAR_PLANE = 0.0001;
	constexpr double FAR_PLANE = 1000.0;

	// Angle of the sky gradient above the horizon, in degrees.
	constexpr Degrees SKY_GRADIENT_ANGLE = 30.0;

	// Max angle of distant clouds above the horizon, in degrees.
	constexpr Degrees DISTANT_CLOUDS_MAX_ANGLE = 25.0;

	constexpr double DEPTH_BUFFER_INFINITY = std::numeric_limits<double>::infinity();

	// Gets the color of a row in the sky gradient at some percent between the top and bottom.
	Double3 getSkyGradientRowColor(double gradientPercent, const Double3 *skyColors, int skyColorCount);

	// Gets the blended thunderstorm flash color for a percentage through the flash animation.
	Double3 getThunderstormFlashColor(double flashPercent, const Double3 *colors, int colorCount);

	/*// Gathers potential intersection data from a voxel containing a "diagonal 1" ID; the 
	// diagonal starting at (nearX, nearZ) and ending at (farX, farZ). Returns whether an 
	// intersection occurred within the voxel.
	bool findDiag1Intersection(const CoordInt2 &coord, const WorldDouble2 &nearPoint,
		const WorldDouble2 &farPoint, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a "diagonal 2" ID; the
	// diagonal starting at (farX, nearZ) and ending at (nearX, farZ). Returns whether an
	// intersection occurred within the voxel.
	bool findDiag2Intersection(const CoordInt2 &coord, const WorldDouble2 &nearPoint,
		const WorldDouble2 &farPoint, RayHit &hit);

	// Gathers potential intersection data from an initial voxel containing an edge ID. The
	// facing determines which edge of the voxel an intersection can occur on.
	bool findInitialEdgeIntersection(const CoordInt2 &coord, VoxelFacing2D edgeFacing,
		bool flipped, const WorldDouble2 &nearPoint, const WorldDouble2 &farPoint, const Camera &camera,
		const Ray &ray, RayHit &hit);

	// Gathers potential intersection data from a voxel containing an edge ID. The facing
	// determines which edge of the voxel an intersection can occur on. This function is separate
	// from the initial case since it's a trivial solution when the edge and near facings match.
	bool findEdgeIntersection(const CoordInt2 &coord, VoxelFacing2D edgeFacing,
		bool flipped, VoxelFacing2D nearFacing, const WorldDouble2 &nearPoint,
		const WorldDouble2 &farPoint, double nearU, const Camera &camera, const Ray &ray, RayHit &hit);

	// Helper method for findInitialDoorIntersection() for swinging doors.
	bool findInitialSwingingDoorIntersection(const CoordInt2 &coord, double percentOpen,
		const WorldDouble2 &nearPoint, const WorldDouble2 &farPoint, bool xAxis, const Camera &camera,
		const Ray &ray, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for the intersection.
	bool findInitialDoorIntersection(const CoordInt2 &coord, ArenaTypes::DoorType doorType,
		double percentOpen, const WorldDouble2 &nearPoint, const WorldDouble2 &farPoint, const Camera &camera,
		const Ray &ray, const ChunkManager &chunkManager, RayHit &hit);

	// Helper method for findDoorIntersection() for swinging doors.
	bool findSwingingDoorIntersection(const CoordInt2 &coord, double percentOpen,
		VoxelFacing2D nearFacing, const WorldDouble2 &nearPoint, const WorldDouble2 &farPoint,
		double nearU, RayHit &hit);

	// Gathers potential intersection data from a voxel containing a door ID. The door
	// type determines what kind of door formula to calculate for the intersection. Raising doors
	// are always hit, so they do not need a specialized method.
	bool findDoorIntersection(const CoordInt2 &coord, ArenaTypes::DoorType doorType,
		double percentOpen, VoxelFacing2D nearFacing, const WorldDouble2 &nearPoint,
		const WorldDouble2 &farPoint, double nearU, RayHit &hit);*/

	// Low-level fog matrix sampling function.
	template <int TextureWidth, int TextureHeight>
	uint8_t sampleFogMatrixTexture(const ArenaRenderUtils::FogMatrix &fogMatrix, double u, double v);

	// Low-level screen-space chasm texture sampling function.
	/*void sampleChasmTexture(const ChasmTexture &texture, double screenXPercent, double screenYPercent,
		double *r, double *g, double *b);*/

	Double3 screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection, Degrees fovY, double aspect);
}

#endif
