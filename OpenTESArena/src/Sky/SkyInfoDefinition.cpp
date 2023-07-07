#include "SkyInfoDefinition.h"

#include "components/debug/Debug.h"

int SkyInfoDefinition::getLandCount() const
{
	return static_cast<int>(this->lands.size());
}

int SkyInfoDefinition::getAirCount() const
{
	return static_cast<int>(this->airs.size());
}

int SkyInfoDefinition::getStarCount() const
{
	return static_cast<int>(this->stars.size());
}

int SkyInfoDefinition::getSunCount() const
{
	return static_cast<int>(this->suns.size());
}

int SkyInfoDefinition::getMoonCount() const
{
	return static_cast<int>(this->moons.size());
}

int SkyInfoDefinition::getLightningCount() const
{
	return static_cast<int>(this->lightnings.size());
}

const SkyLandDefinition &SkyInfoDefinition::getLand(SkyDefinition::LandDefID id) const
{
	DebugAssertIndex(this->lands, id);
	return this->lands[id];
}

const SkyAirDefinition &SkyInfoDefinition::getAir(SkyDefinition::AirDefID id) const
{
	DebugAssertIndex(this->airs, id);
	return this->airs[id];
}

const SkyStarDefinition &SkyInfoDefinition::getStar(SkyDefinition::StarDefID id) const
{
	DebugAssertIndex(this->stars, id);
	return this->stars[id];
}

const SkySunDefinition &SkyInfoDefinition::getSun(SkyDefinition::SunDefID id) const
{
	DebugAssertIndex(this->suns, id);
	return this->suns[id];
}

const SkyMoonDefinition &SkyInfoDefinition::getMoon(SkyDefinition::MoonDefID id) const
{
	DebugAssertIndex(this->moons, id);
	return this->moons[id];
}

const SkyLightningDefinition &SkyInfoDefinition::getLightning(int index) const
{
	DebugAssertIndex(this->lightnings, index);
	return this->lightnings[index];
}

SkyDefinition::LandDefID SkyInfoDefinition::addLand(SkyLandDefinition &&def)
{
	this->lands.emplace_back(std::move(def));
	return static_cast<SkyDefinition::LandDefID>(this->lands.size()) - 1;
}

SkyDefinition::AirDefID SkyInfoDefinition::addAir(SkyAirDefinition &&def)
{
	this->airs.emplace_back(std::move(def));
	return static_cast<SkyDefinition::AirDefID>(this->airs.size()) - 1;
}

SkyDefinition::StarDefID SkyInfoDefinition::addStar(SkyStarDefinition &&def)
{
	this->stars.emplace_back(std::move(def));
	return static_cast<SkyDefinition::StarDefID>(this->stars.size()) - 1;
}

SkyDefinition::SunDefID SkyInfoDefinition::addSun(SkySunDefinition &&def)
{
	this->suns.emplace_back(std::move(def));
	return static_cast<SkyDefinition::SunDefID>(this->suns.size()) - 1;
}

SkyDefinition::MoonDefID SkyInfoDefinition::addMoon(SkyMoonDefinition &&def)
{
	this->moons.emplace_back(std::move(def));
	return static_cast<SkyDefinition::MoonDefID>(this->moons.size()) - 1;
}

void SkyInfoDefinition::addLightning(SkyLightningDefinition &&def)
{
	this->lightnings.emplace_back(std::move(def));
}
