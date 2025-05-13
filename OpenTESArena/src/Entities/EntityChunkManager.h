#ifndef ENTITY_CHUNK_MANAGER_H
#define ENTITY_CHUNK_MANAGER_H

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"

#include "CitizenUtils.h"
#include "EntityAnimationDefinition.h"
#include "EntityAnimationInstance.h"
#include "EntityChunk.h"
#include "EntityGeneration.h"
#include "EntityInstance.h"
#include "EntityUtils.h"
#include "../Items/ItemInventory.h"
#include "../Math/BoundingBox.h"
#include "../World/SpecializedChunkManager.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/RecyclablePool.h"

class AudioManager;
class BinaryAssetLibrary;
class EntityDefinitionLibrary;
class LevelDefinition;
class LevelInfoDefinition;
class Renderer;
class TextureManager;
class VoxelChunk;
class VoxelChunkManager;

struct EntityObservedResult;
struct MapSubDefinition;
struct Player;

struct EntityCitizenName
{
	char name[64];

	EntityCitizenName(const char *name);
	EntityCitizenName();
};

struct EntityInitInfo
{
	EntityDefID defID;
	WorldDouble3 feetPosition;
	char initialAnimStateIndex;
	bool isSensorCollider;
	bool canBeKilled;
	std::optional<Double2> direction;
	std::optional<int8_t> citizenDirectionIndex;
	std::optional<EntityCitizenName> citizenName;
	std::optional<uint16_t> citizenColorSeed;
	std::optional<int> raceID;
	bool hasInventory;
	bool hasCreatureSound;
	std::optional<bool> isLocked;

	EntityInitInfo();
};

struct EntityCombatState
{
	bool isDying;
	bool isDead;

	EntityCombatState();

	bool isInDeathState() const;
};

struct EntityLockState
{
	bool isLocked;

	EntityLockState();
};

// Generated when an entity moves between chunks so systems can update resource ownership.
struct EntityTransferResult
{
	EntityInstanceID id;
	ChunkInt2 oldChunkPos;
	ChunkInt2 newChunkPos;

	EntityTransferResult();
};

class EntityChunkManager final : public SpecializedChunkManager<EntityChunk>
{
private:
	using EntityPool = RecyclablePool<EntityInstance, EntityInstanceID>;
	using EntityPositionPool = RecyclablePool<WorldDouble3, EntityPositionID>;
	using EntityBoundingBoxPool = RecyclablePool<BoundingBox3D, EntityBoundingBoxID>;
	using EntityDirectionPool = RecyclablePool<Double2, EntityDirectionID>;
	using EntityAnimationInstancePool = RecyclablePool<EntityAnimationInstance, EntityAnimationInstanceID>;
	using EntityCombatStatePool = RecyclablePool<EntityCombatState, EntityCombatStateID>;
	using EntityCreatureSoundPool = RecyclablePool<double, EntityCreatureSoundInstanceID>;
	using EntityCitizenDirectionIndexPool = RecyclablePool<int8_t, EntityCitizenDirectionIndexID>;
	using EntityCitizenNamePool = RecyclablePool<EntityCitizenName, EntityCitizenNameID>;
	using EntityPaletteIndicesInstancePool = RecyclablePool<PaletteIndices, EntityPaletteIndicesInstanceID>;
	using EntityItemInventoryInstancePool = RecyclablePool<ItemInventory, EntityItemInventoryInstanceID>;
	using EntityLockStatePool = RecyclablePool<EntityLockState, EntityLockStateID>;

	EntityPool entities;
	EntityPositionPool positions;
	EntityBoundingBoxPool boundingBoxes;
	EntityDirectionPool directions;
	EntityAnimationInstancePool animInsts;
	EntityCombatStatePool combatStates;
	EntityCreatureSoundPool creatureSoundInsts;
	EntityCitizenDirectionIndexPool citizenDirectionIndices;
	EntityCitizenNamePool citizenNames;

	// Each citizen has a unique palette indirection in place of unique textures for memory savings. It was found
	// that hardly any citizen instances share textures due to variations in their random palette. As a result,
	// citizen textures will need to be 8-bit.
	EntityPaletteIndicesInstancePool paletteIndices;

	EntityItemInventoryInstancePool itemInventories;
	EntityLockStatePool lockStates;

	// Entity definitions for this currently-active level. Their definition IDs CANNOT be assumed
	// to be zero-based because these are in addition to ones in the entity definition library.
	// @todo: separate EntityAnimationDefinition from EntityDefinition?
	std::unordered_map<EntityDefID, EntityDefinition> entityDefs;

	// Entities that should have their instance resources freed, either because the chunk they were in
	// was unloaded, or they were otherwise despawned. Cleared at end-of-frame.
	std::vector<EntityInstanceID> destroyedEntityIDs;

	// Entities that have moved from one chunk to another and are still in play.
	std::vector<EntityTransferResult> transferResults;

	EntityDefID addEntityDef(EntityDefinition &&def, const EntityDefinitionLibrary &defLibrary);
	EntityDefID getOrAddEntityDefID(const EntityDefinition &def, const EntityDefinitionLibrary &defLibrary);

	void initializeEntity(EntityInstance &entityInst, EntityInstanceID instID, const EntityDefinition &entityDef,
		const EntityAnimationDefinition &animDef, const EntityInitInfo &initInfo, Random &random, JPH::PhysicsSystem &physicsSystem,
		Renderer &renderer);

