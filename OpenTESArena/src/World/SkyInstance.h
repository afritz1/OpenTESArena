#ifndef SKY_INSTANCE_H
#define SKY_INSTANCE_H

#include <cstdint>
#include <optional>
#include <vector>

#include "../Math/Vector3.h"
#include "../Media/TextureUtils.h"

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
	class ObjectInstance
	{
	public:
		// Slight breakage of the design since small stars don't have a 'texture'. In the future this should
		// allocate a renderer texture ID.
		enum class Type
		{
			General,
			SmallStar
		};

		struct General
		{
			// Current texture of object (may change due to animation).
			TextureBuilderID textureBuilderID;
			bool emissive;
		};

		struct SmallStar
		{
			uint8_t paletteIndex;
		};
	private:
		Double3 baseDirection; // Position in sky before transformation.
		Double3 transformedDirection; // Position in sky usable by other systems (may be updated frequently).
		double width, height;
		Type type;

		union
		{
			General general;
			SmallStar smallStar;
		};

		void init(Type type, const Double3 &baseDirection, double width, double height);
	public:
		ObjectInstance();

		void initGeneral(const Double3 &baseDirection, double width, double height, TextureBuilderID textureBuilderID,
			bool emissive);
		void initSmallStar(const Double3 &baseDirection, double width, double height, uint8_t paletteIndex);

		Type getType() const;
		const Double3 &getBaseDirection() const;
		const Double3 &getTransformedDirection() const;
		double getWidth() const;
		double getHeight() const;
		General &getGeneral();
		const General &getGeneral() const;
		const SmallStar &getSmallStar() const;

		void setTransformedDirection(const Double3 &direction);

		// Intended for lightning bolt updating.
		void setDimensions(double width, double height);
	};

	// Animation data for each sky object with an animation.
	struct AnimInstance
	{
		int objectIndex;
		TextureBuilderIdGroup textureBuilderIDs; // All texture IDs for the animation.
		double targetSeconds, currentSeconds;

		AnimInstance(int objectIndex, const TextureBuilderIdGroup &textureBuilderIDs, double targetSeconds);
	};

	struct LightningState
	{
		int objectIndex;
		int animIndex; // So the texture IDs can be set to another group.

		LightningState(int objectIndex, int animIndex);
	};

	std::vector<ObjectInstance> objectInsts; // Each sky object instance.
	std::vector<AnimInstance> animInsts; // Data for each sky object with an animation.
	Buffer<Buffer<TextureBuilderID>> lightningTextureBuilderIdGroups; // Textures for each lightning bolt.
	int landStart, landEnd, airStart, airEnd, moonStart, moonEnd, sunStart, sunEnd, starStart, starEnd;
	std::optional<LightningState> lightningState;
	bool lightningBoltIsVisible; // Updated by WeatherInstance.
public:
	SkyInstance();

	void init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition,
		int currentDay, TextureManager &textureManager);

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
	std::optional<int> getLightningIndex() const;

	// @todo: this is bad design; there should not be a small star type.
	bool isObjectSmallStar(int objectIndex) const;

	// Whether the lightning bolt is currently visible due to thunderstorm state.
	bool isLightningVisible() const;

	void getObject(int index, Double3 *outDirection, TextureBuilderID *outTextureBuilderID, bool *outEmissive,
		double *outWidth, double *outHeight) const;

	// @todo: this is bad design; there should not be a small star type. Eventually get renderer
	// resource IDs instead probably.
	void getObjectSmallStar(int index, Double3 *outDirection, uint8_t *outPaletteIndex, double *outWidth,
		double *outHeight) const;

	// @todo: this is public for the renderer for now. Remove this once public texture handles are being allocated.
	// Gets the texture builder ID(s) for a non-small-star object. If it doesn't have an animation then there is only
	// one ID.
	TextureBuilderIdGroup getObjectTextureBuilderIDs(int index) const;
	std::optional<double> tryGetObjectAnimPercent(int index) const;

	// Attempts to set this sky active in the renderer.
	// @todo: maybe this and LevelInstance::trySetActive() should be replaced by some MapInstance::trySetLevelActive(int)
	// that does the work for both the level and the sky.
	bool trySetActive(const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
		TextureManager &textureManager, Renderer &renderer);

	void update(double dt, double latitude, double daytimePercent, const WeatherInstance &weatherInst,
		Random &random, const TextureManager &textureManager);
};

#endif
