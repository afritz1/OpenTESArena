#ifndef VOXEL_CHUNK_H
#define VOXEL_CHUNK_H

#include <climits>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "VoxelChasmWallInstance.h"
#include "VoxelDirtyType.h"
#include "VoxelDoorAnimationInstance.h"
#include "VoxelDoorDefinition.h"
#include "VoxelDoorVisibilityInstance.h"
#include "VoxelFadeAnimationInstance.h"
#include "VoxelShadingDefinition.h"
#include "VoxelShapeDefinition.h"
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

class AudioManager;

using VoxelShapeDefID = int;
using VoxelTextureDefID = int;
using VoxelShadingDefID = int;
using VoxelTraitsDefID = int;
using VoxelTransitionDefID = int;
using VoxelTriggerDefID = int;
using VoxelLockDefID = int;
using VoxelBuildingNameID = int;
using VoxelDoorDefID = int;
using VoxelChasmDefID = int;

// Gameplay values for a 3D set of voxels occupying a 64x64 portion of the game world.
struct VoxelChunk final : public Chunk
{
	// Definitions pointed to by voxel IDs.
	std::vector<VoxelShapeDefinition> shapeDefs;
	std::vector<VoxelTextureDefinition> textureDefs;
	std::vector<VoxelShadingDefinition> shadingDefs;
	std::vector<VoxelTraitsDefinition> traitsDefs;
	std::vector<TransitionDefinition> transitionDefs;
	std::vector<VoxelTriggerDefinition> triggerDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<std::string> buildingNames;
	std::vector<VoxelDoorDefinition> doorDefs;

	// Indices into definitions for actual voxels in-game.
	Buffer3D<VoxelShapeDefID> shapeDefIDs;
	Buffer3D<VoxelTextureDefID> textureDefIDs;
	Buffer3D<VoxelShadingDefID> shadingDefIDs;
	Buffer3D<VoxelTraitsDefID> traitsDefIDs;
	VoxelShapeDefID floorReplacementShapeDefID;
	VoxelTextureDefID floorReplacementTextureDefID;
	VoxelShadingDefID floorReplacementShadingDefID;
	VoxelTraitsDefID floorReplacementTraitsDefID;
	VoxelChasmDefID floorReplacementChasmDefID;

	// Voxels that changed this frame. Reset at end-of-frame.
	Buffer3D<VoxelDirtyType> dirtyVoxelTypes;
	std::vector<VoxelInt3> dirtyShapeDefPositions;
	std::vector<VoxelInt3> dirtyFaceActivationPositions;
	std::vector<VoxelInt3> dirtyDoorAnimInstPositions; // Either animating or just closed this frame.
	std::vector<VoxelInt3> dirtyDoorVisInstPositions;
	std::vector<VoxelInt3> dirtyFadeAnimInstPositions; // Either animating or just finished this frame.

	// Indices into decorators (generally sparse in comparison to voxels themselves).
	std::unordered_map<VoxelInt3, VoxelTransitionDefID> transitionDefIndices;
	std::unordered_map<VoxelInt3, VoxelTriggerDefID> triggerDefIndices;
	std::unordered_map<VoxelInt3, VoxelLockDefID> lockDefIndices;
	std::unordered_map<VoxelInt3, VoxelBuildingNameID> buildingNameIndices;
	std::unordered_map<VoxelInt3, VoxelDoorDefID> doorDefIndices;
	std::unordered_map<VoxelInt3, VoxelChasmDefID> chasmDefIndices;

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

	static constexpr VoxelShapeDefID AIR_SHAPE_DEF_ID = 0;
	static constexpr VoxelTextureDefID AIR_TEXTURE_DEF_ID = 0;
	static constexpr VoxelShadingDefID AIR_SHADING_DEF_ID = 0;
	static constexpr VoxelTraitsDefID AIR_TRAITS_DEF_ID = 0;

	VoxelChunk();

	void init(const ChunkInt2 &position, int height);

	// Gets the voxel definitions adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	// This is slightly different than the chunk manager's version since it is chunk-independent (but as
	// a result, voxels on a chunk edge must be updated by the chunk manager).
	void getAdjacentShapeDefIDs(const VoxelInt3 &voxel, VoxelShapeDefID *outNorthID,
		VoxelShapeDefID *outEastID, VoxelShapeDefID *outSouthID, VoxelShapeDefID *outWestID) const;
	void getAdjacentTextureDefIDs(const VoxelInt3 &voxel, VoxelTextureDefID *outNorthID,
		VoxelTextureDefID *outEastID, VoxelTextureDefID *outSouthID, VoxelTextureDefID *outWestID) const;
	void getAdjacentShadingDefIDs(const VoxelInt3 &voxel, VoxelShadingDefID *outNorthID,
		VoxelShadingDefID *outEastID, VoxelShadingDefID *outSouthID, VoxelShadingDefID *outWestID) const;
	void getAdjacentTraitsDefIDs(const VoxelInt3 &voxel, VoxelTraitsDefID *outNorthID,
		VoxelTraitsDefID *outEastID, VoxelTraitsDefID *outSouthID, VoxelTraitsDefID *outWestID) const;

