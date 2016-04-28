#include <cassert>
#include <fstream>

#include "ArtifactParser.h"

#include "AccessoryArtifactData.h"
#include "ArtifactData.h"
#include "BodyArmorArtifactData.h"
#include "MiscellaneousArtifactData.h"
#include "ShieldArtifactData.h"
#include "WeaponArtifactData.h"
#include "../Utilities/File.h"

const std::string ArtifactParser::PATH = "data/text/";
const std::string ArtifactParser::FILENAME = "artifacts.txt";

std::vector<std::unique_ptr<ArtifactData>> ArtifactParser::parse()
{
	auto fullPath = ArtifactParser::PATH + ArtifactParser::FILENAME;

	// Read the artifacts file into a string.
	auto text = File::toString(fullPath);

	// Parsing code here... pushing artifacts into the vector.
	// ...
	// ...

	return std::vector<std::unique_ptr<ArtifactData>>();
}
