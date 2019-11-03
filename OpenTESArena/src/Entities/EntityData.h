#ifndef ENTITY_DATA_H
#define ENTITY_DATA_H

#include "EntityAnimationData.h"

class EntityData
{
private:
	EntityAnimationData animationData;

	// Several copied over from .INF data (not all, just for initial implementation).
	int yOffset;
	bool collider;
	bool puddle;
	bool doubleScale;
	bool dark;
	bool transparent;
	bool ceiling;

	// .INF flat index.
	// @todo: remove dependency on this .INF data index? I.e. just keep all the equivalent data
	// (entity double size, puddle, etc.) in this class.
	int flatIndex;
public:
	EntityData(int flatIndex, int yOffset, bool collider, bool puddle, bool doubleScale,
		bool dark, bool transparent, bool ceiling);

	int getFlatIndex() const;
	int getYOffset() const;
	bool isCollider() const;
	bool isPuddle() const;
	bool isDoubleScale() const;
	bool isDark() const;
	bool isTransparent() const;
	bool isOnCeiling() const;

	EntityAnimationData &getAnimationData();
	const EntityAnimationData &getAnimationData() const;
};

#endif
