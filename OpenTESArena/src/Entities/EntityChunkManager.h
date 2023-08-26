#ifndef ENTITY_CHUNK_MANAGER_H
#define ENTITY_CHUNK_MANAGER_H

#include "CitizenUtils.h"
#include "EntityAnimationDefinition.h"
#include "EntityAnimationInstance.h"
#include "EntityChunk.h"
#include "EntityGeneration.h"
#include "EntityInstance.h"
#include "EntityUtils.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/RecyclablePool.h"

class AudioManager;
class BinaryAssetLibrary;
class EntityDefinitionLibrary;
class LevelDefinition;
class LevelInfoDefinition;
class Player;
class Renderer;
class TextureManager;
class VoxelChunk;
class VoxelChunkManager;

struct EntityVisibilityState2D;
struct EntityVisibilityState3D;
struct MapSubDefinition;

class EntityChunkManager final : public SpecializedChunkManager<EntityChunk>
{
private:
	using EntityPool = RecyclablePool<EntityInstance, EntityInstanceID>;
	using EntityPositionPool = RecyclablePool<CoordDouble2, EntityPositionID>;
	using EntityBoundingBoxPool = RecyclablePool<double, EntityBoundingBoxID>;
	using EntityDirectionPool = RecyclablePool<VoxelDouble2, EntityDirectionID>;
	using EntityAnimationInstancePool = RecyclablePool<EntityAnimationInstance, EntityAnimationInstanceID>;
	using EntityCreatureSoundPool = RecyclablePool<double, EntityCreatureSoundInstanceID>;
	using EntityCitizenDirectionIndexPool = RecyclablePool<int8_t, EntityCitizenDirectionIndexID>;
	using EntityPaletteIndicesInstancePool = RecyclablePool<PaletteIndices, EntityPaletteIndicesInstanceID>;

	EntityPool entities;
	EntityPositionPool positions;
	EntityBoundingBoxPool boundingBoxes;
	EntityDirectionPool directions;
	EntityAnimationInstancePool animInsts;
	EntityCreatureSoundPool creatureSoundInsts;
	EntityCitizenDirectionIndexPool citizenDirectionIndices;

	// Each citizen has a unique palette indirection in place of unique textures for memory savings. It was found
	// that hardly any citizen instances share textures due to variations in their random palette. As a result,
	// citizen textures will need to be 8-bit.
	EntityPaletteIndicesInstancePool paletteIndices;

	// Entity definitions for this currently-active level. Their definition IDs CANNOT be assumed
	// to be zero-based because these are in addition to ones in the entity definition library.
	// @todo: separate EntityAnimationDefinition from EntityDefinition?
	std::unordered_map<EntityDefID, EntityDefinition> entityDefs;

	// Entities that should have their instance resources freed, either because the chunk they were in
	// was unloaded, or they were otherwise despawned. Cleared at end-of-frame.
	std::vector<EntityInstanceID> destroyedEntityIDs;

	EntityDefID addEntityDef(EntityDefinition &&def, const EntityDefinitionLibrary &defLibrary);
	EntityDefID getOrAddEntityDefID(const EntityDefinition &def, const EntityDefinitionLibrary &defLibrary);

	EntityInstanceID spawnEntity();

	void populateChunkEntities(EntityChunk &entityChunk, const VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const WorldInt2 &levelOffset,
		const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);
	void populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk, const LevelDefinition &levelDef,
		const LevelInfoDefinition &levelInfoDef, const MapSubDefinition &mapSubDef, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double ceilingScale,
		Random &random, const EntityDefinitionLibrary &entityDefLibrary, const BinaryAssetLibrary &binaryAssetLibrary,
		TextureManager &textureManager, Renderer &renderer);

	void updateCitizenStates(double dt, EntityChunk &entityChunk, const CoordDouble2 &playerCoordXZ, bool isPlayerMoving,
		bool isPlayerWeaponSheathed, Random &random, const VoxelChunkManager &voxelChunkManager);

	std::string getCreatureSoundFilename(const EntityDefID defID) const;
	void updateCreatureSounds(double dt, EntityChunk &entityChunk, const CoordDouble3 &playerCoord,
		double ceilingScale, Random &random, AudioManager &audioManager);
public:
	const EntityDefinition &getEntityDef(EntityDefID defID) const;
	const EntityInstance &getEntity(EntityInstanceID id) const;
	const CoordDouble2 &getEntityPosition(EntityPositionID id) const;
	double getEntityBoundingBox(EntityBoundingBoxID id) const;
	const VoxelDouble2 &getEntityDirection(EntityDirectionID id) const;
	EntityAnimationInstance &getEntityAnimationInstance(EntityAnimationInstanceID id);
	const EntityAnimationInstance &getEntityAnimationInstance(EntityAnimationInstanceID id) const;
	const int8_t &getEntityCitizenDirectionIndex(EntityCitizenDirectionIndexID id) const;
	const PaletteIndices &getEntityPaletteIndices(EntityPaletteIndicesInstanceID id) const;

	// Gets the entities scheduled for destruction this frame. If they're in this list, they should no longer be
	// simulated or rendered.
	BufferView<const EntityInstanceID> getQueuedDestroyEntityIDs() const;

	// Count functions for specialized entities.
	int getCountInChunkWithDirection(const ChunkInt2 &chunkPos) const;
	int getCountInChunkWithCreatureSound(const ChunkInt2 &chunkPos) const;
	int getCountInChunkWithCitizenDirection(const ChunkInt2 &chunkPos) const;

	// Gets the entity visibility data necessary for rendering and ray cast selection.
	void getEntityVisibilityState2D(EntityInstanceID id, const CoordDouble2 &eye2D, EntityVisibilityState2D &outVisState) const;
	void getEntityVisibilityState3D(EntityInstanceID id, const CoordDouble2 &eye2D, double ceilingScale,
		const VoxelChunkManager &voxelChunkManager, EntityVisibilityState3D &outVisState) const;

	void update(double dt, const BufferView<const ChunkInt2> &activeChunkPositions,
		const BufferView<const ChunkInt2> &newChunkPositions, const BufferView<const ChunkInt2> &freedChunkPositions,
		const Player &player, const LevelDefinition *activeLevelDef, const LevelInfoDefinition *activeLevelInfoDef,
		const MapSubDefinition &mapSubDef, BufferView<const LevelDefinition> levelDefs,
		BufferView<const int> levelInfoDefIndices, BufferView<const LevelInfoDefinition> levelInfoDefs,
		const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		double ceilingScale, Random &random, const VoxelChunkManager &voxelChunkManager, AudioManager &audioManager,
		TextureManager &textureManager, Renderer &renderer);

	// Prepares an entity for destruction later this frame.
	void queueEntityDestroy(EntityInstanceID entityInstID);

	// @todo: support spawning an entity not from the level def

	void cleanUp();
	void clear();
};

#endif
