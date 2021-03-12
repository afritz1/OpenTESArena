#include <algorithm>
#include <cmath>
#include <cstring>

#include "EntityDefinitionLibrary.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Assets/MIFUtils.h"
#include "../Game/Game.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../World/ChunkUtils.h"

#include "components/debug/Debug.h"

EntityManager::EntityVisibilityData::EntityVisibilityData() :
	flatPosition(ChunkInt2::Zero, VoxelDouble3::Zero)
{
	this->entity = nullptr;
	this->stateIndex = -1;
	this->angleIndex = -1;
	this->keyframeIndex = -1;
}

void EntityManager::EntityVisibilityData::init(const Entity *entity, const CoordDouble3 &flatPosition,
	int stateIndex, int angleIndex, int keyframeIndex)
{
	this->entity = entity;
	this->flatPosition = flatPosition;
	this->stateIndex = stateIndex;
	this->angleIndex = angleIndex;
	this->keyframeIndex = keyframeIndex;
}

template <typename T>
int EntityManager::EntityGroup<T>::getCount() const
{
	return static_cast<int>(this->entities.size());
}

template <typename T>
T *EntityManager::EntityGroup<T>::getEntityAtIndex(int index)
{
	DebugAssert(this->validEntities.size() == this->entities.size());
	DebugAssertIndex(this->validEntities, index);
	const bool isValid = this->validEntities[index];
	return isValid ? &this->entities[index] : nullptr;
}

template <typename T>
const T *EntityManager::EntityGroup<T>::getEntityAtIndex(int index) const
{
	DebugAssert(this->validEntities.size() == this->entities.size());
	DebugAssertIndex(this->validEntities, index);
	const bool isValid = this->validEntities[index];
	return isValid ? &this->entities[index] : nullptr;
}

template <typename T>
int EntityManager::EntityGroup<T>::getEntities(Entity **outEntities, int outSize)
{
	DebugAssert(outEntities != nullptr);
	DebugAssert(outSize >= 0);
	DebugAssert(this->validEntities.size() == this->entities.size());

	int writeIndex = 0;
	const int count = this->getCount();
	for (int i = 0; i < count; i++)
	{
		// Break if the destination buffer is full.
		if (writeIndex == outSize)
		{
			break;
		}

		DebugAssertIndex(this->validEntities, i);
		const bool isValid = this->validEntities[i];

		// Skip if entity is not valid.
		if (!isValid)
		{
			continue;
		}

		T &entity = this->entities[i];
		outEntities[writeIndex] = &entity;
		writeIndex++;
	}

	return writeIndex;
}

template <typename T>
int EntityManager::EntityGroup<T>::getEntities(const Entity **outEntities, int outSize) const
{
	DebugAssert(outEntities != nullptr);
	DebugAssert(outSize >= 0);
	DebugAssert(this->validEntities.size() == this->entities.size());

	int writeIndex = 0;
	const int count = this->getCount();
	for (int i = 0; i < count; i++)
	{
		// Break if the destination buffer is full.
		if (writeIndex == outSize)
		{
			break;
		}

		DebugAssertIndex(this->validEntities, i);
		const bool isValid = this->validEntities[i];

		// Skip if entity is not valid.
		if (!isValid)
		{
			continue;
		}

		const T &entity = this->entities[i];
		outEntities[writeIndex] = &entity;
		writeIndex++;
	}

	return writeIndex;
}

template <typename T>
std::optional<int> EntityManager::EntityGroup<T>::getEntityIndex(EntityID id) const
{
	const auto iter = this->indices.find(id);
	if (iter != this->indices.end())
	{
		return iter->second;
	}
	else
	{
		return std::nullopt;
	}
}

template <typename T>
int EntityManager::EntityGroup<T>::nextFreeIndex()
{
	int index;
	if (this->freeIndices.size() > 0)
	{
		// Reuse a previously-owned entity slot.
		index = this->freeIndices.back();
		this->freeIndices.pop_back();

		DebugAssertIndex(this->validEntities, index);
		this->validEntities[index] = true;
	}
	else
	{
		// Insert new at the end of the entities list.
		index = static_cast<int>(this->entities.size());
		this->entities.push_back(T());
		this->validEntities.push_back(true);
	}

	return index;
}

