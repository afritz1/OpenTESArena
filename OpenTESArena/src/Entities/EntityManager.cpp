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

namespace
{
	constexpr EntityID FIRST_ENTITY_ID = 0;
	constexpr SNInt DEFAULT_CHUNK_X = 0;
	constexpr WEInt DEFAULT_CHUNK_Z = 0;
}

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
void EntityManager::EntityGroup<T>::acquireEntity(EntityID id, EntityGroup<T> &oldGroup)
{
	DebugAssert(id != EntityManager::NO_ID);

	// Entity ID must not already be in use.
	DebugAssert(!this->getEntityIndex(id).has_value());

	std::optional<int> oldEntityIndex = oldGroup.getEntityIndex(id);
	if (!oldEntityIndex.has_value())
	{
		DebugLogWarning("Entity \"" + std::to_string(id) + "\" not in old group.");
		return;
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

void EntityManager::init(SNInt chunkCountX, WEInt chunkCountZ)
{
	this->staticGroups.init(chunkCountX, chunkCountZ);
	this->dynamicGroups.init(chunkCountX, chunkCountZ);
	this->nextID = FIRST_ENTITY_ID;
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

bool EntityManager::isValidChunk(const ChunkInt2 &chunk) const
{
	const SNInt chunkCountX = this->staticGroups.getWidth();
	const WEInt chunkCountZ = this->staticGroups.getHeight();
	return (chunk.x >= 0) && (chunk.x < chunkCountX) && (chunk.y >= 0) && (chunk.y < chunkCountZ);
}

EntityRef EntityManager::makeEntity(EntityType type)
{
	const EntityID id = this->nextFreeID();
	EntityRef entityRef = [this, type, id]()
	{
		if (type == EntityType::Static)
		{
			auto &group = this->staticGroups.get(DEFAULT_CHUNK_X, DEFAULT_CHUNK_Z);
			StaticEntity *entity = group.addEntity(id);
			return EntityRef(this, id, type);
		}
		else if (type == EntityType::Dynamic)
		{
			auto &group = this->dynamicGroups.get(DEFAULT_CHUNK_X, DEFAULT_CHUNK_Z);
			DynamicEntity *entity = group.addEntity(id);
			return EntityRef(this, id, type);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
			return EntityRef(nullptr, EntityManager::NO_ID, EntityType::Static);
		}
	}();

	DebugAssert(entityRef.getID() == id);
	return entityRef;
}

template <typename T>
Entity *EntityManager::getInternal(EntityID id, EntityGroup<T> &group)
{
	if (id == EntityManager::NO_ID)
	{
		return nullptr;
	}

	const std::optional<int> index = group.getEntityIndex(id);
	return index.has_value() ? group.getEntityAtIndex(*index) : nullptr;
}

template <typename T>
const Entity *EntityManager::getInternal(EntityID id, const EntityGroup<T> &group) const
{
	if (id == EntityManager::NO_ID)
	{
		return nullptr;
	}

	const std::optional<int> index = group.getEntityIndex(id);
	return index.has_value() ? group.getEntityAtIndex(*index) : nullptr;
}

Entity *EntityManager::getEntityHandle(EntityID id, EntityType type)
{
	// Use the given entity type to determine which entity group to look in.
	auto tryGetEntityHandle = [this, id](auto &entityGroups) -> Entity*
	{
		for (WEInt z = 0; z < entityGroups.getHeight(); z++)
		{
			for (SNInt x = 0; x < entityGroups.getWidth(); x++)
			{
				auto &group = entityGroups.get(x, z);
				Entity *entity = this->getInternal(id, group);
				if (entity != nullptr)
				{
					return entity;
				}
			}
		}

		return nullptr;
	};

	switch (type)
	{
	case EntityType::Static:
		return tryGetEntityHandle(this->staticGroups);
	case EntityType::Dynamic:
		return tryGetEntityHandle(this->dynamicGroups);
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
		return nullptr;
	}
}

const Entity *EntityManager::getEntityHandle(EntityID id, EntityType type) const
{
	// Use the given entity type to determine which entity group to look in.
	auto tryGetEntityHandle = [this, id](const auto &entityGroups) -> const Entity*
	{
		for (WEInt z = 0; z < entityGroups.getHeight(); z++)
		{
			for (SNInt x = 0; x < entityGroups.getWidth(); x++)
			{
				const auto &group = entityGroups.get(x, z);
				const Entity *entity = this->getInternal(id, group);
				if (entity != nullptr)
				{
					return entity;
				}
			}
		}

		return nullptr;
	};

	switch (type)
	{
	case EntityType::Static:
		return tryGetEntityHandle(this->staticGroups);
	case EntityType::Dynamic:
		return tryGetEntityHandle(this->dynamicGroups);
	default:
		DebugNotImplementedMsg(std::to_string(static_cast<int>(type)));
		return nullptr;
	}
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

int EntityManager::getCount(EntityType entityType) const
{
	auto getCountFromGroups = [](const auto &entityGroups)
	{
		int count = 0;
		for (WEInt z = 0; z < entityGroups.getHeight(); z++)
		{
			for (SNInt x = 0; x < entityGroups.getWidth(); x++)
			{
				const auto &entityGroup = entityGroups.get(x, z);
				count += entityGroup.getCount();
			}
		}

		return count;
	};

	switch (entityType)
	{
	case EntityType::Static:
		return getCountFromGroups(this->staticGroups);
	case EntityType::Dynamic:
		return getCountFromGroups(this->dynamicGroups);
	default:
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(entityType)));
	}
}

int EntityManager::getTotalCountInChunk(const ChunkInt2 &chunk) const
{
	DebugAssert(this->staticGroups.getWidth() == this->dynamicGroups.getWidth());
	DebugAssert(this->staticGroups.getHeight() == this->dynamicGroups.getHeight());

	if (!this->isValidChunk(chunk))
	{
		return 0;
	}

	const auto &staticGroup = this->staticGroups.get(chunk.x, chunk.y);
	const auto &dynamicGroup = this->dynamicGroups.get(chunk.x, chunk.y);
	return staticGroup.getCount() + dynamicGroup.getCount();
}

int EntityManager::getTotalCount() const
{
	return this->getCount(EntityType::Static) + this->getCount(EntityType::Dynamic);
}

int EntityManager::getEntities(EntityType entityType, Entity **outEntities, int outSize)
{
	DebugAssert(outEntities != nullptr);
	DebugAssert(outSize >= 0);

	auto getEntitiesFromGroups = [](auto &entityGroups, Entity **outEntities, int outSize)
	{
		int writeIndex = 0;
		for (WEInt z = 0; z < entityGroups.getHeight(); z++)
		{
			for (SNInt x = 0; x < entityGroups.getWidth(); x++)
			{
				if (writeIndex == outSize)
				{
					break;
				}

				auto &entityGroup = entityGroups.get(x, z);
				writeIndex += entityGroup.getEntities(outEntities + writeIndex, outSize - writeIndex);
			}
		}

		return writeIndex;
	};

	// Get entities from the desired type.
	switch (entityType)
	{
	case EntityType::Static:
		return getEntitiesFromGroups(this->staticGroups, outEntities, outSize);
	case EntityType::Dynamic:
		return getEntitiesFromGroups(this->dynamicGroups, outEntities, outSize);
	default:
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(entityType)));
	}
}

