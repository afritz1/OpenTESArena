#ifndef ENTITY_DATA_H
#define ENTITY_DATA_H

#include "EntityAnimationData.h"

class EntityData
{
private:
	EntityAnimationData animationData;

	// @todo: convert to discriminated union like VoxelData so we can have creature
	// properties like blood animation index, "has no corpse", etc..

	// Several copied over from .INF data (not all, just for initial implementation).
	int yOffset;
	bool collider;
	bool puddle;
	bool largeScale;
	bool dark;
	bool transparent;
	bool ceiling;
	bool mediumScale;

	// .INF flat index.
	// @todo: remove dependency on this .INF data index? I.e. just keep all the equivalent data
	// (entity double size, puddle, etc.) in this class.
	int flatIndex;
public:
	EntityData();

	void init(int flatIndex, int yOffset, bool collider, bool puddle, bool largeScale,
		bool dark, bool transparent, bool ceiling, bool mediumScale);

	int getFlatIndex() const;
	int getYOffset() const;
	bool isCollider() const;
	bool isPuddle() const;
	bool isLargeScale() const;
	bool isDark() const;
	bool isTransparent() const;
	bool isOnCeiling() const;
	bool isMediumScale() const;

	EntityAnimationData &getAnimationData();
	const EntityAnimationData &getAnimationData() const;
};

#endif