template <typename T>
T *EntityManager::EntityGroup<T>::addEntity(EntityID id)
{
	DebugAssert(id != EntityManager::NO_ID);
	DebugAssert(this->validEntities.size() == this->entities.size());

	// Entity ID must not already be in use.
	DebugAssert(!this->getEntityIndex(id).has_value());

	// Find available position in entities array, allocating space if needed.
	const int index = this->nextFreeIndex();

	// Initialize basic entity data.
	DebugAssertIndex(this->entities, index);
	T &entitySlot = this->entities[index];
	entitySlot.reset();
	entitySlot.setID(id);

	// Insert into ID -> entity index table.
	this->indices.insert(std::make_pair(id, index));

	return &entitySlot;
}

template <typename T>
bool EntityManager::EntityGroup<T>::tryAcquireEntity(EntityID id, EntityGroup<T> &oldGroup)
{
	if (id == EntityManager::NO_ID)
	{
		DebugLogWarning("Cannot acquire invalid entity.");
		return false;
	}

	// Entity ID must not already be in use.
	if (this->getEntityIndex(id).has_value())
	{
		DebugLogWarning("Entity \"" + std::to_string(id) + "\" already in this group.");
		return false;
	}

	std::optional<int> oldEntityIndex = oldGroup.getEntityIndex(id);
	if (!oldEntityIndex.has_value())
	{
		DebugLogWarning("Entity \"" + std::to_string(id) + "\" not in old group.");
		return false;
	}

	// Move entity from old group to new group.
	const int newEntityIndex = this->nextFreeIndex();
	DebugAssertIndex(this->entities, newEntityIndex);
	this->entities[newEntityIndex] = std::move(oldGroup.entities[*oldEntityIndex]);
	this->indices.insert(std::make_pair(id, newEntityIndex));

	// Clean up old group.
	oldGroup.validEntities[*oldEntityIndex] = false;
	oldGroup.indices.erase(id);
	oldGroup.freeIndices.push_back(*oldEntityIndex);

	return true;
}

template <typename T>
void EntityManager::EntityGroup<T>::remove(EntityID id)
{
	DebugAssert(id != EntityManager::NO_ID);
	DebugAssert(this->validEntities.size() == this->entities.size());

	// Find entity with the given ID.
	const std::optional<int> optIndex = this->getEntityIndex(id);
	if (optIndex.has_value())
	{
		const int index = optIndex.value();

		// Clear entity slot.
		DebugAssertIndex(this->entities, index);
		this->entities[index].reset();
		this->validEntities[index] = false;

		// Clear ID mapping.
		this->indices.erase(id);

		// Add entity index to previously-owned slots list.
		this->freeIndices.push_back(index);
	}
	else
	{
		// Not in entity group.
		DebugLogWarning("Tried to remove missing entity \"" + std::to_string(id) + "\".");
	}
}

template <typename T>
void EntityManager::EntityGroup<T>::clear()
{
	this->entities.clear();
	this->validEntities.clear();
	this->indices.clear();
	this->freeIndices.clear();
}

void EntityManager::EntityChunk::init(const ChunkInt2 &chunk)
{
	this->chunk = chunk;
}

void EntityManager::EntityChunk::clear()
{
	this->staticGroup.clear();
	this->dynamicGroup.clear();
}

EntityManager::EntityManager()
{
	this->nextID = 0;
}

EntityID EntityManager::nextFreeID()
{
	// Check if any pre-owned entity IDs are available.
	if (this->freeIDs.size() > 0)
	{
		const EntityID id = this->freeIDs.back();
		this->freeIDs.pop_back();
		return id;
	}
	else
	{
		// Get the next available ID.
		const EntityID id = this->nextID;
		this->nextID++;
		return id;
	}
}

