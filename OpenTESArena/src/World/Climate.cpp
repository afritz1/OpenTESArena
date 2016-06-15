#include <cassert>
#include <map>

#include "Climate.h"

#include "ClimateName.h"

const std::map<ClimateName, std::string> ClimateDisplayNames =
{
	{ ClimateName::Cold, "Cold" },
	{ ClimateName::Desert, "Desert" },
	{ ClimateName::Grassy, "Grassy" },
	{ ClimateName::Interior, "Interior" },
	{ ClimateName::Snowy, "Snowy" }
};

Climate::Climate(ClimateName climateName)
{
	this->climateName = climateName;
}

Climate::~Climate()
{

}

ClimateName Climate::getClimateName() const
{
	return this->climateName;
}

std::string Climate::toString() const
{
	auto displayName = ClimateDisplayNames.at(this->getClimateName());
	return displayName;
}
