#ifndef LOCATION_PARSER_H
#define LOCATION_PARSER_H

#include <memory>
#include <vector>

class Location;

class LocationParser
{
private:
	static const std::string PATH;
	static const std::string FILENAME;

	LocationParser() = delete;
	LocationParser(const LocationParser&) = delete;
	~LocationParser() = delete;
public:
	static std::vector<std::unique_ptr<Location>> parse();
};

#endif