EntityRef EntityManager::makeEntity(EntityType type)
{
	// Get the default chunk for creating the entity in.
	DebugAssertMsg(!this->entityChunks.empty(), "Need at least one active chunk for creating an entity.");
	EntityChunk &defaultEntityChunk = this->entityChunks.front();

	const EntityID id = this->nextFreeID();
	EntityRef entityRef = [this, type, &defaultEntityChunk, id]()
	{
		// Instantiate the entity based on their type.
		if (type == EntityType::Static)
		{
			EntityGroup<StaticEntity> &group = defaultEntityChunk.staticGroup;
			StaticEntity *entity = group.addEntity(id);
			return EntityRef(this, id, type);
		}
		else if (type == EntityType::Dynamic)
		{
			EntityGroup<DynamicEntity> &group = defaultEntityChunk.dynamicGroup;
			DynamicEntity *entity = group.addEntity(id);
			return EntityRef(this, id, type);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
			return EntityRef(nullptr, EntityManager::NO_ID, static_cast<EntityType>(-1));
		}
	}();

	DebugAssert(entityRef.getID() == id);
	return entityRef;
}

template <typename T>
T *EntityManager::getInternal(EntityID id, EntityGroup<T> &group)
{
	if (id == EntityManager::NO_ID)
	{
		return nullptr;
	}

	const std::optional<int> index = group.getEntityIndex(id);
	if (!index.has_value())
	{
		return nullptr;
	}

	return group.getEntityAtIndex(*index);
}

template <typename T>
const T *EntityManager::getInternal(EntityID id, const EntityGroup<T> &group) const
{
	if (id == EntityManager::NO_ID)
	{
		return nullptr;
	}

	const std::optional<int> index = group.getEntityIndex(id);
	if (!index.has_value())
	{
		return nullptr;
	}

	return group.getEntityAtIndex(*index);
}

std::optional<int> EntityManager::tryGetChunkIndex(const ChunkInt2 &chunk) const
{
	const auto iter = std::find_if(this->entityChunks.begin(), this->entityChunks.end(),
		[&chunk](const EntityChunk &entityChunk)
	{
		return entityChunk.chunk == chunk;
	});

	if (iter != this->entityChunks.end())
	{
		return static_cast<int>(std::distance(this->entityChunks.begin(), iter));
	}
	else
	{
		return std::nullopt;
	}
}

Entity *EntityManager::getEntityHandle(EntityID id, EntityType type)
{
	for (EntityChunk &entityChunk : this->entityChunks)
	{
		if (type == EntityType::Static)
		{
			EntityGroup<StaticEntity> &group = entityChunk.staticGroup;
			StaticEntity *entity = this->getInternal(id, group);
			if (entity != nullptr)
			{
				return entity;
			}
		}
		else if (type == EntityType::Dynamic)
		{
			EntityGroup<DynamicEntity> &group = entityChunk.dynamicGroup;
			DynamicEntity *entity = this->getInternal(id, group);
			if (entity != nullptr)
			{
				return entity;
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
		}
	}

	return nullptr;
}

const Entity *EntityManager::getEntityHandle(EntityID id, EntityType type) const
{
	for (const EntityChunk &entityChunk : this->entityChunks)
	{
		if (type == EntityType::Static)
		{
			const EntityGroup<StaticEntity> &group = entityChunk.staticGroup;
			const StaticEntity *entity = this->getInternal(id, group);
			if (entity != nullptr)
			{
				return entity;
			}
		}
		else if (type == EntityType::Dynamic)
		{
			const EntityGroup<DynamicEntity> &group = entityChunk.dynamicGroup;
			const DynamicEntity *entity = this->getInternal(id, group);
			if (entity != nullptr)
			{
				return entity;
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
		}
	}

	return nullptr;
}

Entity *EntityManager::getEntityHandle(EntityID id)
{
	// Find which entity group the given entity ID is in. This is a slow look-up because there is
	// no hint where the entity is at.
	Entity *entity = this->getEntityHandle(id, EntityType::Static);
	if (entity != nullptr)
	{
		// Static entity.
		return entity;
	}

	entity = this->getEntityHandle(id, EntityType::Dynamic);
	if (entity != nullptr)
	{
		// Dynamic entity.
		return entity;
	}

	// Not in any entity group.
	return nullptr;
}

const Entity *EntityManager::getEntityHandle(EntityID id) const
{
	// Find which entity group the given entity ID is in. This is a slow look-up because there is
	// no hint where the entity is at.
	const Entity *entity = this->getEntityHandle(id, EntityType::Static);
	if (entity != nullptr)
	{
		// Static entity.
		return entity;
	}

	entity = this->getEntityHandle(id, EntityType::Dynamic);
	if (entity != nullptr)
	{
		// Dynamic entity.
		return entity;
	}

	// Not in any entity group.
	return nullptr;
}

EntityRef EntityManager::getEntityRef(EntityID id, EntityType type)
{
	return EntityRef(this, id, type);
}

ConstEntityRef EntityManager::getEntityRef(EntityID id, EntityType type) const
{
	return ConstEntityRef(this, id, type);
}

EntityRef EntityManager::getEntityRef(EntityID id)
{
	// Get the entity's type if possible.
	Entity *entity = this->getEntityHandle(id);
	EntityType entityType = (entity != nullptr) ? entity->getEntityType() : EntityType::Static;
	return this->getEntityRef(id, entityType);
}

ConstEntityRef EntityManager::getEntityRef(EntityID id) const
{
	// Get the entity's type if possible.
	const Entity *entity = this->getEntityHandle(id);
	EntityType entityType = (entity != nullptr) ? entity->getEntityType() : EntityType::Static;
	return this->getEntityRef(id, entityType);
}

int EntityManager::getCountOfType(EntityType entityType) const
{
	int count = 0;
	for (const EntityChunk &entityChunk : this->entityChunks)
	{
		if (entityType == EntityType::Static)
		{
			count += entityChunk.staticGroup.getCount();
		}
		else if (entityType == EntityType::Dynamic)
		{
			count += entityChunk.dynamicGroup.getCount();
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(entityType)));
		}
	}

	return count;
}