int EntityManager::getEntities(EntityType entityType, const Entity **outEntities, int outSize) const
{
	DebugAssert(outEntities != nullptr);
	DebugAssert(outSize >= 0);

	auto getEntitiesFromGroups = [](const auto &entityGroups, const Entity **outEntities, int outSize)
	{
		int writeIndex = 0;
		for (WEInt z = 0; z < entityGroups.getHeight(); z++)
		{
			for (SNInt x = 0; x < entityGroups.getWidth(); x++)
			{
				if (writeIndex == outSize)
				{
					break;
				}

				const auto &entityGroup = entityGroups.get(x, z);
				writeIndex += entityGroup.getEntities(outEntities + writeIndex, outSize - writeIndex);
			}
		}

		return writeIndex;
	};

	// Get entities from the desired type.
	switch (entityType)
	{
	case EntityType::Static:
		return getEntitiesFromGroups(this->staticGroups, outEntities, outSize);
	case EntityType::Dynamic:
		return getEntitiesFromGroups(this->dynamicGroups, outEntities, outSize);
	default:
		DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(entityType)));
	}
}

int EntityManager::getTotalEntitiesInChunk(const ChunkInt2 &chunk, const Entity **outEntities, int outSize) const
{
	if (!this->isValidChunk(chunk))
	{
		return 0;
	}

	// Can't assume the given pointer is not null.
	if ((outEntities == nullptr) || (outSize == 0))
	{
		return 0;
	}

	// Fill the output buffer with as many entities as will fit.
	int writeIndex = 0;
	auto tryWriteEntities = [outEntities, outSize, &writeIndex](const auto &entityGroup)
	{
		const int entityCount = entityGroup.getCount();

		for (int i = 0; i < entityCount; i++)
		{
			// Break if the output buffer is full.
			if (writeIndex == outSize)
			{
				break;
			}

			outEntities[writeIndex] = entityGroup.getEntityAtIndex(i);
			writeIndex++;
		}
	};

	auto &staticGroup = this->staticGroups.get(chunk.x, chunk.y);
	auto &dynamicGroup = this->dynamicGroups.get(chunk.x, chunk.y);
	tryWriteEntities(staticGroup);
	tryWriteEntities(dynamicGroup);

	return writeIndex;
}

