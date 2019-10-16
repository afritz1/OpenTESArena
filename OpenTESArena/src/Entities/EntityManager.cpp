#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"

#include "components/debug/Debug.h"

const int EntityManager::NO_ID = -1;

int EntityManager::entityComparer(const std::unique_ptr<Entity> &a, const std::unique_ptr<Entity> &b)
{
	return a->getID() < b->getID();
}

EntityManager::EntityList &EntityManager::getEntityList(EntityType entityType)
{
	const int index = static_cast<int>(entityType);
	DebugAssertIndex(this->entityLists, index);
	return this->entityLists[index];
}

const EntityManager::EntityList &EntityManager::getEntityList(EntityType entityType) const
{
	const int index = static_cast<int>(entityType);
	DebugAssertIndex(this->entityLists, index);
	return this->entityLists[index];
}

int EntityManager::getEntityInsertIndex(int id, const EntityList &entityList)
{
	DebugAssert(id != EntityManager::NO_ID);

	// Increase index until given entity ID is greater.
	int index = 0;
	for (const auto &entity : entityList)
	{
		const int currentID = entity->getID();
		DebugAssert(currentID != id);

		if (currentID < id)
		{
			index++;
		}
		else
		{
			break;
		}
	}

	return index;
}

bool EntityManager::tryGetEntityIndex(int id, const EntityList &entityList, int *outIndex)
{
	// @todo: maybe make this function a function of a new EntityGroup class, and
	// make the EntityGroup have a vector of IDs that can be binary-searched here, so
	// we can get the entity index in log(n) time.

	// Assume the list is sorted by ID, to allow for early-out.
	DebugAssert(std::is_sorted(entityList.begin(), entityList.end(),
		EntityManager::entityComparer));

	const int entityCount = static_cast<int>(entityList.size());
	for (int i = 0; i < entityCount; i++)
	{
		const std::unique_ptr<Entity> &entity = entityList[i];
		const int entityID = entity->getID();

		if (entityID == id)
		{
			*outIndex = i;
			return true;
		}
		else if (entityID > id)
		{
			break;
		}
	}

	return false;
}

bool EntityManager::tryGetEntityListIndex(int id, int *outIndex) const
{
	const int entityListCount = static_cast<int>(this->entityLists.size());

	for (int i = 0; i < entityListCount; i++)
	{
		const auto &entityList = this->entityLists[i];

		int dummy;
		if (EntityManager::tryGetEntityIndex(id, entityList, &dummy))
		{
			*outIndex = i;
			return true;
		}
	}

	return false;
}

int EntityManager::getFreeID() const
{
	// Iterate through IDs from 0 to infinity until one is available.
	int id = 0;
	while (this->get(id) == nullptr)
	{
		id++;
	}

	return id;
}

Entity *EntityManager::get(int id) const
{
	// See if the entity exists in any entity list.
	int entityListIndex;
	if (this->tryGetEntityListIndex(id, &entityListIndex))
	{
		const auto &entityList = this->entityLists[entityListIndex];

		// Get the entity's index in the entity list.
		int entityIndex;
		if (EntityManager::tryGetEntityIndex(id, entityList, &entityIndex))
		{
			return entityList[entityIndex].get();
		}
		else
		{
			// Entity not found.
			return nullptr;
		}
	}
	else
	{
		// Entity list not found.
		return nullptr;
	}
}

int EntityManager::getCount(EntityType entityType) const
{
	const EntityList &entityList = this->getEntityList(entityType);
	return static_cast<int>(entityList.size());
}

int EntityManager::getTotalCount() const
{
	int count = 0;
	for (const auto &entityList : this->entityLists)
	{
		count += static_cast<int>(entityList.size());
	}

	return count;
}

int EntityManager::getEntities(EntityType entityType, Entity **outEntities, int outSize)
{
	DebugAssert(outEntities != nullptr);
	DebugAssert(outSize >= 0);

	// Get entity list for the given entity type.
	const auto &entityList = this->getEntityList(entityType);
	const int writeCount = std::min(static_cast<int>(entityList.size()), outSize);
	for (int i = 0; i < writeCount; i++)
	{
		const std::unique_ptr<Entity> &entity = entityList[i];
		outEntities[i] = entity.get();
	}

	return writeCount;
}

int EntityManager::getTotalEntities(Entity **outEntities, int outSize)
{
	DebugAssert(outEntities != nullptr);
	DebugAssert(outSize >= 0);

	int writeIndex = 0;
	for (const auto &entityList : this->entityLists)
	{
		for (const std::unique_ptr<Entity> &entity : entityList)
		{
			if (writeIndex == outSize)
			{
				return writeIndex;
			}

			outEntities[writeIndex] = entity.get();
			writeIndex++;
		}
	}

	return writeIndex;
}

Entity *EntityManager::add(std::unique_ptr<Entity> entity)
{
	DebugAssert(entity.get() != nullptr);
	DebugAssert(entity->getID() == EntityManager::NO_ID);

	// Assign entity ID.
	entity->setID(this->getFreeID());

	// Get entity list for the entity.
	EntityList &entityList = this->getEntityList(entity->getEntityType());
	DebugAssert(std::is_sorted(entityList.begin(), entityList.end(),
		EntityManager::entityComparer));

	Entity *entityPtr = entity.get();

	// Insert the entity into the sorted list.
	const int insertIndex = EntityManager::getEntityInsertIndex(entity->getID(), entityList);
	entityList.insert(entityList.begin() + insertIndex, std::move(entity));

	return entityPtr;
}

void EntityManager::remove(int id)
{
	int entityListIndex;
	if (this->tryGetEntityListIndex(id, &entityListIndex))
	{
		auto &entityList = this->entityLists[entityListIndex];

		int entityIndex;
		if (EntityManager::tryGetEntityIndex(id, entityList, &entityIndex))
		{
			entityList.erase(entityList.begin() + entityIndex);
		}
	}
}
