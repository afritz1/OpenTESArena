#ifndef SKY_INSTANCE_H
#define SKY_INSTANCE_H

#include <cstdint>
#include <vector>

#include "../Math/Vector3.h"
#include "../Media/TextureUtils.h"

// Contains distant sky object instances and their state.

// The renderer should only care about 1) current direction, 2) current texture ID, 3) anchor,
// and 4) shading type. Maybe also rendering order. It has the option of doing visibility culling
// as well.

class SkyDefinition;
class SkyInfoDefinition;
class TextureManager;

class SkyInstance
{
private:
	class ObjectInstance
	{
	public:
		// Slight breakage of the design since small stars don't have a 'texture'; in the future this should
		// allocate a renderer texture ID instead of assuming 8-bit image handles.
		enum class Type
		{
			General,
			SmallStar
		};

		struct General
		{
			// Current texture of object (may change due to animation).
			TextureBuilderID textureBuilderID;
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

		void initGeneral(const Double3 &baseDirection, double width, double height, TextureBuilderID textureBuilderID);
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
	};

	// Animation data for each sky object with an animation.
	struct AnimInstance
	{
		int objectIndex;
		TextureBuilderIdGroup textureBuilderIDs; // All texture IDs for the animation.
		double targetSeconds, currentSeconds;

		AnimInstance(int objectIndex, const TextureBuilderIdGroup &textureBuilderIDs, double targetSeconds);
	};

	std::vector<ObjectInstance> objectInsts; // Each sky object instance.
	std::vector<AnimInstance> animInsts; // Data for each sky object with an animation.
	int landStart, landEnd, airStart, airEnd, starStart, starEnd, sunStart, sunEnd, moonStart, moonEnd;
public:
	void init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition,
		TextureManager &textureManager);

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

	// @todo: this is bad design; there should not be a small star type.
	bool isObjectSmallStar(int objectIndex) const;

	void getObject(int index, Double3 *outDirection, TextureBuilderID *outTextureBuilderID, double *outWidth,
		double *outHeight) const;

	// @todo: this is bad design; there should not be a small star type. Eventually get renderer
	// resource IDs instead probably.
	void getObjectSmallStar(int index, Double3 *outDirection, uint8_t *outPaletteIndex, double *outWidth,
		double *outHeight) const;

	void update(double dt, double latitude, double daytimePercent);
};

#endif