int EntityManager::getTotalEntities(const Entity **outEntities, int outSize) const
{
	// Apparently std::vector::data() can be either null or non-null when the container is empty,
	// so can't assume the given pointer is not null.
	if ((outEntities == nullptr) || (outSize == 0))
	{
		return 0;
	}

	// Fill the output buffer with as many entities as will fit.
	int writeIndex = 0;
	for (WEInt z = 0; z < this->staticGroups.getHeight(); z++)
	{
		for (SNInt x = 0; x < this->staticGroups.getWidth(); x++)
		{
			if (writeIndex == outSize)
			{
				break;
			}

			writeIndex += this->getTotalEntitiesInChunk(
				ChunkInt2(x, z), outEntities + writeIndex, outSize - writeIndex);
		}
	}

	return writeIndex;
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
	double ceilingHeight, const VoxelGrid &voxelGrid, const EntityDefinitionLibrary &entityDefLibrary,
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
	const double flatYOffset =  static_cast<double>(-baseYOffset) / MIFUtils::ARENA_UNITS;

	// If the entity is in a raised platform voxel, they are set on top of it.
	const CoordDouble2 &entityPosition = entity.getPosition();
	const double raisedPlatformYOffset = [ceilingHeight, &voxelGrid, &entityPosition]()
	{
		const NewDouble2 absoluteEntityPositionXZ = VoxelUtils::coordToNewPoint(entityPosition);
		const NewInt2 absoluteEntityVoxelPosXZ = VoxelUtils::pointToVoxel(absoluteEntityPositionXZ);
		const uint16_t voxelID = voxelGrid.getVoxel(absoluteEntityVoxelPosXZ.x, 1, absoluteEntityVoxelPosXZ.y);
		const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);

		if (voxelDef.type == ArenaTypes::VoxelType::Raised)
		{
			const VoxelDefinition::RaisedData &raised = voxelDef.raised;
			return (raised.yOffset + raised.ySize) * ceilingHeight;
		}
		else
		{
			// No raised platform offset.
			return 0.0;
		}
	}();

	// Bottom center of flat.
	const VoxelDouble3 newCoordPoint(
		entityPosition.point.x,
		ceilingHeight + flatYOffset + raisedPlatformYOffset,
		entityPosition.point.y);
	outVisData.flatPosition = CoordDouble3(entityPosition.chunk, newCoordPoint);
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

void EntityManager::updateEntityChunk(Entity *entity, const VoxelGrid &voxelGrid)
{
	if (entity == nullptr)
	{
		DebugLogWarning("Can't update null entity's chunk.");
		return;
	}

	// Find which chunk they were in.
	SNInt oldChunkX = -1;
	WEInt oldChunkZ = -1;

	auto tryGetEntityGroupInfo = [entity, &oldChunkX, &oldChunkZ](auto &entityGroups, auto **groupPtr)
	{
		// Find which entity group the given entity ID is in. This is a slow look-up because there is
		// no hint where the entity is at.
		for (WEInt z = 0; z < entityGroups.getHeight(); z++)
		{
			for (SNInt x = 0; x < entityGroups.getWidth(); x++)
			{
				auto &entityGroup = entityGroups.get(x, z);
				const std::optional<int> entityIndex = entityGroup.getEntityIndex(entity->getID());
				if (entityIndex.has_value())
				{
					oldChunkX = x;
					oldChunkZ = z;
					*groupPtr = &entityGroup;
					return true;
				}
			}
		}

		return false;
	};

	auto trySwapEntityGroup = [&oldChunkX, &oldChunkZ](Entity *entity, auto &oldGroup, auto &entityGroups)
	{
		auto swapEntityGroup = [](Entity *entity, auto &oldGroup, auto &newGroup)
		{
			newGroup.acquireEntity(entity->getID(), oldGroup);
		};

		const ChunkInt2 &newChunk = entity->getPosition().chunk;
		const bool groupHasChanged = (newChunk.x != oldChunkX) || (newChunk.y != oldChunkZ);
		if (groupHasChanged)
		{
			auto &newGroup = entityGroups.get(newChunk.x, newChunk.y);
			swapEntityGroup(entity, oldGroup, newGroup);
		}
	};

	if (entity->getEntityType() == EntityType::Static)
	{
		EntityGroup<StaticEntity> *staticEntityGroupPtr = nullptr;
		if (tryGetEntityGroupInfo(this->staticGroups, &staticEntityGroupPtr))
		{
			trySwapEntityGroup(entity, *staticEntityGroupPtr, this->staticGroups);
		}
	}
	else if (entity->getEntityType() == EntityType::Dynamic)
	{
		EntityGroup<DynamicEntity> *dynamicEntityGroupPtr = nullptr;
		if (tryGetEntityGroupInfo(this->dynamicGroups, &dynamicEntityGroupPtr))
		{
			trySwapEntityGroup(entity, *dynamicEntityGroupPtr, this->dynamicGroups);
		}
	}
	else
	{
		DebugLogError("Unhandled entity type \"" +
			std::to_string(static_cast<int>(entity->getEntityType())) + "\".");
	}
}