int EntityManager::getCountInChunk(const ChunkInt2 &chunk) const
{
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunk);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("Entity chunk \"" + chunk.toString() + "\" not in entity manager.");
		return 0;
	}

	const EntityChunk &entityChunk = this->entityChunks[*chunkIndex];
	return entityChunk.staticGroup.getCount() + entityChunk.dynamicGroup.getCount();
}

int EntityManager::getCount() const
{
	return this->getCountOfType(EntityType::Static) + this->getCountOfType(EntityType::Dynamic);
}

int EntityManager::getEntitiesOfType(EntityType entityType, Entity **outEntities, int outSize)
{
	DebugAssert(outEntities != nullptr);
	DebugAssert(outSize >= 0);

	int writeIndex = 0;
	for (EntityChunk &entityChunk : this->entityChunks)
	{
		if (writeIndex == outSize)
		{
			break;
		}

		if (entityType == EntityType::Static)
		{
			EntityGroup<StaticEntity> &group = entityChunk.staticGroup;
			writeIndex += group.getEntities(outEntities + writeIndex, outSize - writeIndex);
		}
		else if (entityType == EntityType::Dynamic)
		{
			EntityGroup<DynamicEntity> &group = entityChunk.dynamicGroup;
			writeIndex += group.getEntities(outEntities + writeIndex, outSize - writeIndex);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(entityType)));
		}
	}

	return writeIndex;
}

int EntityManager::getEntitiesOfType(EntityType entityType, const Entity **outEntities, int outSize) const
{
	DebugAssert(outEntities != nullptr);
	DebugAssert(outSize >= 0);

	int writeIndex = 0;
	for (const EntityChunk &entityChunk : this->entityChunks)
	{
		if (writeIndex == outSize)
		{
			break;
		}

		if (entityType == EntityType::Static)
		{
			const EntityGroup<StaticEntity> &group = entityChunk.staticGroup;
			writeIndex += group.getEntities(outEntities + writeIndex, outSize - writeIndex);
		}
		else if (entityType == EntityType::Dynamic)
		{
			const EntityGroup<DynamicEntity> &group = entityChunk.dynamicGroup;
			writeIndex += group.getEntities(outEntities + writeIndex, outSize - writeIndex);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(entityType)));
		}
	}

	return writeIndex;
}

