#ifndef MISCELLANEOUS_ARTIFACT_DATA_H
#define MISCELLANEOUS_ARTIFACT_DATA_H

#include <string>

#include "ArtifactData.h"
#include "MiscellaneousArtifactName.h"

enum class MiscellaneousItemType;

class MiscellaneousArtifactData : public ArtifactData
{
private:
	MiscellaneousArtifactName artifactName;
public:
	MiscellaneousArtifactData(MiscellaneousArtifactName artifactName);
	virtual ~MiscellaneousArtifactData();

	const MiscellaneousArtifactName &getArtifactName() const;
	MiscellaneousItemType getMiscellaneousItemType() const;

	virtual ArtifactName getParentArtifactName() const override;
	virtual std::string getDisplayName() const override;
	virtual std::string getFlavorText() const override;
};

#endif