void EntityManager::remove(EntityID id)
{
	DebugAssert(this->staticGroups.getWidth() == this->dynamicGroups.getWidth());
	DebugAssert(this->staticGroups.getHeight() == this->dynamicGroups.getHeight());

	// Find which entity group the given entity ID is in. This is a slow look-up because there is
	// no hint where the entity is at.
	for (WEInt z = 0; z < this->staticGroups.getHeight(); z++)
	{
		for (SNInt x = 0; x < this->staticGroups.getWidth(); x++)
		{
			auto &staticGroup = this->staticGroups.get(x, z);
			std::optional<int> entityIndex = staticGroup.getEntityIndex(id);
			if (entityIndex.has_value())
			{
				// Static entity.
				staticGroup.remove(id);

				// Insert entity ID into the free list.
				this->freeIDs.push_back(id);
				return;
			}

			auto &dynamicGroup = this->dynamicGroups.get(x, z);
			entityIndex = dynamicGroup.getEntityIndex(id);
			if (entityIndex.has_value())
			{
				// Dynamic entity.
				dynamicGroup.remove(id);

				// Insert entity ID into the free list.
				this->freeIDs.push_back(id);
				return;
			}
		}
	}

	// Not in any entity group.
	DebugLogWarning("Tried to remove missing entity \"" + std::to_string(id) + "\".");
}

void EntityManager::clear()
{
	for (WEInt z = 0; z < this->staticGroups.getHeight(); z++)
	{
		for (SNInt x = 0; x < this->staticGroups.getWidth(); x++)
		{
			auto &staticGroup = this->staticGroups.get(x, z);
			auto &dynamicGroup = this->dynamicGroups.get(x, z);
			staticGroup.clear();
			dynamicGroup.clear();
		}
	}

	this->entityDefs.clear();
	this->freeIDs.clear();
	this->nextID = FIRST_ENTITY_ID;
}

void EntityManager::clearChunk(const ChunkInt2 &coord)
{
	auto &staticGroup = this->staticGroups.get(coord.x, coord.y);
	auto &dynamicGroup = this->dynamicGroups.get(coord.x, coord.y);
	staticGroup.clear();
	dynamicGroup.clear();
}

void EntityManager::tick(Game &game, double dt)
{
	// Only want to tick entities near the player, so get the chunks near the player.
	const ChunkInt2 playerChunk = [&game]()
	{
		auto &gameData = game.getGameData();
		const CoordDouble3 &playerCoord = gameData.getPlayer().getPosition();
		return playerCoord.chunk;
	}();

	const int chunkDistance = [&game]()
	{
		const auto &options = game.getOptions();
		return options.getMisc_ChunkDistance();
	}();

	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(playerChunk, chunkDistance, &minChunk, &maxChunk);

	auto tickNearbyEntityGroups = [&game, dt, &minChunk, &maxChunk](auto &entityGroups)
	{
		for (WEInt z = minChunk.y; z <= maxChunk.y; z++)
		{
			for (SNInt x = minChunk.x; x <= maxChunk.x; x++)
			{
				const bool coordIsValid = (x >= 0) && (x < entityGroups.getWidth()) &&
					(z >= 0) && (z < entityGroups.getHeight());

				if (coordIsValid)
				{
					auto &entityGroup = entityGroups.get(x, z);
					const int entityCount = entityGroup.getCount();

					for (int i = 0; i < entityCount; i++)
					{
						auto *entity = entityGroup.getEntityAtIndex(i);
						if (entity != nullptr)
						{
							entity->tick(game, dt);
						}
					}
				}
			}
		}
	};

	tickNearbyEntityGroups(this->staticGroups);
	tickNearbyEntityGroups(this->dynamicGroups);
}