int EntityManager::getEntitiesInChunk(const ChunkInt2 &chunk, const Entity **outEntities, int outSize) const
{
	// Can't assume the given pointer is not null.
	if ((outEntities == nullptr) || (outSize == 0))
	{
		return 0;
	}

	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunk);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("Entity chunk \"" + chunk.toString() + "\" not in entity manager.");
		return 0;
	}

	const EntityChunk &entityChunk = this->entityChunks[*chunkIndex];

	// Fill the output buffer with as many entities as will fit.
	int writeIndex = 0;

	const EntityGroup<StaticEntity> &staticGroup = entityChunk.staticGroup;
	const int staticEntityCount = staticGroup.getCount();
	for (int i = 0; i < staticEntityCount; i++)
	{
		// Break if the output buffer is full.
		if (writeIndex == outSize)
		{
			break;
		}

		outEntities[writeIndex] = staticGroup.getEntityAtIndex(i);
		writeIndex++;
	}

	const EntityGroup<DynamicEntity> &dynamicGroup = entityChunk.dynamicGroup;
	const int dynamicEntityCount = dynamicGroup.getCount();
	for (int i = 0; i < dynamicEntityCount; i++)
	{
		// Break if the output buffer is full.
		if (writeIndex == outSize)
		{
			break;
		}

		outEntities[writeIndex] = dynamicGroup.getEntityAtIndex(i);
		writeIndex++;
	}

	return writeIndex;
}

int EntityManager::getEntities(const Entity **outEntities, int outSize) const
{
	// Apparently std::vector::data() can be either null or non-null when the container is empty,
	// so can't assume the given pointer is not null.
	if ((outEntities == nullptr) || (outSize == 0))
	{
		return 0;
	}

	// Fill the output buffer with as many entities as will fit.
	int writeIndex = 0;
	for (const EntityChunk &entityChunk : this->entityChunks)
	{
		if (writeIndex == outSize)
		{
			break;
		}

		writeIndex += this->getEntitiesInChunk(entityChunk.chunk, outEntities + writeIndex, outSize - writeIndex);
	}

	return writeIndex;
}

bool EntityManager::hasChunk(const ChunkInt2 &chunk) const
{
	const auto iter = std::find_if(this->entityChunks.begin(), this->entityChunks.end(),
		[&chunk](const EntityChunk &entityChunk)
	{
		return entityChunk.chunk == chunk;
	});

	return iter != this->entityChunks.end();
}

bool EntityManager::hasEntityDef(EntityDefID defID) const
{
	return (defID >= 0) && (defID < static_cast<int>(this->entityDefs.size()));
}

const EntityDefinition &EntityManager::getEntityDef(EntityDefID defID,
	const EntityDefinitionLibrary &entityDefLibrary) const
{
	const auto iter = this->entityDefs.find(defID);
	if (iter != this->entityDefs.end())
	{
		return iter->second;
	}
	else
	{
		return entityDefLibrary.getDefinition(defID);
	}
}

EntityDefID EntityManager::addEntityDef(EntityDefinition &&def,
	const EntityDefinitionLibrary &entityDefLibrary)
{
	const int libraryDefCount = entityDefLibrary.getDefinitionCount();
	const EntityDefID defID = static_cast<EntityDefID>(libraryDefCount + this->entityDefs.size());
	this->entityDefs.emplace(std::make_pair(defID, std::move(def)));
	return defID;
}

