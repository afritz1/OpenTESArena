#ifndef SKY_DEFINITION_H
#define SKY_DEFINITION_H

#include <vector>

#include "../Math/MathUtils.h"
#include "../Media/Color.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/BufferView.h"

// Contains a location's distant sky values and objects (mountains, clouds, stars, etc.).
// Similar to LevelDefinition where it defines where various sky objects will be once they
// are instanced.

class SkyDefinition
{
public:
	using LandDefID = int;
	using AirDefID = int;
	using StarDefID = int;
	using SunDefID = int;
	using MoonDefID = int;

	struct LandPlacementDef
	{
		LandDefID id;
		std::vector<Radians> positions;

		LandPlacementDef(LandDefID id, std::vector<Radians> &&positions);
	};

	struct AirPlacementDef
	{
		AirDefID id;
		std::vector<std::pair<Radians, Radians>> positions;

		AirPlacementDef(AirDefID id, std::vector<std::pair<Radians, Radians>> &&positions);
	};

	struct StarPlacementDef
	{
		StarDefID id;
		std::vector<Double3> positions;

		StarPlacementDef(StarDefID id, std::vector<Double3> &&positions);
	};

	struct SunPlacementDef
	{
		SunDefID id; // @todo: remove bonus latitude from SunObjectDefinition
		std::vector<double> positions; // Bonus latitudes to combine with location latitude.
		// @todo: make no assumptions about the sun being at the horizon at 6am and just define like
		// an arbitrary star?

		SunPlacementDef(SunDefID id, std::vector<double> &&positions);
	};

	struct MoonPlacementDef
	{
		MoonDefID id;
		int imageIndex; // Index in moon definition phase images (full/half/new/etc.).
		// @todo: given the situation this placement def is in, maybe want the MoonObjectDefinition to support
		// all image IDs and then some phasePercent here would tell the moon instance which imageID to use.

		// @todo: phase day offset maybe?

		/*// Base position in the sky before latitude and time-of-day adjustments.
		Double3 baseDir;

		// Added to location latitude to get 'moon latitude'.
		double bonusLatitude;

		int phaseCount; // Number of days in period.
		int phaseIndexDayOffset; // Bias to phase start.*/

		//MoonPlacementDef(MoonDefID id);
	};
private:
	std::vector<LandPlacementDef> landPlacementDefs;
	std::vector<AirPlacementDef> airPlacementDefs;
	std::vector<StarPlacementDef> starPlacementDefs;
	std::vector<SunPlacementDef> sunPlacementDefs;
	std::vector<MoonPlacementDef> moonPlacementDefs;
	Buffer<Color> skyColors; // Colors for an entire day.
public:
	void init(Buffer<Color> &&skyColors);

	int getSkyColorCount() const;
	const Color &getSkyColor(int index);

	int getLandPlacementDefCount() const;
	const LandPlacementDef &getLandPlacementDef(int index) const;
	int getAirPlacementDefCount() const;
	const AirPlacementDef &getAirPlacementDef(int index) const;
	int getStarPlacementDefCount() const;
	const StarPlacementDef &getStarPlacementDef(int index) const;
	int getSunPlacementDefCount() const;
	const SunPlacementDef &getSunPlacementDef(int index) const;
	int getMoonPlacementDefCount() const;
	const MoonPlacementDef &getMoonPlacementDef(int index) const;

	void addLand(LandDefID id, Radians angle);
	void addAir(AirDefID id, Radians angleX, Radians angleY);
	void addStar(StarDefID id, const Double3 &direction);
	void addSun(SunDefID id, double bonusLatitude);
	void addMoon(MoonDefID id, int imageIndex, double bonusLatitude, double phasePercent);
};

#endif
