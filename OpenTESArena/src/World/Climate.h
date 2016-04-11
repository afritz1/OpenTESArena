#ifndef CLIMATE_H
#define CLIMATE_H

#include <string>

#include "ClimateName.h"

class Climate
{
private:
	ClimateName climateName;
public:
	Climate(ClimateName climateName);
	~Climate();

	const ClimateName &getClimateName() const;
	std::string toString() const;
};

#endif
