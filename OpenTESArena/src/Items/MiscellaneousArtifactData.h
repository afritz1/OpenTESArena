#ifndef MISCELLANEOUS_ARTIFACT_DATA_H
#define MISCELLANEOUS_ARTIFACT_DATA_H

#include <string>

#include "ArtifactData.h"

enum class ItemType;
enum class MiscellaneousItemType;

class MiscellaneousArtifactData : public ArtifactData
{
private:
	MiscellaneousItemType miscItemType;
public:
	MiscellaneousArtifactData(const std::string &displayName,
		const std::string &flavorText, const std::vector<ProvinceName> &provinces, 
		const MiscellaneousItemType &miscItemType);
	virtual ~MiscellaneousArtifactData();

	virtual std::unique_ptr<ArtifactData> clone() const override;

	const MiscellaneousItemType &getMiscellaneousItemType() const;

	virtual ItemType getItemType() const override;
};

#endif
