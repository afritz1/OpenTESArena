#ifndef ENTITY_REF_H
#define ENTITY_REF_H

#include <type_traits>

// Entity manager look-up wrapper to avoid dangling pointer issues. Can't cache chunk coordinates
// or index into entity group because it might change while this ref is in scope. Need to pass
// entity manager argument for const/non-const.

class Entity;

template <typename EntityManagerT>
class EntityRefWrapper
{
private:
	EntityManagerT *manager;
	EntityID id;
	EntityType type; // Hint for entity group.
public:
	EntityRefWrapper(EntityManagerT *manager, EntityID id, EntityType type)
	{
		this->manager = manager;
		this->id = id;
		this->type = type;
	}

	EntityID getID() const
	{
		return this->id;
	}

	template <typename C = EntityManagerT>
	typename std::enable_if_t<!std::is_const_v<C>, Entity*> get()
	{
		return this->manager->getEntityHandle(this->id, this->type);
	}

	template <typename C = EntityManagerT>
	typename std::enable_if_t<std::is_const_v<C>, const Entity*> get() const
	{
		return this->manager->getEntityHandle(this->id, this->type);
	}

	// Helpers functions for derived entity types.
	template <typename DerivedT>
	DerivedT *getDerived()
	{
		static_assert(std::is_base_of_v<Entity, DerivedT>);
		Entity *entity = this->get();
		return dynamic_cast<DerivedT*>(entity);
	}

	template <typename DerivedT>
	const DerivedT *getDerived() const
	{
		static_assert(std::is_base_of_v<Entity, DerivedT>);
		const Entity *entity = this->get();
		return dynamic_cast<const DerivedT*>(entity);
	}
};

using EntityRef = EntityRefWrapper<EntityManager>;
using ConstEntityRef = EntityRefWrapper<const EntityManager>;

#endif
