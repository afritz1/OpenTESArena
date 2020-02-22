#ifndef ENTITY_DEFINITION_H
#define ENTITY_DEFINITION_H

#include <optional>
#include <string>
#include <string_view>

#include "EntityAnimationData.h"

class EntityDefinition
{
private:
	std::string displayName;

	EntityAnimationData animationData;

	// @todo: convert to discriminated union like VoxelDefinition so we can have creature
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
	bool streetLight;
	std::optional<int> lightIntensity;
	std::optional<uint8_t> creatureSoundIndex;

	// .INF flat index.
	// @todo: remove dependency on this .INF data index? I.e. just keep all the equivalent data
	// (entity double size, puddle, etc.) in this class.
	int flatIndex;
public:
	EntityDefinition();

	void init(std::string &&displayName, int flatIndex, int yOffset, bool collider, bool puddle,
		bool largeScale, bool dark, bool transparent, bool ceiling, bool mediumScale,
		bool streetLight, const std::optional<int> &lightIntensity,
		const std::optional<uint8_t> &creatureSoundIndex);

	std::string_view getDisplayName() const;
	int getFlatIndex() const;
	int getYOffset() const;
	bool isCollider() const;
	bool isPuddle() const;
	bool isLargeScale() const;
	bool isDark() const;
	bool isTransparent() const;
	bool isOnCeiling() const;
	bool isMediumScale() const;
	bool isStreetLight() const;
	const int *getLightIntensity() const;
	const uint8_t *getCreatureSoundIndex() const;

	EntityAnimationData &getAnimationData();
	const EntityAnimationData &getAnimationData() const;
};

#endif
