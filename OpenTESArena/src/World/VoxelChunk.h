#ifndef VOXEL_CHUNK_H
#define VOXEL_CHUNK_H

#include <array>
#include <climits>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "ChasmDefinition.h"
#include "ChunkUtils.h"
#include "Coord.h"
#include "DoorDefinition.h"
#include "LockDefinition.h"
#include "TransitionDefinition.h"
#include "TriggerDefinition.h"
#include "VoxelDoorAnimationInstance.h"
#include "VoxelFadeAnimationInstance.h"
#include "VoxelInstance.h"
#include "VoxelMeshDefinition.h"
#include "VoxelTextureDefinition.h"
#include "VoxelTraitsDefinition.h"
#include "VoxelUtils.h"
#include "../Math/MathUtils.h"

#include "components/utilities/Buffer3D.h"

// A 3D set of voxels for a portion of the game world.

class AudioManager;

class VoxelChunk
{
public:
	using VoxelMeshDefID = int;
	using VoxelTextureDefID = int;
	using VoxelTraitsDefID = int;
	using TransitionDefID = int;
	using TriggerDefID = int;
	using LockDefID = int;
	using BuildingNameID = int;
	using DoorDefID = int;
	using ChasmDefID = int;
private:
	// Voxel definitions, pointed to by voxel IDs.
	std::vector<VoxelMeshDefinition> voxelMeshDefs;
	std::vector<VoxelTextureDefinition> voxelTextureDefs;
	std::vector<VoxelTraitsDefinition> voxelTraitsDefs;

	// Indices into voxel definitions.
	Buffer3D<VoxelMeshDefID> voxelMeshDefIDs;
	Buffer3D<VoxelTextureDefID> voxelTextureDefIDs;
	Buffer3D<VoxelTraitsDefID> voxelTraitsDefIDs;

	// Voxels that changed this frame. Reset at end-of-frame.
	Buffer3D<bool> dirtyVoxels;
	std::vector<VoxelInt3> dirtyVoxelPositions;

	// Instance data for voxels that are uniquely different in some way.
	std::vector<VoxelInstance> voxelInsts;
	std::vector<VoxelDoorAnimationInstance> doorAnimInsts;
	std::vector<VoxelFadeAnimationInstance> fadeAnimInsts;

	// Decorators.
	std::vector<TransitionDefinition> transitionDefs;
	std::vector<TriggerDefinition> triggerDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<std::string> buildingNames;
	std::vector<DoorDefinition> doorDefs;
	std::vector<ChasmDefinition> chasmDefs;

	// Indices into decorators (generally sparse in comparison to voxels themselves).
	std::unordered_map<VoxelInt3, TransitionDefID> transitionDefIndices;
	std::unordered_map<VoxelInt3, TriggerDefID> triggerDefIndices;
	std::unordered_map<VoxelInt3, LockDefID> lockDefIndices;
	std::unordered_map<VoxelInt3, BuildingNameID> buildingNameIndices;
	std::unordered_map<VoxelInt3, DoorDefID> doorDefIndices;
	std::unordered_map<VoxelInt3, ChasmDefID> chasmDefIndices;

	// Chunk coordinates in the world.
	ChunkInt2 position;

	// Gets the voxel definitions adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	// This is slightly different than the chunk manager's version since it is chunk-independent (but as
	// a result, voxels on a chunk edge must be updated by the chunk manager).
	template <typename VoxelIdType>
	void getAdjacentVoxelIDsInternal(const VoxelInt3 &voxel, const Buffer3D<VoxelIdType> &voxelIDs,
		VoxelIdType defaultID, VoxelIdType *outNorthID, VoxelIdType *outEastID, VoxelIdType *outSouthID,
		VoxelIdType *outWestID);
	void getAdjacentVoxelMeshDefIDs(const VoxelInt3 &voxel, VoxelMeshDefID *outNorthID,
		VoxelMeshDefID *outEastID, VoxelMeshDefID *outSouthID, VoxelMeshDefID *outWestID);
	void getAdjacentVoxelTextureDefIDs(const VoxelInt3 &voxel, VoxelTextureDefID *outNorthID,
		VoxelTextureDefID *outEastID, VoxelTextureDefID *outSouthID, VoxelTextureDefID *outWestID);
	void getAdjacentVoxelTraitsDefIDs(const VoxelInt3 &voxel, VoxelTraitsDefID *outNorthID,
		VoxelTraitsDefID *outEastID, VoxelTraitsDefID *outSouthID, VoxelTraitsDefID *outWestID);

	// Sets this voxel dirty for geometry updating, etc. if not already.
	// @todo: should this take flags instead?
	void setVoxelDirty(SNInt x, int y, WEInt z);
public:
	static constexpr SNInt WIDTH = ChunkUtils::CHUNK_DIM;
	static constexpr WEInt DEPTH = WIDTH;
	static_assert(MathUtils::isPowerOf2(WIDTH));

