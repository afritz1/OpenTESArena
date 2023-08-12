#ifndef SKY_INSTANCE_H
#define SKY_INSTANCE_H

#include <cstdint>
#include <optional>
#include <vector>

#include "../Assets/TextureUtils.h"
#include "../Math/Vector3.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

class MapDefinition;
class Random;
class Renderer;
class SkyDefinition;
class SkyInfoDefinition;
class TextureManager;
class WeatherInstance;

enum class SkyObjectTextureType
{
	TextureAsset,
	PaletteIndex
};

// Determines how the sky object's mesh is anchored relative to its base position in the sky.
enum class SkyObjectPivotType
{
	Center,
	Bottom
};

// Shareable entries so each sky object doesn't need a whole buffer of texture assets.
struct SkyObjectTextureAssetEntry
{
	// Contains one element if there's no animation.
	Buffer<TextureAsset> textureAssets;
};

struct SkyObjectPaletteIndexEntry
{
	uint8_t paletteIndex;
};

using SkyObjectTextureAssetEntryID = int;
using SkyObjectPaletteIndexEntryID = int;

struct SkyObjectInstance
{
	Double3 baseDirection; // Position in sky before transformation.
	Double3 transformedDirection; // Position in sky usable by other systems (may be updated frequently).
	double width, height; // @todo: might change if this is a lightning bolt.

	SkyObjectTextureType textureType;
	union
	{
		SkyObjectTextureAssetEntryID textureAssetEntryID;
		SkyObjectPaletteIndexEntryID paletteIndexEntryID;
	};

	bool emissive;
	int animIndex; // Non-negative if this has an animation.
	SkyObjectPivotType pivotType;

	SkyObjectInstance();

	void initTextured(const Double3 &baseDirection, double width, double height, SkyObjectTextureAssetEntryID textureAssetEntryID, bool emissive, int animIndex);
	void initTextured(const Double3 &baseDirection, double width, double height, SkyObjectTextureAssetEntryID textureAssetEntryID, bool emissive);
	void initPaletteIndex(const Double3 &baseDirection, double width, double height, SkyObjectPaletteIndexEntryID paletteIndexEntryID, bool emissive);
};

struct SkyObjectAnimationInstance
{
	double targetSeconds, currentSeconds, percentDone;
	int skyObjectIndex;

	SkyObjectAnimationInstance();

	void init(int skyObjectIndex, double targetSeconds);
};

// Contains distant sky object instances and their animation state.
class SkyInstance
{
private:
	std::vector<SkyObjectTextureAssetEntry> textureAssetEntries;
	std::vector<SkyObjectPaletteIndexEntry> paletteIndexEntries;
	std::vector<SkyObjectInstance> skyObjectInsts; // Each sky object instance.
	std::vector<SkyObjectAnimationInstance> animInsts; // Data for each sky object with an animation.

	Buffer<int> lightningAnimIndices; // Non-empty during thunderstorm so animations can be updated.
	std::optional<int> currentLightningBoltObjectIndex; // Updated by WeatherInstance.

	bool tryGetTextureAssetEntryID(BufferView<const TextureAsset> textureAssets, SkyObjectTextureAssetEntryID *outID) const;
	bool tryGetPaletteIndexEntryID(uint8_t paletteIndex, SkyObjectPaletteIndexEntryID *outID) const;
public:
	// Start (inclusive) and end (exclusive) indices of each sky object type.
	int landStart, landEnd, airStart, airEnd, moonStart, moonEnd, sunStart, sunEnd, starStart, starEnd, lightningStart, lightningEnd;

	SkyInstance();

	void init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition, int currentDay, TextureManager &textureManager);

	const SkyObjectInstance &getSkyObjectInst(int index) const;
	const SkyObjectAnimationInstance &getAnimInst(int index) const;

	const SkyObjectTextureAssetEntry &getTextureAssetEntry(SkyObjectTextureAssetEntryID id) const;
	const SkyObjectPaletteIndexEntry &getPaletteIndexEntry(SkyObjectPaletteIndexEntryID id) const;

	// Whether the lightning bolt is currently visible due to thunderstorm state.
	bool isLightningVisible(int objectIndex) const;

	void update(double dt, double latitude, double daytimePercent, const WeatherInstance &weatherInst, Random &random);

	void clear();
};

#endif