	void populateChunkEntities(EntityChunk &entityChunk, const VoxelChunk &chunk, const LevelDefinition &levelDefinition,
		const LevelInfoDefinition &levelInfoDefinition, const WorldInt2 &levelOffset,
		const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		Random &random, const EntityDefinitionLibrary &entityDefLibrary, JPH::PhysicsSystem &physicsSystem,
		TextureManager &textureManager, Renderer &renderer);
	void populateChunk(EntityChunk &entityChunk, const VoxelChunk &voxelChunk, const LevelDefinition &levelDef,
		const LevelInfoDefinition &levelInfoDef, const MapSubDefinition &mapSubDef, const EntityGeneration::EntityGenInfo &entityGenInfo,
		const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo, double ceilingScale,
		Random &random, const EntityDefinitionLibrary &entityDefLibrary, JPH::PhysicsSystem &physicsSystem,
		TextureManager &textureManager, Renderer &renderer);

	void updateCitizenStates(double dt, EntityChunk &entityChunk, const WorldDouble2 &playerPositionXZ, bool isPlayerMoving,
		bool isPlayerWeaponSheathed, Random &random, JPH::PhysicsSystem &physicsSystem, const VoxelChunkManager &voxelChunkManager);

	std::string getCreatureSoundFilename(const EntityDefID defID) const;
	void updateCreatureSounds(double dt, EntityChunk &entityChunk, const WorldDouble3 &playerPosition, Random &random, AudioManager &audioManager);
	void updateFadedElevatedPlatforms(EntityChunk &entityChunk, const VoxelChunk &voxelChunk, double ceilingScale, JPH::PhysicsSystem &physicsSystem);
	void updateEnemyDeathStates(EntityChunk &entityChunk, JPH::PhysicsSystem &physicsSystem);
	void updateVfx(EntityChunk &entityChunk);
public:
	const EntityDefinition &getEntityDef(EntityDefID defID) const;
	const EntityInstance &getEntity(EntityInstanceID id) const;
	const WorldDouble3 &getEntityPosition(EntityPositionID id) const;
	const BoundingBox3D &getEntityBoundingBox(EntityBoundingBoxID id) const;
	const VoxelDouble2 &getEntityDirection(EntityDirectionID id) const;
	EntityAnimationInstance &getEntityAnimationInstance(EntityAnimationInstanceID id);
	const EntityAnimationInstance &getEntityAnimationInstance(EntityAnimationInstanceID id) const;
	EntityCombatState &getEntityCombatState(EntityCombatStateID id);
	const EntityCombatState &getEntityCombatState(EntityCombatStateID id) const;
	int8_t getEntityCitizenDirectionIndex(EntityCitizenDirectionIndexID id) const;
	const EntityCitizenName &getEntityCitizenName(EntityCitizenNameID id) const;
	const PaletteIndices &getEntityPaletteIndices(EntityPaletteIndicesInstanceID id) const;
	ItemInventory &getEntityItemInventory(EntityItemInventoryInstanceID id);
	EntityLockState &getEntityLockState(EntityLockStateID id);
	const EntityLockState &getEntityLockState(EntityLockStateID id) const;

	EntityInstanceID getEntityFromPhysicsBodyID(JPH::BodyID bodyID) const;

	// Gets the entities scheduled for destruction this frame. If they're in this list, they should no longer be
	// simulated or rendered.
	BufferView<const EntityInstanceID> getQueuedDestroyEntityIDs() const;

	// Gets all entities who have moved between chunks this frame. Cleared at end of frame.
	BufferView<const EntityTransferResult> getEntityTransferResults() const;

	// Count functions for specialized entities.
	int getCountInChunkWithDirection(const ChunkInt2 &chunkPos) const;
	int getCountInChunkWithCreatureSound(const ChunkInt2 &chunkPos) const;
	int getCountInChunkWithCitizenDirection(const ChunkInt2 &chunkPos) const;

	// Gets the entity visibility state necessary for rendering and ray cast selection.
	void getEntityObservedResult(EntityInstanceID id, const WorldDouble3 &eyePosition, EntityObservedResult &result) const;

	// Creates a fully-initialized entity in the scene that is immediately ready to simulate.
	EntityInstanceID createEntity(const EntityInitInfo &initInfo, Random &random, JPH::PhysicsSystem &physicsSystem, Renderer &renderer);

	void update(double dt, BufferView<const ChunkInt2> activeChunkPositions,
		BufferView<const ChunkInt2> newChunkPositions, BufferView<const ChunkInt2> freedChunkPositions,
		const Player &player, const LevelDefinition *activeLevelDef, const LevelInfoDefinition *activeLevelInfoDef,
		const MapSubDefinition &mapSubDef, BufferView<const LevelDefinition> levelDefs,
		BufferView<const int> levelInfoDefIndices, BufferView<const LevelInfoDefinition> levelInfoDefs,
		const EntityGeneration::EntityGenInfo &entityGenInfo, const std::optional<CitizenUtils::CitizenGenInfo> &citizenGenInfo,
		double ceilingScale, Random &random, const VoxelChunkManager &voxelChunkManager, AudioManager &audioManager,
		JPH::PhysicsSystem &physicsSystem, TextureManager &textureManager, Renderer &renderer);

	// Prepares an entity for destruction later this frame, optionally notifying its chunk to remove its reference.
	// Don't need to notify the chunk if it's being unloaded this frame.
	void queueEntityDestroy(EntityInstanceID entityInstID, const ChunkInt2 *chunkToNotify);
	void queueEntityDestroy(EntityInstanceID entityInstID, bool notifyChunk);

	// @todo: support spawning an entity not from the level def

	void cleanUp(JPH::PhysicsSystem &physicsSystem, Renderer &renderer);
	void clear(JPH::PhysicsSystem &physicsSystem, Renderer &renderer);
};

#endif
