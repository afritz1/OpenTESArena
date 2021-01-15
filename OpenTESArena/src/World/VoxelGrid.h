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
	SNInt width;
	int height;
	WEInt depth;

	// Converts XYZ coordinate to index.
	int getIndex(SNInt x, int y, WEInt z) const;
public:
	VoxelGrid(SNInt width, int height, WEInt depth);

	// Gets the dimensions of the voxel grid.
	SNInt getWidth() const;
	int getHeight() const;
	WEInt getDepth() const;

	// Returns whether the given coordinate lies within the voxel grid.
	bool coordIsValid(SNInt x, int y, WEInt z) const;

	// Convenience method for getting a voxel's ID.
	uint16_t getVoxel(SNInt x, int y, WEInt z) const;

	int getVoxelDefCount() const;

	// Gets the voxel definitions associated with an ID.
	VoxelDefinition &getVoxelDef(uint16_t id);
	const VoxelDefinition &getVoxelDef(uint16_t id) const;
	
	// Finds a voxel definition ID that matches the predicate, or none if not found.
	std::optional<uint16_t> findVoxelDef(const VoxelDefPredicate &predicate) const;

	// Adds a voxel definition and returns its assigned ID.
	uint16_t addVoxelDef(const VoxelDefinition &voxelDef);

	// Convenience method for setting a voxel's ID.
	void setVoxel(SNInt x, int y, WEInt z, uint16_t id);
};

#endif
