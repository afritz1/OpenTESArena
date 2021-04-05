#include <algorithm>

#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "../Game/Game.h"
#include "../World/ChunkUtils.h"

Entity::Entity()
	: position(ChunkInt2::Zero, VoxelDouble2::Zero)
{
	this->id = EntityManager::NO_ID;
	this->defID = EntityManager::NO_DEF_ID;
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

void Entity::getViewDependentBBox2D(const CoordDouble2 &cameraCoord, CoordDouble2 *outMin, CoordDouble2 *outMax) const
{
	DebugAssert(this->defID != EntityManager::NO_DEF_ID);

	// @todo: get the animation frame that would be shown to the camera.
	// - get from EntityAnimationInstance
	DebugNotImplemented();
}

void Entity::getViewIndependentBBox2D(const EntityManager &entityManager,
	const EntityDefinitionLibrary &entityDefLibrary, CoordDouble2 *outMin, CoordDouble2 *outMax) const
{
	DebugAssert(this->defID != EntityManager::NO_DEF_ID);

	const EntityDefinition &entityDef = entityManager.getEntityDef(this->defID, entityDefLibrary);
	const EntityAnimationDefinition &animDef = entityDef.getAnimDef();

	// Get the largest width from the animation frames.
	double maxAnimWidth = 0.0;
	for (int i = 0; i < animDef.getStateCount(); i++)
	{
		const EntityAnimationDefinition::State &state = animDef.getState(i);
		for (int j = 0; j < state.getKeyframeListCount(); j++)
		{
			const EntityAnimationDefinition::KeyframeList &keyframeList = state.getKeyframeList(j);
			for (int k = 0; k < keyframeList.getKeyframeCount(); k++)
			{
				const EntityAnimationDefinition::Keyframe &keyframe = keyframeList.getKeyframe(k);
				maxAnimWidth = std::max(maxAnimWidth, keyframe.getWidth());
			}
		}
	}

	const double halfMaxWidth = maxAnimWidth * 0.50;

	// Orient the bounding box so it is largest with respect to the grid. Recalculate the coordinates in case
	// the min and max are in different chunks.
	*outMin = ChunkUtils::recalculateCoord(
		this->position.chunk,
		VoxelDouble2(this->position.point.x - halfMaxWidth, this->position.point.y - halfMaxWidth));
	*outMax = ChunkUtils::recalculateCoord(
		this->position.chunk,
		VoxelDouble2(this->position.point.x + halfMaxWidth, this->position.point.y + halfMaxWidth));
}

void Entity::setID(EntityID id)
{
	this->id = id;
}

void Entity::setPosition(const CoordDouble2 &position, EntityManager &entityManager)
{
	this->position = position;
	entityManager.updateEntityChunk(this);
}

void Entity::reset()
{
	// Don't change the entity type -- the entity manager doesn't change an allocation's entity
	// group between lifetimes.
	this->id = EntityManager::NO_ID;
	this->defID = EntityManager::NO_DEF_ID;
	this->position = CoordDouble2(ChunkInt2::Zero, VoxelDouble2::Zero);
	this->animInst.reset();
}

void Entity::tick(Game &game, double dt)
{
	const EntityAnimationDefinition &animDef = [this, &game]() -> const EntityAnimationDefinition&
	{
		const GameState &gameState = game.getGameState();
		const MapInstance &mapInst = gameState.getActiveMapInst();
		const LevelInstance &levelInst = mapInst.getActiveLevel();
		const EntityManager &entityManager = levelInst.getEntityManager();
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
