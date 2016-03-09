#include <cassert>
#include <map>

#include "Climate.h"

const auto ClimateDisplayNames = std::map<ClimateName, std::string>
{
	{ ClimateName::Cold, "Cold" },
	{ ClimateName::Desert, "Desert" },
	{ ClimateName::Grassy, "Grassy" },
	{ ClimateName::Snowy, "Snowy" }
};

Climate::Climate(ClimateName climateName)
{
	this->climateName = climateName;
}

Climate::~Climate()
{

}

const ClimateName &Climate::getClimateName() const
{
	return this->climateName;
}

std::string Climate::toString() const
{
	auto displayName = ClimateDisplayNames.at(this->getClimateName());
	assert(displayName.size() > 0);
	return displayName;
}
