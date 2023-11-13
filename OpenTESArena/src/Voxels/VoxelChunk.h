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
#include "VoxelDirtyType.h"
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
	std::vector<VoxelMeshDefinition> meshDefs;
	std::vector<VoxelTextureDefinition> textureDefs;
	std::vector<VoxelTraitsDefinition> traitsDefs;
	std::vector<TransitionDefinition> transitionDefs;
	std::vector<VoxelTriggerDefinition> triggerDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<std::string> buildingNames;
	std::vector<DoorDefinition> doorDefs;
	std::vector<ChasmDefinition> chasmDefs;

	// Indices into definitions for actual voxels in-game.
	Buffer3D<VoxelMeshDefID> meshDefIDs;
	Buffer3D<VoxelTextureDefID> textureDefIDs;
	Buffer3D<VoxelTraitsDefID> traitsDefIDs;
	VoxelMeshDefID floorReplacementMeshDefID;
	VoxelTextureDefID floorReplacementTextureDefID;
	VoxelTraitsDefID floorReplacementTraitsDefID;
	ChasmDefID floorReplacementChasmDefID;

	// Voxels that changed this frame. Reset at end-of-frame.
	Buffer3D<VoxelDirtyType> dirtyVoxelTypes;
	std::vector<VoxelInt3> dirtyMeshDefPositions;
	std::vector<VoxelInt3> dirtyDoorAnimInstPositions;
	std::vector<VoxelInt3> dirtyDoorVisInstPositions;
	std::vector<VoxelInt3> dirtyFadeAnimInstPositions;
	std::vector<VoxelInt3> dirtyChasmWallInstPositions;

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

	// Destroyed animations to be cleaned up at end of frame. This was added so it's less confusing when
	// a voxel says it was dirtied (by animating) but there is no anim inst available.
	std::vector<VoxelInt3> destroyedDoorAnimInsts;
	std::vector<VoxelInt3> destroyedFadeAnimInsts;

	// Gets the voxel definitions adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	// This is slightly different than the chunk manager's version since it is chunk-independent (but as
	// a result, voxels on a chunk edge must be updated by the chunk manager).
	void getAdjacentMeshDefIDs(const VoxelInt3 &voxel, VoxelMeshDefID *outNorthID,
		VoxelMeshDefID *outEastID, VoxelMeshDefID *outSouthID, VoxelMeshDefID *outWestID);
	void getAdjacentTextureDefIDs(const VoxelInt3 &voxel, VoxelTextureDefID *outNorthID,
		VoxelTextureDefID *outEastID, VoxelTextureDefID *outSouthID, VoxelTextureDefID *outWestID);
	void getAdjacentTraitsDefIDs(const VoxelInt3 &voxel, VoxelTraitsDefID *outNorthID,
		VoxelTraitsDefID *outEastID, VoxelTraitsDefID *outSouthID, VoxelTraitsDefID *outWestID);

	// Sets this voxel dirty for geometry updating, etc. if not already.
	void trySetVoxelDirtyInternal(SNInt x, int y, WEInt z, std::vector<VoxelInt3> &dirtyPositions, VoxelDirtyType dirtyType);
	void setMeshDefDirty(SNInt x, int y, WEInt z);
	void setDoorAnimInstDirty(SNInt x, int y, WEInt z);
	void setDoorVisInstDirty(SNInt x, int y, WEInt z);
	void setFadeAnimInstDirty(SNInt x, int y, WEInt z);
	void setChasmWallInstDirty(SNInt x, int y, WEInt z);
