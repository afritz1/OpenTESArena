#ifndef ACCESSORY_ARTIFACT_DATA_H
#define ACCESSORY_ARTIFACT_DATA_H

#include "ArtifactData.h"

enum class AccessoryType;
enum class MetalType;

class AccessoryArtifactData : public ArtifactData
{
private:
	AccessoryType accessoryType;
	MetalType metalType;
public:
	AccessoryArtifactData(const std::string &displayName, 
		const std::string &flavorText, const std::vector<int> &provinceIDs,
		AccessoryType accessoryType, MetalType metalType);
	virtual ~AccessoryArtifactData();

	virtual std::unique_ptr<ArtifactData> clone() const override;
		
	AccessoryType getAccessoryType() const;
	MetalType getMetalType() const;

	virtual ItemType getItemType() const override;
};

#endif
