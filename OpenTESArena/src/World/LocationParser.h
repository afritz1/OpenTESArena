#ifndef LOCATION_PARSER_H
#define LOCATION_PARSER_H

#include <memory>
#include <vector>

// This class might need to be changed to load from INF and MIF files, or
// maybe that's the virtual file system's job.

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
