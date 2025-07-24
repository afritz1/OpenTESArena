#ifndef VOXEL_SHAPE_DEFINITION_H
#define VOXEL_SHAPE_DEFINITION_H

#include <vector> // @todo: Buffer<T> would be better although there were annoyances with the deleted copy constructor

#include "../Assets/ArenaTypes.h"
#include "../Math/MathUtils.h"
#include "../World/ArenaMeshUtils.h"

#include "components/utilities/Span.h"

enum class VoxelShapeType
{
	Box
};

// For voxels that are affected differently by ceiling scale (e.g. raised platforms and water/lava chasms).
enum class VoxelShapeScaleType
{
	ScaledFromMin,
	UnscaledFromMin,
	UnscaledFromMax
};

struct VoxelBoxShapeDefinition
{
	double width, height, depth;
	double yOffset; // Elevation above bottom of voxel.
	Radians yRotation; // For diagonal walls.

	void init(double width, double height, double depth, double yOffset, Radians yRotation);

	bool operator==(const VoxelBoxShapeDefinition &other) const;
};

// For rendering.
struct VoxelMeshDefinition
{
	static constexpr int MAX_DRAW_CALLS = 6; // One per voxel face.

	std::vector<double> rendererPositions, rendererNormals, rendererTexCoords;
	std::vector<int32_t> indicesLists[MAX_DRAW_CALLS];
	VoxelFacing3D facings[MAX_DRAW_CALLS]; // Up to 6 voxel faces, associated with index buffers, used with face combining.
	bool fullFacingCoverages[MAX_DRAW_CALLS]; // Each voxel face that is physically covered by the mesh.
	int textureSlotIndices[MAX_DRAW_CALLS]; // Maps index buffer to its voxel texture definition slot.
	int indicesListCount;
	int facingCount;
	int textureSlotIndexCount;

	VoxelMeshDefinition();

	void initClassic(const ArenaShapeInitCache &shapeInitCache, VoxelShapeScaleType scaleType, double ceilingScale);

	bool isEmpty() const;

	// Finds the index buffer (if any) associated with the voxel facing. Does not have to fully cover the voxel face, just has
	// to represent that particular surface normal. Used with mesh combining.
	int findIndexBufferIndexWithFacing(VoxelFacing3D facing) const;

	int findTextureSlotIndexWithFacing(VoxelFacing3D facing) const;
	bool hasFullCoverageOfFacing(VoxelFacing3D facing) const;
};

// Provides geometry for physics and rendering.
struct VoxelShapeDefinition
{
	VoxelShapeType type;

	union
	{
		VoxelBoxShapeDefinition box;
	};

	VoxelMeshDefinition mesh;
	VoxelShapeScaleType scaleType;
	bool allowsBackFaces; // Back face culling for rendering.
	bool allowsAdjacentDoorFaces; // For voxels that don't prevent a door's face from rendering.
	bool allowsInternalFaceRemoval; // For voxels that can disable their faces when blocked by an opaque neighbor's face.
	bool allowsAdjacentFaceCombining; // For voxels that can combine their faces with adjacent voxel faces in the same plane to create a larger mesh.
	bool enablesNeighborGeometry; // For voxels that influence adjacent context-sensitive voxels like chasms.
	bool isContextSensitive; // For voxels like chasms whose geometry is conditional to what's around them.
	bool isElevatedPlatform; // For voxels that entities sit on top of and for letting player sleep in peace.

	VoxelShapeDefinition();

	void initBoxFromClassic(const ArenaShapeInitCache &shapeInitCache, VoxelShapeScaleType scaleType, double ceilingScale);
};

#endif
