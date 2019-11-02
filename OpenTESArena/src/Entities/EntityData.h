#ifndef ENTITY_DATA_H
#define ENTITY_DATA_H

#include "EntityAnimationData.h"

class EntityData
{
private:
	EntityAnimationData animationData;

	// .INF flat index.
	// @todo: remove dependency on this .INF data index? I.e. just keep all the equivalent data
	// (entity double size, puddle, etc.) in this class.
	int flatIndex;
public:
	EntityData(int flatIndex);

	int getFlatIndex() const;
	EntityAnimationData &getAnimationData();
	const EntityAnimationData &getAnimationData() const;
};

#endif
