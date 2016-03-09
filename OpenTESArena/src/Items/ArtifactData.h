#ifndef ARTIFACT_DATA_H
#define ARTIFACT_DATA_H

#include <string>
#include <vector>

// ArtifactData will be abstract, and will be implemented by each concrete item
// that has an artifact.

// It could just be an attribute... hmm, a nullable class member! I'm thinking of 
// it more as an extra tooltip message than its own type, kind of like World of 
// Warcraft legendaries. An artifact is an existing item with some effect(s) and 
// flavor text.

enum class ArtifactName;
enum class ProvinceName;

class ArtifactData
{
public:
	ArtifactData();
	virtual ~ArtifactData();

	static std::vector<ArtifactName> getArtifactsInProvince(ProvinceName province);

	virtual ArtifactName getParentArtifactName() const = 0;
	virtual std::string getDisplayName() const = 0;
	virtual std::string getFlavorText() const = 0;
};

#endif