void EntityManager::getEntityVisibilityData(const Entity &entity, const CoordDouble2 &eye2D,
	double ceilingScale, const ChunkManager &chunkManager, const EntityDefinitionLibrary &entityDefLibrary,
	EntityVisibilityData &outVisData) const
{
	outVisData.entity = &entity;
	const EntityDefinition &entityDef = this->getEntityDef(entity.getDefinitionID(), entityDefLibrary);
	const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
	const EntityAnimationInstance &animInst = entity.getAnimInstance();

	// Get active animation state.
	const int animStateIndex = animInst.getStateIndex();
	const EntityAnimationDefinition::State &animDefState = animDef.getState(animStateIndex);
	const EntityAnimationInstance::State &animInstState = animInst.getState(animStateIndex);
	outVisData.stateIndex = animStateIndex;

	// Get animation angle based on entity direction relative to some camera/eye.
	const int angleCount = animInstState.getKeyframeListCount();
	const Radians animAngle = [&entity, &eye2D, angleCount]()
	{
		if (entity.getEntityType() == EntityType::Static)
		{
			// Static entities always face the camera.
			return 0.0;
		}
		else if (entity.getEntityType() == EntityType::Dynamic)
		{
			// Dynamic entities are angle-dependent.
			const DynamicEntity &dynamicEntity = static_cast<const DynamicEntity&>(entity);
			const NewDouble2 &entityDir = dynamicEntity.getDirection();
			const VoxelDouble2 diffDir = (eye2D - dynamicEntity.getPosition()).normalized();

			const Radians entityAngle = MathUtils::fullAtan2(entityDir);
			const Radians diffAngle = MathUtils::fullAtan2(diffDir);

			// Use the difference of the two angles to get the relative angle.
			const Radians resultAngle = Constants::TwoPi + (entityAngle - diffAngle);

			// Angle bias so the final direction is centered within its angle range.
			const Radians angleBias = (Constants::TwoPi / static_cast<double>(angleCount)) * 0.50;

			return std::fmod(resultAngle + angleBias, Constants::TwoPi);
		}
		else
		{
			DebugUnhandledReturnMsg(double,
				std::to_string(static_cast<int>(entity.getEntityType())));
		}
	}();

	// Index into animation keyframe lists for the state.
	outVisData.angleIndex = [angleCount, animAngle]()
	{
		const double angleCountReal = static_cast<double>(angleCount);
		const double anglePercent = animAngle / Constants::TwoPi;
		const int angleIndex = static_cast<int>(angleCountReal * anglePercent);
		return std::clamp(angleIndex, 0, angleCount - 1);
	}();

	// Keyframe list for the current state and angle.
	const EntityAnimationDefinition::KeyframeList &animDefKeyframeList =
		animDefState.getKeyframeList(outVisData.angleIndex);

	// Progress through current animation.
	outVisData.keyframeIndex = [&animInst, &animDefState, &animDefKeyframeList]()
	{
		const int keyframeCount = animDefKeyframeList.getKeyframeCount();
		const double keyframeCountReal = static_cast<double>(keyframeCount);
		const double animCurSeconds = animInst.getCurrentSeconds();
		const double animTotalSeconds = animDefState.getTotalSeconds();
		const double animPercent = animCurSeconds / animTotalSeconds;
		const int keyframeIndex = static_cast<int>(keyframeCountReal * animPercent);
		return std::clamp(keyframeIndex, 0, keyframeCount - 1);
	}();

	// Current animation frame based on everything above.
	const EntityAnimationDefinition::Keyframe &animDefKeyframe =
		animDefKeyframeList.getKeyframe(outVisData.keyframeIndex);

	const double flatWidth = animDefKeyframe.getWidth();
	const double flatHeight = animDefKeyframe.getHeight();
	const double flatHalfWidth = flatWidth * 0.50;

	const int baseYOffset = EntityUtils::getYOffset(entityDef);
	const double flatYOffset = static_cast<double>(-baseYOffset) / MIFUtils::ARENA_UNITS;

	// If the entity is in a raised platform voxel, they are set on top of it.
	const CoordDouble2 &entityCoord = entity.getPosition();
	const double raisedPlatformYOffset = [ceilingScale, &chunkManager, &entityCoord]()
	{
		const CoordInt2 entityVoxelCoord(entityCoord.chunk, VoxelUtils::pointToVoxel(entityCoord.point));
		const Chunk *chunk = chunkManager.tryGetChunk(entityVoxelCoord.chunk);
		if (chunk == nullptr)
		{
			// Not sure this is ever reachable, but handle just in case.
			return 0.0;
		}

		const Chunk::VoxelID voxelID = chunk->getVoxel(entityVoxelCoord.voxel.x, 1, entityVoxelCoord.voxel.y);
		const VoxelDefinition &voxelDef = chunk->getVoxelDef(voxelID);

		if (voxelDef.type == ArenaTypes::VoxelType::Raised)
		{
			const VoxelDefinition::RaisedData &raised = voxelDef.raised;
			return (raised.yOffset + raised.ySize) * ceilingScale;
		}
		else
		{
			// No raised platform offset.
			return 0.0;
		}
	}();

	// Bottom center of flat.
	const VoxelDouble3 newCoordPoint(
		entityCoord.point.x,
		ceilingScale + flatYOffset + raisedPlatformYOffset,
		entityCoord.point.y);
	outVisData.flatPosition = CoordDouble3(entityCoord.chunk, newCoordPoint);
}

