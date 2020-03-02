#ifndef VOXEL_GRID_H
#define VOXEL_GRID_H

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "VoxelDefinition.h"
#include "../Math/Vector2.h"

// A voxel grid is a 3D array of voxel IDs with their associated voxel definitions.

// In very complex scenes with several different kinds of voxels (including chasms, etc.),
// there are over a few hundred unique voxel definitions, which mandates that the voxel
// type itself be at least unsigned 16-bit.

class VoxelGrid
{
public:
	using VoxelDefPredicate = std::function<bool(const VoxelDefinition&)>;
private:
	std::vector<uint16_t> voxels;
	std::vector<VoxelDefinition> voxelDefs;
	int width, height, depth;

	// Converts XYZ coordinate to index.
	int getIndex(int x, int y, int z) const;
public:
	VoxelGrid(int width, int height, int depth);

	// Transformation methods for converting voxel coordinates between Arena's format
	// (+X west, +Z south) and the new format (+X north, +Z east). This is a bi-directional
	// conversion (i.e., it works both ways. Not exactly sure why).
	static Int2 getTransformedCoordinate(const Int2 &voxel, int gridWidth, int gridDepth);
	static Double2 getTransformedCoordinate(const Double2 &voxel, int gridWidth, int gridDepth);

	// Gets the dimensions of the voxel grid.
	int getWidth() const;
	int getHeight() const;
	int getDepth() const;

	// Returns whether the given coordinate lies within the voxel grid.
	bool coordIsValid(int x, int y, int z) const;

	// Gets a pointer to the voxel grid data.
	uint16_t *getVoxels();
	const uint16_t *getVoxels() const;

	// Convenience method for getting a voxel's ID.
	uint16_t getVoxel(int x, int y, int z) const;

	// Gets the voxel definitions associated with an ID.
	VoxelDefinition &getVoxelDef(uint16_t id);
	const VoxelDefinition &getVoxelDef(uint16_t id) const;
	
	// Finds a voxel definition ID that matches the predicate, or none if not found.
	std::optional<uint16_t> findVoxelDef(const VoxelDefPredicate &predicate) const;

	// Adds a voxel definition and returns its assigned ID.
	uint16_t addVoxelDef(const VoxelDefinition &voxelDef);

	// Convenience method for setting a voxel's ID.
	void setVoxel(int x, int y, int z, uint16_t id);
};

#endif