	// Sets this voxel dirty for geometry updating, etc. if not already.
	void trySetVoxelDirtyInternal(SNInt x, int y, WEInt z, std::vector<VoxelInt3> &dirtyPositions, VoxelDirtyType dirtyType);
	void setShapeDefDirty(SNInt x, int y, WEInt z);
	void setFaceActivationDirty(SNInt x, int y, WEInt z);
	void setDoorAnimInstDirty(SNInt x, int y, WEInt z);
	void setDoorVisInstDirty(SNInt x, int y, WEInt z);
	void setFadeAnimInstDirty(SNInt x, int y, WEInt z);

	bool tryGetTransitionDefID(SNInt x, int y, WEInt z, VoxelTransitionDefID *outID) const;
	bool tryGetTriggerDefID(SNInt x, int y, WEInt z, VoxelTriggerDefID *outID) const;
	bool tryGetLockDefID(SNInt x, int y, WEInt z, VoxelLockDefID *outID) const;
	bool tryGetBuildingNameID(SNInt x, int y, WEInt z, VoxelBuildingNameID *outID) const;
	bool tryGetDoorDefID(SNInt x, int y, WEInt z, VoxelDoorDefID *outID) const;
	bool tryGetChasmDefID(SNInt x, int y, WEInt z, VoxelChasmDefID *outID) const;

	// Animation instance getters. A destroyed instance is still valid to read until end-of-frame.
	bool tryGetDoorAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	bool tryGetFadeAnimInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;

	bool tryGetChasmWallInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	bool tryGetDoorVisibilityInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;
	bool tryGetTriggerInstIndex(SNInt x, int y, WEInt z, int *outIndex) const;

	void setShapeDefID(SNInt x, int y, WEInt z, VoxelShapeDefID id);
	void setTextureDefID(SNInt x, int y, WEInt z, VoxelTextureDefID id);
	void setShadingDefID(SNInt x, int y, WEInt z, VoxelShadingDefID id);
	void setTraitsDefID(SNInt x, int y, WEInt z, VoxelTraitsDefID id);

	VoxelShapeDefID addShapeDef(VoxelShapeDefinition &&shapeDef);
	VoxelTextureDefID addTextureDef(VoxelTextureDefinition &&textureDef);
	VoxelShadingDefID addShadingDef(VoxelShadingDefinition &&shadingDef);
	VoxelTraitsDefID addTraitsDef(VoxelTraitsDefinition &&traitsDef);
	VoxelTransitionDefID addTransitionDef(TransitionDefinition &&transition);
	VoxelTriggerDefID addTriggerDef(VoxelTriggerDefinition &&trigger);
	VoxelLockDefID addLockDef(LockDefinition &&lock);
	VoxelBuildingNameID addBuildingName(std::string &&buildingName);
	VoxelDoorDefID addDoorDef(VoxelDoorDefinition &&door);

	void addTransitionDefPosition(VoxelTransitionDefID id, const VoxelInt3 &voxel);
	void addTriggerDefPosition(VoxelTriggerDefID id, const VoxelInt3 &voxel);
	void addLockDefPosition(VoxelLockDefID id, const VoxelInt3 &voxel);
	void addBuildingNamePosition(VoxelBuildingNameID id, const VoxelInt3 &voxel);
	void addDoorDefPosition(VoxelDoorDefID id, const VoxelInt3 &voxel);
	void addChasmDefPosition(VoxelChasmDefID id, const VoxelInt3 &voxel);

	void addDirtyFaceActivationPosition(const VoxelInt3 &voxel);
	void addDirtyDoorVisInstPosition(const VoxelInt3 &voxel);

	void addDoorAnimInst(VoxelDoorAnimationInstance &&animInst);
	void addFadeAnimInst(VoxelFadeAnimationInstance &&animInst);

	void addChasmWallInst(VoxelChasmWallInstance &&inst);
	void addDoorVisibilityInst(VoxelDoorVisibilityInstance &&inst);
	void addTriggerInst(VoxelTriggerInstance &&inst);
	void removeChasmWallInst(const VoxelInt3 &voxel);

	// Simulates the chunk's voxels by delta time.
	// @todo: evaluate just letting the chunk manager do all the updating for the chunk, due to the complexity
	// of chunk perimeters, etc. and the amount of almost-identical problem solving between the two classes.
	void updateDoorAnimInsts(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager);
	void updateFadeAnimInsts(double dt);

	// End-of-frame clean-up.
	void endFrame();

	// Clears all chunk state.
	void clear();
};

#endif
