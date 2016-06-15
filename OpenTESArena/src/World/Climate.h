#ifndef CLIMATE_H
#define CLIMATE_H

#include <string>

enum class ClimateName;

class Climate
{
private:
	ClimateName climateName;
public:
	Climate(ClimateName climateName);
	~Climate();

	ClimateName getClimateName() const;
	std::string toString() const;
};

#endif