const EntityAnimationDefinition::Keyframe &EntityManager::getEntityAnimKeyframe(const Entity &entity,
	const EntityVisibilityData &visData, const EntityDefinitionLibrary &entityDefLibrary) const
{
	const EntityDefinition &entityDef = this->getEntityDef(entity.getDefinitionID(), entityDefLibrary);
	const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
	const EntityAnimationDefinition::State &animState = animDef.getState(visData.stateIndex);
	const EntityAnimationDefinition::KeyframeList &animKeyframeList =
		animState.getKeyframeList(visData.angleIndex);
	const EntityAnimationDefinition::Keyframe &animKeyframe =
		animKeyframeList.getKeyframe(visData.keyframeIndex);
	return animKeyframe;
}

void EntityManager::getEntityBoundingBox(const Entity &entity, const EntityVisibilityData &visData,
	const EntityDefinitionLibrary &entityDefLibrary, CoordDouble3 *outMin, CoordDouble3 *outMax) const
{
	// Get animation frame from visibility data.
	const EntityAnimationDefinition::Keyframe &keyframe =
		this->getEntityAnimKeyframe(entity, visData, entityDefLibrary);

	// Start with bounding cylinder.
	const double radius = keyframe.getWidth() * 0.50;
	const double height = keyframe.getHeight();

	// Convert bounding cylinder to axis-aligned bounding box. Need to calculate the resulting chunk coordinates
	// since the bounding box might cross chunk boundaries.
	const CoordDouble3 &flatPos = visData.flatPosition;
	const VoxelDouble3 minPoint(flatPos.point.x - radius, flatPos.point.y, flatPos.point.z - radius);
	const VoxelDouble3 maxPoint(flatPos.point.x + radius, flatPos.point.y + height, flatPos.point.z + radius);
	*outMin = ChunkUtils::recalculateCoord(flatPos.chunk, minPoint);
	*outMax = ChunkUtils::recalculateCoord(flatPos.chunk, maxPoint);
}

void EntityManager::updateEntityChunk(Entity *entity)
{
	if (entity == nullptr)
	{
		DebugLogWarning("Can't update null entity's chunk.");
		return;
	}

	const EntityID entityID = entity->getID();
	const EntityType entityType = entity->getEntityType();

	// Find which chunk they were in before.
	// @todo: if this is slow, we could use the entity's current chunk as a hint.
	std::optional<int> oldChunkIndex;
	for (int i = 0; i < static_cast<int>(this->entityChunks.size()); i++)
	{
		const EntityChunk &entityChunk = this->entityChunks[i];
		if (entityType == EntityType::Static)
		{
			const EntityGroup<StaticEntity> &group = entityChunk.staticGroup;
			const std::optional<int> entityIndex = group.getEntityIndex(entityID);
			if (entityIndex.has_value())
			{
				oldChunkIndex = i;
				break;
			}
		}
		else if (entityType == EntityType::Dynamic)
		{
			const EntityGroup<DynamicEntity> &group = entityChunk.dynamicGroup;
			const std::optional<int> entityIndex = group.getEntityIndex(entityID);
			if (entityIndex.has_value())
			{
				oldChunkIndex = i;
				break;
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(entityType)));
		}
	}

	if (!oldChunkIndex.has_value())
	{
		DebugLogError("Couldn't find old chunk that entity \"" + std::to_string(entityID) + "\" was in before.");
		return;
	}

	// See if the entity changed chunks.
	EntityChunk &oldEntityChunk = this->entityChunks[*oldChunkIndex];
	const ChunkInt2 &oldChunk = oldEntityChunk.chunk;

	const CoordDouble2 &entityPosition = entity->getPosition();
	const ChunkInt2 &newChunk = entityPosition.chunk;
	if (newChunk != oldChunk)
	{
		// Get the new chunk to move the entity to.
		const std::optional<int> newChunkIndex = this->tryGetChunkIndex(newChunk);
		if (!newChunkIndex.has_value())
		{
			DebugLogWarning("Couldn't get new entity chunk \"" + newChunk.toString() + "\" that the entity should be in.");
			return;
		}

		EntityChunk &newEntityChunk = this->entityChunks[*newChunkIndex];
		if (entityType == EntityType::Static)
		{
			EntityGroup<StaticEntity> &oldGroup = oldEntityChunk.staticGroup;
			EntityGroup<StaticEntity> &newGroup = newEntityChunk.staticGroup;
			if (!newGroup.tryAcquireEntity(entityID, oldGroup))
			{
				DebugLogError("Couldn't move static entity \"" + std::to_string(entityID) + "\" from old to new group.");
			}
		}
		else if (entityType == EntityType::Dynamic)
		{
			EntityGroup<DynamicEntity> &oldGroup = oldEntityChunk.dynamicGroup;
			EntityGroup<DynamicEntity> &newGroup = newEntityChunk.dynamicGroup;
			if (!newGroup.tryAcquireEntity(entityID, oldGroup))
			{
				DebugLogError("Couldn't move dynamic entity \"" + std::to_string(entityID) + "\" from old to new group.");
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(entityType)));
		}
	}
}

