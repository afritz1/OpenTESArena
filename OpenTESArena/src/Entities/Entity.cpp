#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/Game.h"

Entity::Entity()
	: position(ChunkInt2::Zero, VoxelDouble2::Zero)
{
	this->id = EntityManager::NO_ID;
	this->defID = EntityManager::NO_DEF_ID;
	this->renderID = EntityManager::NO_RENDER_ID;
	this->animInst.reset();
}

void Entity::init(EntityDefID defID, const EntityAnimationInstance &animInst)
{
	DebugAssert(this->id != EntityManager::NO_ID);
	this->defID = defID;
	this->animInst = animInst;
}

EntityID Entity::getID() const
{
	return this->id;
}

EntityDefID Entity::getDefinitionID() const
{
	return this->defID;
}

EntityRenderID Entity::getRenderID() const
{
	return this->renderID;
}

const CoordDouble2 &Entity::getPosition() const
{
	return this->position;
}

EntityAnimationInstance &Entity::getAnimInstance()
{
	return this->animInst;
}

const EntityAnimationInstance &Entity::getAnimInstance() const
{
	return this->animInst;
}

void Entity::setID(EntityID id)
{
	this->id = id;
}

void Entity::setRenderID(EntityRenderID id)
{
	this->renderID = id;
}

void Entity::setPosition(const CoordDouble2 &position, EntityManager &entityManager,
	const VoxelGrid &voxelGrid)
{
	this->position = position;
	entityManager.updateEntityChunk(this, voxelGrid);
}

void Entity::reset()
{
	// Don't change the entity type -- the entity manager doesn't change an allocation's entity
	// group between lifetimes.
	this->id = EntityManager::NO_ID;
	this->defID = EntityManager::NO_DEF_ID;
	this->renderID = EntityManager::NO_RENDER_ID;
	this->position = CoordDouble2(ChunkInt2::Zero, VoxelDouble2::Zero);
	this->animInst.reset();
}

void Entity::tick(Game &game, double dt)
{
	const EntityAnimationDefinition &animDef = [this, &game]() -> const EntityAnimationDefinition&
	{
		const WorldData &worldData = game.getGameData().getActiveWorld();
		const LevelData &levelData = worldData.getActiveLevel();
		const EntityManager &entityManager = levelData.getEntityManager();
		const EntityDefinitionLibrary &entityDefLibrary = game.getEntityDefinitionLibrary();
		const EntityDefinition &entityDef = entityManager.getEntityDef(
			this->getDefinitionID(), entityDefLibrary);
		return entityDef.getAnimDef();
	}();

	// Get current animation keyframe from instance, so we know which anim def state to get.
	const int stateIndex = this->animInst.getStateIndex();
	const EntityAnimationDefinition::State &animDefState = animDef.getState(stateIndex);

	// Animate.
	// @todo: maybe want to add an 'isRandom' bool to EntityAnimationDefinition::State so
	// it can more closely match citizens' animations from the original game. Either that
	// or have a separate tickRandom() method so it's more optimizable.
	this->animInst.tick(dt, animDefState.getTotalSeconds(), animDefState.isLooping());
}