public:
	static constexpr VoxelMeshDefID AIR_MESH_DEF_ID = 0;
	static constexpr VoxelTextureDefID AIR_TEXTURE_DEF_ID = 0;
	static constexpr VoxelTraitsDefID AIR_TRAITS_DEF_ID = 0;

	VoxelChunk();

	void init(const ChunkInt2 &position, int height);

	int getMeshDefCount() const;
	int getTextureDefCount() const;
	int getTraitsDefCount() const;
	int getTransitionDefCount() const;
	int getTriggerDefCount() const;
	int getLockDefCount() const;
	int getBuildingNameDefCount() const;
	int getDoorDefCount() const;
	int getChasmDefCount() const;

	// Gets the definition associated with a voxel def ID (can iterate with an index too).
	const VoxelMeshDefinition &getMeshDef(VoxelMeshDefID id) const;
	const VoxelTextureDefinition &getTextureDef(VoxelTextureDefID id) const;
	const VoxelTraitsDefinition &getTraitsDef(VoxelTraitsDefID id) const;
	const TransitionDefinition &getTransitionDef(TransitionDefID id) const;
	const VoxelTriggerDefinition &getTriggerDef(TriggerDefID id) const;
	const LockDefinition &getLockDef(LockDefID id) const;
	const std::string &getBuildingName(BuildingNameID id) const;
	const DoorDefinition &getDoorDef(DoorDefID id) const;
	const ChasmDefinition &getChasmDef(ChasmDefID id) const;

	VoxelMeshDefID getMeshDefID(SNInt x, int y, WEInt z) const;
	VoxelTextureDefID getTextureDefID(SNInt x, int y, WEInt z) const;
	VoxelTraitsDefID getTraitsDefID(SNInt x, int y, WEInt z) const;

	BufferView<const VoxelInt3> getDirtyMeshDefPositions() const;
	BufferView<const VoxelInt3> getDirtyDoorAnimInstPositions() const; // Either animating or just closed this frame.
	BufferView<const VoxelInt3> getDirtyDoorVisInstPositions() const;
	BufferView<const VoxelInt3> getDirtyFadeAnimInstPositions() const; // Either animating or just finished this frame.
	BufferView<const VoxelInt3> getDirtyChasmWallInstPositions() const;

	bool tryGetTransitionDefID(SNInt x, int y, WEInt z, TransitionDefID *outID) const;
	bool tryGetTriggerDefID(SNInt x, int y, WEInt z, TriggerDefID *outID) const;
	bool tryGetLockDefID(SNInt x, int y, WEInt z, LockDefID *outID) const;
	bool tryGetBuildingNameID(SNInt x, int y, WEInt z, BuildingNameID *outID) const;
	bool tryGetDoorDefID(SNInt x, int y, WEInt z, DoorDefID *outID) const;
	bool tryGetChasmDefID(SNInt x, int y, WEInt z, ChasmDefID *outID) const;

	// Animation instance getters. A destroyed instance is still valid to read until end-of-frame.
	BufferView<const VoxelDoorAnimationInstance> getDoorAnimInsts() const;
	bool tryGetDoorAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	BufferView<const VoxelFadeAnimationInstance> getFadeAnimInsts() const;
	bool tryGetFadeAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;

	BufferView<VoxelChasmWallInstance> getChasmWallInsts();
	BufferView<const VoxelChasmWallInstance> getChasmWallInsts() const;
	bool tryGetChasmWallInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	BufferView<VoxelDoorVisibilityInstance> getDoorVisibilityInsts();
	BufferView<const VoxelDoorVisibilityInstance> getDoorVisibilityInsts() const;
	bool tryGetDoorVisibilityInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	BufferView<const VoxelTriggerInstance> getTriggerInsts() const;
	bool tryGetTriggerInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;

	void setMeshDefID(SNInt x, int y, WEInt z, VoxelMeshDefID id);
	void setTextureDefID(SNInt x, int y, WEInt z, VoxelTextureDefID id);
	void setTraitsDefID(SNInt x, int y, WEInt z, VoxelTraitsDefID id);
	
	void setFloorReplacementMeshDefID(VoxelMeshDefID id);
	void setFloorReplacementTextureDefID(VoxelTextureDefID id);
	void setFloorReplacementTraitsDefID(VoxelTraitsDefID id);
	void setFloorReplacementChasmDefID(ChasmDefID id);

	VoxelMeshDefID addMeshDef(VoxelMeshDefinition &&meshDef);
	VoxelTextureDefID addTextureDef(VoxelTextureDefinition &&textureDef);
	VoxelTraitsDefID addTraitsDef(VoxelTraitsDefinition &&traitsDef);
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

	void addDirtyChasmWallInstPosition(const VoxelInt3 &voxel);

	void addDoorAnimInst(VoxelDoorAnimationInstance &&animInst);
	void addFadeAnimInst(VoxelFadeAnimationInstance &&animInst);

	void addChasmWallInst(VoxelChasmWallInstance &&inst);
	void addDoorVisibilityInst(VoxelDoorVisibilityInstance &&inst);
	void addTriggerInst(VoxelTriggerInstance &&inst);
	void removeChasmWallInst(const VoxelInt3 &voxel);

	// Simulates the chunk's voxels by delta time.
	// @todo: evaluate just letting the chunk manager do all the updating for the chunk, due to the complexity
	// of chunk perimeters, etc. and the amount of almost-identical problem solving between the two classes.
	void update(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager);

	// End-of-frame clean-up.
	void cleanUp();

	// Clears all chunk state.
	void clear();
};

#endif