void EntityManager::remove(EntityID id)
{
	// Find which entity group the given entity ID is in. This is a slow look-up because there is
	// no hint where the entity is at.
	for (EntityChunk &entityChunk : this->entityChunks)
	{
		// If the entity is in either the static or dynamic entity group, delete it and return its ID
		// to the free IDs.
		EntityGroup<StaticEntity> &staticGroup = entityChunk.staticGroup;
		std::optional<int> entityIndex = staticGroup.getEntityIndex(id);
		if (entityIndex.has_value())
		{
			staticGroup.remove(id);
			this->freeIDs.push_back(id);
			return;
		}

		EntityGroup<DynamicEntity> &dynamicGroup = entityChunk.dynamicGroup;
		entityIndex = dynamicGroup.getEntityIndex(id);
		if (entityIndex.has_value())
		{
			dynamicGroup.remove(id);
			this->freeIDs.push_back(id);
			return;
		}
	}

	// Not in any entity group.
	DebugLogWarning("Tried to remove missing entity \"" + std::to_string(id) + "\".");
}

void EntityManager::clear()
{
	this->entityChunks.clear();
	this->entityDefs.clear();
	this->freeIDs.clear();
	this->nextID = 0;
}

void EntityManager::addChunk(const ChunkInt2 &chunk)
{
	if (this->tryGetChunkIndex(chunk).has_value())
	{
		DebugLogWarning("Entity chunk \"" + chunk.toString() + "\" already exists.");
		return;
	}

	// Add a new empty entity chunk.
	EntityChunk entityChunk;
	entityChunk.init(chunk);
	this->entityChunks.emplace_back(std::move(entityChunk));
}

void EntityManager::removeChunk(const ChunkInt2 &chunk)
{
	const std::optional<int> chunkIndex = this->tryGetChunkIndex(chunk);
	if (!chunkIndex.has_value())
	{
		DebugLogWarning("No entity chunk \"" + chunk.toString() + "\" to remove.");
		return;
	}

	this->entityChunks.erase(this->entityChunks.begin() + *chunkIndex);
}

void EntityManager::tick(Game &game, double dt)
{
	// Update all entities in each chunk.
	for (EntityChunk &entityChunk : this->entityChunks)
	{
		EntityGroup<StaticEntity> &staticGroup = entityChunk.staticGroup;
		const int staticEntityCount = staticGroup.getCount();
		for (int i = 0; i < staticEntityCount; i++)
		{
			StaticEntity *entity = staticGroup.getEntityAtIndex(i);
			if (entity != nullptr)
			{
				entity->tick(game, dt);
			}
		}

		EntityGroup<DynamicEntity> &dynamicGroup = entityChunk.dynamicGroup;
		const int dynamicEntityCount = dynamicGroup.getCount();
		for (int i = 0; i < dynamicEntityCount; i++)
		{
			DynamicEntity *entity = dynamicGroup.getEntityAtIndex(i);
			if (entity != nullptr)
			{
				entity->tick(game, dt);
				this->updateEntityChunk(entity);
			}
		}
	}
}
