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
#include "DoorDefinition.h"
#include "VoxelChasmWallInstance.h"
#include "VoxelDoorAnimationInstance.h"
#include "VoxelDoorVisibilityInstance.h"
#include "VoxelFadeAnimationInstance.h"
#include "VoxelMeshDefinition.h"
#include "VoxelTextureDefinition.h"
#include "VoxelTraitsDefinition.h"
#include "VoxelTriggerDefinition.h"
#include "VoxelTriggerInstance.h"
#include "VoxelUtils.h"
#include "../Math/MathUtils.h"
#include "../World/Chunk.h"
#include "../World/ChunkUtils.h"
#include "../World/Coord.h"
#include "../World/LockDefinition.h"
#include "../World/TransitionDefinition.h"

#include "components/utilities/Buffer3D.h"

// A 3D set of voxels for a portion of the game world.

class AudioManager;

class VoxelChunk final : public Chunk
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
	// Definitions pointed to by voxel IDs.
	std::vector<VoxelMeshDefinition> voxelMeshDefs;
	std::vector<VoxelTextureDefinition> voxelTextureDefs;
	std::vector<VoxelTraitsDefinition> voxelTraitsDefs;
	std::vector<TransitionDefinition> transitionDefs;
	std::vector<VoxelTriggerDefinition> triggerDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<std::string> buildingNames;
	std::vector<DoorDefinition> doorDefs;
	std::vector<ChasmDefinition> chasmDefs;

	// Indices into definitions for actual voxels in-game.
	Buffer3D<VoxelMeshDefID> voxelMeshDefIDs;
	Buffer3D<VoxelTextureDefID> voxelTextureDefIDs;
	Buffer3D<VoxelTraitsDefID> voxelTraitsDefIDs;

	// Voxels that changed this frame. Reset at end-of-frame.
	Buffer3D<bool> dirtyVoxels;
	std::vector<VoxelInt3> dirtyVoxelPositions;

	// Indices into decorators (generally sparse in comparison to voxels themselves).
	std::unordered_map<VoxelInt3, TransitionDefID> transitionDefIndices;
	std::unordered_map<VoxelInt3, TriggerDefID> triggerDefIndices;
	std::unordered_map<VoxelInt3, LockDefID> lockDefIndices;
	std::unordered_map<VoxelInt3, BuildingNameID> buildingNameIndices;
	std::unordered_map<VoxelInt3, DoorDefID> doorDefIndices;
	std::unordered_map<VoxelInt3, ChasmDefID> chasmDefIndices;

	// Animations.
	std::vector<VoxelDoorAnimationInstance> doorAnimInsts;
	std::vector<VoxelFadeAnimationInstance> fadeAnimInsts;

	// Unique voxel states.
	std::vector<VoxelChasmWallInstance> chasmWallInsts;
	std::vector<VoxelDoorVisibilityInstance> doorVisInsts;
	std::vector<VoxelTriggerInstance> triggerInsts;

	// Gets the voxel definitions adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	// This is slightly different than the chunk manager's version since it is chunk-independent (but as
	// a result, voxels on a chunk edge must be updated by the chunk manager).
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
	static constexpr VoxelMeshDefID AIR_VOXEL_MESH_DEF_ID = 0;
	static constexpr VoxelTextureDefID AIR_VOXEL_TEXTURE_DEF_ID = 0;
	static constexpr VoxelTraitsDefID AIR_VOXEL_TRAITS_DEF_ID = 0;

	void init(const ChunkInt2 &position, int height);

	int getVoxelMeshDefCount() const;
	int getVoxelTextureDefCount() const;
	int getVoxelTraitsDefCount() const;
	int getTransitionDefCount() const;
	int getTriggerDefCount() const;
	int getLockDefCount() const;
	int getBuildingNameDefCount() const;
	int getDoorDefCount() const;
	int getChasmDefCount() const;

	// Gets the definition associated with a voxel def ID (can iterate with an index too).
	const VoxelMeshDefinition &getVoxelMeshDef(VoxelMeshDefID id) const;
	const VoxelTextureDefinition &getVoxelTextureDef(VoxelTextureDefID id) const;
	const VoxelTraitsDefinition &getVoxelTraitsDef(VoxelTraitsDefID id) const;
	const TransitionDefinition &getTransitionDef(TransitionDefID id) const;
	const VoxelTriggerDefinition &getTriggerDef(TriggerDefID id) const;
	const LockDefinition &getLockDef(LockDefID id) const;
	const std::string &getBuildingName(BuildingNameID id) const;
	const DoorDefinition &getDoorDef(DoorDefID id) const;
	const ChasmDefinition &getChasmDef(ChasmDefID id) const;

	VoxelMeshDefID getVoxelMeshDefID(SNInt x, int y, WEInt z) const;
	VoxelTextureDefID getVoxelTextureDefID(SNInt x, int y, WEInt z) const;
	VoxelTraitsDefID getVoxelTraitsDefID(SNInt x, int y, WEInt z) const;

	int getDirtyVoxelCount() const;
	const VoxelInt3 &getDirtyVoxel(int index) const;

	bool tryGetTransitionDefID(SNInt x, int y, WEInt z, TransitionDefID *outID) const;
	bool tryGetTriggerDefID(SNInt x, int y, WEInt z, TriggerDefID *outID) const;
	bool tryGetLockDefID(SNInt x, int y, WEInt z, LockDefID *outID) const;
	bool tryGetBuildingNameID(SNInt x, int y, WEInt z, BuildingNameID *outID) const;
	bool tryGetDoorDefID(SNInt x, int y, WEInt z, DoorDefID *outID) const;
	bool tryGetChasmDefID(SNInt x, int y, WEInt z, ChasmDefID *outID) const;

	int getDoorAnimInstCount() const;
	int getFadeAnimInstCount() const;
	const VoxelDoorAnimationInstance &getDoorAnimInst(int index) const;
	bool tryGetDoorAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	const VoxelFadeAnimationInstance &getFadeAnimInst(int index) const;
	bool tryGetFadeAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;

	int getChasmWallInstCount() const;
	int getDoorVisibilityInstCount() const;
	int getTriggerInstCount() const;
	VoxelChasmWallInstance &getChasmWallInst(int index);
	const VoxelChasmWallInstance &getChasmWallInst(int index) const;
	bool tryGetChasmWallInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	VoxelDoorVisibilityInstance &getDoorVisibilityInst(int index);
	const VoxelDoorVisibilityInstance &getDoorVisibilityInst(int index) const;
	bool tryGetDoorVisibilityInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	const VoxelTriggerInstance &getTriggerInst(int index) const;
	bool tryGetTriggerInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;

	void setVoxelMeshDefID(SNInt x, int y, WEInt z, VoxelMeshDefID id);
	void setVoxelTextureDefID(SNInt x, int y, WEInt z, VoxelTextureDefID id);
	void setVoxelTraitsDefID(SNInt x, int y, WEInt z, VoxelTraitsDefID id);

	VoxelMeshDefID addVoxelMeshDef(VoxelMeshDefinition &&voxelMeshDef);
	VoxelTextureDefID addVoxelTextureDef(VoxelTextureDefinition &&voxelTextureDef);
	VoxelTraitsDefID addVoxelTraitsDef(VoxelTraitsDefinition &&voxelTraitsDef);
	TransitionDefID addTransitionDef(TransitionDefinition &&transition);
	TriggerDefID addTriggerDef(VoxelTriggerDefinition &&trigger);
	LockDefID addLockDef(LockDefinition &&lock);
	BuildingNameID addBuildingName(std::string &&buildingName);
	DoorDefID addDoorDef(DoorDefinition &&door);
	ChasmDefID addChasmDef(ChasmDefinition &&chasm);

	void addTransitionDefPosition(TransitionDefID id, const VoxelInt3 &voxel);
	void addTriggerDefPosition(TriggerDefID id, const VoxelInt3 &voxel);
	void addLockDefPosition(LockDefID id, const VoxelInt3 &voxel);
	void addBuildingNamePosition(BuildingNameID id, const VoxelInt3 &voxel);
	void addDoorDefPosition(DoorDefID id, const VoxelInt3 &voxel);
	void addChasmDefPosition(ChasmDefID id, const VoxelInt3 &voxel);

	void addDoorAnimInst(VoxelDoorAnimationInstance &&animInst);
	void addFadeAnimInst(VoxelFadeAnimationInstance &&animInst);

	void addChasmWallInst(VoxelChasmWallInstance &&inst);
	void addDoorVisibilityInst(VoxelDoorVisibilityInstance &&inst);
	void addTriggerInst(VoxelTriggerInstance &&inst);
	void removeChasmWallInst(const VoxelInt3 &voxel);

	// Clears all chunk state.
	void clear();

	// Clears the dirty voxels list, meant to be done at the end of a frame.
	void clearDirtyVoxels();

	// Simulates the chunk's voxels by delta time.
	// @todo: evaluate just letting the chunk manager do all the updating for the chunk, due to the complexity
	// of chunk perimeters, etc. and the amount of almost-identical problem solving between the two classes.
	void update(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager);
};

#endif
