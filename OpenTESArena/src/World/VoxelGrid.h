#ifndef VOXEL_GRID_H
#define VOXEL_GRID_H

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "VoxelDefinition.h"
#include "VoxelUtils.h"
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
	NSInt width; // Width is north/south.
	int height;
	EWInt depth; // Depth is east/west.

	// Converts XYZ coordinate to index.
	int getIndex(NSInt x, int y, EWInt z) const;
public:
	VoxelGrid(NSInt width, int height, EWInt depth);

	// Gets the dimensions of the voxel grid.
	NSInt getWidth() const;
	int getHeight() const;
	EWInt getDepth() const;

	// Returns whether the given coordinate lies within the voxel grid.
	bool coordIsValid(NSInt x, int y, EWInt z) const;

	// Convenience method for getting a voxel's ID.
	uint16_t getVoxel(NSInt x, int y, EWInt z) const;

	// Gets the voxel definitions associated with an ID.
	VoxelDefinition &getVoxelDef(uint16_t id);
	const VoxelDefinition &getVoxelDef(uint16_t id) const;
	
	// Finds a voxel definition ID that matches the predicate, or none if not found.
	std::optional<uint16_t> findVoxelDef(const VoxelDefPredicate &predicate) const;

	// Adds a voxel definition and returns its assigned ID.
	uint16_t addVoxelDef(const VoxelDefinition &voxelDef);

	// Convenience method for setting a voxel's ID.
	void setVoxel(NSInt x, int y, EWInt z, uint16_t id);
};

#endif