	static constexpr VoxelMeshDefID AIR_VOXEL_MESH_DEF_ID = 0;
	static constexpr VoxelTextureDefID AIR_VOXEL_TEXTURE_DEF_ID = 0;
	static constexpr VoxelTraitsDefID AIR_VOXEL_TRAITS_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);

	int getHeight() const;

	// Gets the chunk's XY coordinate in the world.
	const ChunkInt2 &getPosition() const;

	// Returns whether the given voxel coordinate is in the chunk.
	bool isValidVoxel(SNInt x, int y, WEInt z) const;

	// Gets the voxel ID at the given coordinate.
	VoxelMeshDefID getVoxelMeshDefID(SNInt x, int y, WEInt z) const;
	VoxelTextureDefID getVoxelTextureDefID(SNInt x, int y, WEInt z) const;
	VoxelTraitsDefID getVoxelTraitsDefID(SNInt x, int y, WEInt z) const;

	int getVoxelMeshDefCount() const;
	int getVoxelTextureDefCount() const;
	int getVoxelTraitsDefCount() const;

	// Gets the voxel definition associated with a voxel def ID (equivalent to an index).
	const VoxelMeshDefinition &getVoxelMeshDef(VoxelMeshDefID id) const;
	const VoxelTextureDefinition &getVoxelTextureDef(VoxelTextureDefID id) const;
	const VoxelTraitsDefinition &getVoxelTraitsDef(VoxelTraitsDefID id) const;

	int getDirtyVoxelCount() const;
	const VoxelInt3 &getDirtyVoxel(int index) const;

	int getVoxelInstCount() const;
	VoxelInstance &getVoxelInst(int index);
	const VoxelInstance &getVoxelInst(int index) const;

	int getDoorAnimInstCount() const;
	const VoxelDoorAnimationInstance &getDoorAnimInst(int index) const;
	bool tryGetDoorAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;

	int getFadeAnimInstCount() const;
	const VoxelFadeAnimationInstance &getFadeAnimInst(int index) const;
	bool tryGetFadeAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;

	// Convenience functions for attempting to get a voxel instance at the given voxel.
	std::optional<int> tryGetVoxelInstIndex(const VoxelInt3 &voxel, VoxelInstance::Type type) const;
	VoxelInstance *tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type);
	const VoxelInstance *tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type) const;

	bool tryGetTransitionDefID(SNInt x, int y, WEInt z, TransitionDefID *outID) const;
	bool tryGetTriggerDefID(SNInt x, int y, WEInt z, TriggerDefID *outID) const;
	bool tryGetLockDefID(SNInt x, int y, WEInt z, LockDefID *outID) const;
	bool tryGetBuildingNameID(SNInt x, int y, WEInt z, BuildingNameID *outID) const;
	bool tryGetDoorDefID(SNInt x, int y, WEInt z, DoorDefID *outID) const;
	bool tryGetChasmDefID(SNInt x, int y, WEInt z, ChasmDefID *outID) const;

	int getTransitionDefCount() const;
	int getTriggerDefCount() const;
	int getLockDefCount() const;
	int getBuildingNameDefCount() const;
	int getDoorDefCount() const;
	int getChasmDefCount() const;

	const TransitionDefinition &getTransitionDef(TransitionDefID id) const;
	const TriggerDefinition &getTriggerDef(TriggerDefID id) const;
	const LockDefinition &getLockDef(LockDefID id) const;
	const std::string &getBuildingName(BuildingNameID id) const;
	const DoorDefinition &getDoorDef(DoorDefID id) const;
	const ChasmDefinition &getChasmDef(ChasmDefID id) const;

	// Sets the voxel at the given coordinate.
	void setVoxelMeshDefID(SNInt x, int y, WEInt z, VoxelMeshDefID id);
	void setVoxelTextureDefID(SNInt x, int y, WEInt z, VoxelTextureDefID id);
	void setVoxelTraitsDefID(SNInt x, int y, WEInt z, VoxelTraitsDefID id);

	// Adds a voxel definition and returns its assigned ID.
	VoxelMeshDefID addVoxelMeshDef(VoxelMeshDefinition &&voxelMeshDef);
	VoxelTextureDefID addVoxelTextureDef(VoxelTextureDefinition &&voxelTextureDef);
	VoxelTraitsDefID addVoxelTraitsDef(VoxelTraitsDefinition &&voxelTraitsDef);

	void addVoxelInst(VoxelInstance &&voxelInst);
	void addDoorAnimInst(VoxelDoorAnimationInstance &&animInst);
	void addFadeAnimInst(VoxelFadeAnimationInstance &&animInst);

	// Adds a chunk decorator definition to the chunk and returns its newly assigned ID.
	TransitionDefID addTransition(TransitionDefinition &&transition);
	TriggerDefID addTrigger(TriggerDefinition &&trigger);
	LockDefID addLock(LockDefinition &&lock);
	BuildingNameID addBuildingName(std::string &&buildingName);
	DoorDefID addDoorDef(DoorDefinition &&door);
	ChasmDefID addChasmDef(ChasmDefinition &&chasm);

	// Adds a mapping of the chunk decorator definition ID to the given voxel.
	void addTransitionPosition(TransitionDefID id, const VoxelInt3 &voxel);
	void addTriggerPosition(TriggerDefID id, const VoxelInt3 &voxel);
	void addLockPosition(LockDefID id, const VoxelInt3 &voxel);
	void addBuildingNamePosition(BuildingNameID id, const VoxelInt3 &voxel);
	void addDoorPosition(DoorDefID id, const VoxelInt3 &voxel);
	void addChasmPosition(ChasmDefID id, const VoxelInt3 &voxel);

	// Removes a certain type of voxel instance from the given voxel (if any). This might be useful when
	// updating a chunk edge due to adjacent chunks changing.
	void removeVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type);

	// Clears all chunk state.
	void clear();

	// Clears the dirty voxels list, meant to be done at the end of a frame.
	void clearDirtyVoxels();

	// Animates the chunk's voxels by delta time.
	// @todo: evaluate just letting the chunk manager do all the updating for the chunk, due to the complexity
	// of chunk perimeters, etc. and the amount of almost-identical problem solving between the two classes.
	void update(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager);
};

#endif
