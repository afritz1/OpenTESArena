#ifndef ARTIFACT_PARSER_H
#define ARTIFACT_PARSER_H

#include <memory>
#include <vector>

class ArtifactData;

class ArtifactParser
{
private:
	static const std::string PATH;
	static const std::string FILENAME;
	
	ArtifactParser() = delete;
	ArtifactParser(const ArtifactParser&) = delete;
	~ArtifactParser() = delete;
public:
	static std::vector<std::unique_ptr<ArtifactData>> parse();
};

#endif
