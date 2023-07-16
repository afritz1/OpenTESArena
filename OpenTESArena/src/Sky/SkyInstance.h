#ifndef SKY_INSTANCE_H
#define SKY_INSTANCE_H

#include <cstdint>
#include <optional>
#include <vector>

#include "../Assets/TextureUtils.h"
#include "../Math/Vector3.h"

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
	Buffer<Int2> dimensions;
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
	double width, height; // Might change if this is a lightning bolt.

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

	void init(const Double3 &baseDirection, double width, double height, ObjectTextureID objectTextureID, bool emissive);
};

struct SkyObjectAnimationInstance
{
	Buffer<TextureAsset> textureAssets;
	double targetSeconds, currentSeconds;

	SkyObjectAnimationInstance(Buffer<TextureAsset> &&textureAssets, double targetSeconds);
};

// Contains distant sky object instances and their animation state.
class SkyInstance
{
private:
	std::vector<SkyObjectTextureAssetEntry> textureAssetEntries;
	std::vector<SkyObjectPaletteIndexEntry> paletteIndexEntries;
	std::vector<SkyObjectInstance> objectInsts; // Each sky object instance.
	std::vector<SkyObjectAnimationInstance> animInsts; // Data for each sky object with an animation.

	int landStart, landEnd, airStart, airEnd, moonStart, moonEnd, sunStart, sunEnd, starStart, starEnd, lightningStart, lightningEnd;
	Buffer<int> lightningAnimIndices; // Non-empty during thunderstorm so animations can be updated.
	std::optional<int> currentLightningBoltObjectIndex; // Updated by WeatherInstance.

	bool tryGetTextureAssetEntryID(BufferView<const TextureAsset> textureAssets, SkyObjectTextureAssetEntryID *outID) const;
	bool tryGetPaletteIndexEntryID(uint8_t paletteIndex, SkyObjectPaletteIndexEntryID *outID) const;
public:
	SkyInstance();

	void init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition, int currentDay, TextureManager &textureManager);

	// Start (inclusive) and end (exclusive) indices of each sky object type.
	int getLandStartIndex() const;
	int getLandEndIndex() const;
	int getAirStartIndex() const;
	int getAirEndIndex() const;
	int getStarStartIndex() const;
	int getStarEndIndex() const;
	int getSunStartIndex() const;
	int getSunEndIndex() const;
	int getMoonStartIndex() const;
	int getMoonEndIndex() const;
	int getLightningStartIndex() const;
	int getLightningEndIndex() const;

	// Whether the lightning bolt is currently visible due to thunderstorm state.
	bool isLightningVisible(int objectIndex) const;

	void getSkyObject(int index, Double3 *outDirection, ObjectTextureID *outObjectTextureID, bool *outEmissive,
		double *outWidth, double *outHeight) const;

	std::optional<double> tryGetObjectAnimPercent(int index) const;

	void update(double dt, double latitude, double daytimePercent, const WeatherInstance &weatherInst,
		Random &random, const TextureManager &textureManager);

	void clear();
};

#endif
