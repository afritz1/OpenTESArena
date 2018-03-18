#ifndef ARTIFACT_DATA_H
#define ARTIFACT_DATA_H

#include <memory>
#include <string>
#include <vector>

// ArtifactData will be abstract, and will be implemented by each concrete item
// that has at least one artifact.

// It could just be an attribute... hmm, a nullable class member! I'm thinking of 
// it more as an extra tooltip message than its own type, kind of like World of 
// Warcraft legendaries. An artifact is an existing item with some effect(s) and 
// flavor text.

enum class ItemType;

class ArtifactData
{
private:
	std::string displayName, flavorText;
	std::vector<int> provinceIDs;
public:
	ArtifactData(const std::string &displayName, const std::string &flavorText,
		const std::vector<int> &provinceIDs);
	virtual ~ArtifactData() = default;

	virtual std::unique_ptr<ArtifactData> clone() const = 0;

	const std::string &getDisplayName() const;
	const std::string &getFlavorText() const;
	const std::vector<int> &getProvinceIDs() const;

	virtual ItemType getItemType() const = 0;
};

#endif
