#include <algorithm>

#include "SkyDefinition.h"

#include "components/debug/Debug.h"

SkyDefinition::LandPlacementDef::LandPlacementDef(LandDefID id, std::vector<Radians> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

SkyDefinition::AirPlacementDef::AirPlacementDef(AirDefID id,
	std::vector<std::pair<Radians, Radians>> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

SkyDefinition::StarPlacementDef::StarPlacementDef(StarDefID id, std::vector<Double3> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

SkyDefinition::SunPlacementDef::SunPlacementDef(SunDefID id, std::vector<double> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

SkyDefinition::MoonPlacementDef::Position::Position(const Double3 &baseDir, double orbitPercent,
	double bonusLatitude, int imageIndex)
	: baseDir(baseDir)
{
	this->orbitPercent = orbitPercent;
	this->bonusLatitude = bonusLatitude;
	this->imageIndex = imageIndex;
}

SkyDefinition::MoonPlacementDef::MoonPlacementDef(MoonDefID id,
	std::vector<MoonPlacementDef::Position> &&positions)
	: positions(std::move(positions))
{
	this->id = id;
}

void SkyDefinition::init(Buffer<Color> &&skyColors)
{
	this->skyColors = std::move(skyColors);
}

int SkyDefinition::getSkyColorCount() const
{
	return this->skyColors.getCount();
}

const Color &SkyDefinition::getSkyColor(int index)
{
	return this->skyColors.get(index);
}

int SkyDefinition::getLandPlacementDefCount() const
{
	return static_cast<int>(this->landPlacementDefs.size());
}

const SkyDefinition::LandPlacementDef &SkyDefinition::getLandPlacementDef(int index) const
{
	DebugAssertIndex(this->landPlacementDefs, index);
	return this->landPlacementDefs[index];
}

int SkyDefinition::getAirPlacementDefCount() const
{
	return static_cast<int>(this->airPlacementDefs.size());
}

const SkyDefinition::AirPlacementDef &SkyDefinition::getAirPlacementDef(int index) const
{
	DebugAssertIndex(this->airPlacementDefs, index);
	return this->airPlacementDefs[index];
}

int SkyDefinition::getStarPlacementDefCount() const
{
	return static_cast<int>(this->starPlacementDefs.size());
}

const SkyDefinition::StarPlacementDef &SkyDefinition::getStarPlacementDef(int index) const
{
	DebugAssertIndex(this->starPlacementDefs, index);
	return this->starPlacementDefs[index];
}

int SkyDefinition::getSunPlacementDefCount() const
{
	return static_cast<int>(this->sunPlacementDefs.size());
}

const SkyDefinition::SunPlacementDef &SkyDefinition::getSunPlacementDef(int index) const
{
	DebugAssertIndex(this->sunPlacementDefs, index);
	return this->sunPlacementDefs[index];
}

int SkyDefinition::getMoonPlacementDefCount() const
{
	return static_cast<int>(this->moonPlacementDefs.size());
}

const SkyDefinition::MoonPlacementDef &SkyDefinition::getMoonPlacementDef(int index) const
{
	DebugAssertIndex(this->moonPlacementDefs, index);
	return this->moonPlacementDefs[index];
}

void SkyDefinition::addLand(LandDefID id, Radians angle)
{
	const auto iter = std::find_if(this->landPlacementDefs.begin(), this->landPlacementDefs.end(),
		[id](const LandPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->landPlacementDefs.end())
	{
		std::vector<Radians> &positions = iter->positions;
		positions.push_back(angle);
	}
	else
	{
		this->landPlacementDefs.emplace_back(id, std::vector<Radians> { angle });
	}
}

void SkyDefinition::addAir(AirDefID id, Radians angleX, Radians angleY)
{
	const auto iter = std::find_if(this->airPlacementDefs.begin(), this->airPlacementDefs.end(),
		[id](const AirPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->airPlacementDefs.end())
	{
		std::vector<std::pair<Radians, Radians>> &positions = iter->positions;
		positions.emplace_back(angleX, angleY);
	}
	else
	{
		this->airPlacementDefs.emplace_back(id,
			std::vector<std::pair<Radians, Radians>> { std::make_pair(angleX, angleY) });
	}
}

void SkyDefinition::addStar(StarDefID id, const Double3 &direction)
{
	const auto iter = std::find_if(this->starPlacementDefs.begin(), this->starPlacementDefs.end(),
		[id](const StarPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->starPlacementDefs.end())
	{
		std::vector<Double3> &positions = iter->positions;
		positions.emplace_back(direction);
	}
	else
	{
		this->starPlacementDefs.emplace_back(id, std::vector<Double3> { direction });
	}
}

void SkyDefinition::addSun(SunDefID id, double bonusLatitude)
{
	const auto iter = std::find_if(this->sunPlacementDefs.begin(), this->sunPlacementDefs.end(),
		[id](const SunPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->sunPlacementDefs.end())
	{
		std::vector<double> &positions = iter->positions;
		positions.push_back(bonusLatitude);
	}
	else
	{
		this->sunPlacementDefs.emplace_back(id, std::vector<double> { bonusLatitude });
	}
}

void SkyDefinition::addMoon(MoonDefID id, const Double3 &baseDir, double orbitPercent,
	double bonusLatitude, int imageIndex)
{
	const auto iter = std::find_if(this->moonPlacementDefs.begin(), this->moonPlacementDefs.end(),
		[id](const MoonPlacementDef &def)
	{
		return def.id == id;
	});

	if (iter != this->moonPlacementDefs.end())
	{
		std::vector<MoonPlacementDef::Position> &positions = iter->positions;
		positions.emplace_back(baseDir, orbitPercent, bonusLatitude, imageIndex);
	}
	else
	{
		std::vector<MoonPlacementDef::Position> positions;
		positions.emplace_back(baseDir, orbitPercent, bonusLatitude, imageIndex);
		this->moonPlacementDefs.emplace_back(id, std::move(positions));
	}
}
