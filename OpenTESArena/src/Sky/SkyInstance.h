#ifndef SKY_INSTANCE_H
#define SKY_INSTANCE_H

#include <cstdint>
#include <optional>
#include <vector>

#include "../Assets/TextureUtils.h"
#include "../Math/Vector3.h"

// Contains distant sky object instances and their state.

// The renderer should only care about 1) current direction, 2) current texture ID, 3) anchor,
// and 4) shading type. Maybe also rendering order. It has the option of doing visibility culling
// as well.

class MapDefinition;
class Random;
class Renderer;
class SkyDefinition;
class SkyInfoDefinition;
class TextureManager;
class WeatherInstance;

class SkyInstance
{
private:
	struct LoadedSkyObjectTextureEntry
	{
		TextureAsset textureAsset;
		ScopedObjectTextureRef objectTextureRef;

		void init(const TextureAsset &textureAsset, ScopedObjectTextureRef &&objectTextureRef);
	};

	struct LoadedSmallStarTextureEntry
	{
		uint8_t paletteIndex;
		ScopedObjectTextureRef objectTextureRef;

		void init(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef);
	};

	class ObjectInstance
	{
	private:
		Double3 baseDirection; // Position in sky before transformation.
		Double3 transformedDirection; // Position in sky usable by other systems (may be updated frequently).
		double width, height;
		
		// Current texture of object (may change due to animation).
		ObjectTextureID objectTextureID;
		bool emissive;
	public:
		ObjectInstance();

		void init(const Double3 &baseDirection, double width, double height, ObjectTextureID objectTextureID, bool emissive);

		const Double3 &getBaseDirection() const;
		const Double3 &getTransformedDirection() const;
		double getWidth() const;
		double getHeight() const;
		ObjectTextureID getObjectTextureID() const;
		bool isEmissive() const;

		void setTransformedDirection(const Double3 &direction);

		// Intended for lightning bolt updating.
		void setDimensions(double width, double height);

		// Set when updating this sky object's animation.
		void setObjectTextureID(ObjectTextureID id);
	};

	// Animation data for each sky object with an animation.
	struct AnimInstance
	{
		int objectIndex;
		Buffer<ObjectTextureID> objectTextureIDs; // All object textures for the animation.
		double targetSeconds, currentSeconds;

		AnimInstance(int objectIndex, Buffer<ObjectTextureID> &&objectTextureIDs, double targetSeconds);
	};

	std::vector<LoadedSkyObjectTextureEntry> loadedSkyObjectTextures;
	std::vector<LoadedSmallStarTextureEntry> loadedSmallStarTextures;
	ScopedObjectTextureRef skyColorsTextureRef; // Renderer-allocated colors for all times during the day.

	std::vector<ObjectInstance> objectInsts; // Each sky object instance.
	std::vector<AnimInstance> animInsts; // Data for each sky object with an animation.

	int landStart, landEnd, airStart, airEnd, moonStart, moonEnd, sunStart, sunEnd, starStart, starEnd,
		lightningStart, lightningEnd;
	Buffer<int> lightningAnimIndices; // Non-empty during thunderstorm so animations can be updated.
	std::optional<int> currentLightningBoltObjectIndex; // Updated by WeatherInstance.
public:
	SkyInstance();

	void init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition,
		int currentDay, TextureManager &textureManager, Renderer &renderer);

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

	ObjectTextureID getSkyColorsTextureID() const;

	// Attempts to set this sky active in the renderer.
	// @todo: maybe this and LevelInstance::trySetActive() should be replaced by some MapInstance::trySetLevelActive(int)
	// that does the work for both the level and the sky.
	bool trySetActive(const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		TextureManager &textureManager, Renderer &renderer);

	void update(double dt, double latitude, double daytimePercent, const WeatherInstance &weatherInst,
		Random &random, const TextureManager &textureManager);
};

#endif
