#include <cassert>

#include "LocationParser.h"

#include "Location.h"
#include "../Utilities/File.h"

const std::string LocationParser::PATH = "data/text/";
const std::string LocationParser::FILENAME = "locations.txt";

std::vector<std::unique_ptr<Location>> LocationParser::parse()
{
	auto fullPath = LocationParser::PATH + LocationParser::FILENAME;

	// Read the locations file into a string.
	auto text = File::toString(fullPath);

	// Parsing code here... pushing locations into the vector.
	// ...
	// ...

	return std::vector<std::unique_ptr<Location>>();
}
