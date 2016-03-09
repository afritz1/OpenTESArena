#ifndef MISCELLANEOUS_ITEM_H
#define MISCELLANEOUS_ITEM_H

#include <memory>
#include <string>

#include "Item.h"
#include "MiscellaneousItemType.h"

class MiscellaneousArtifactData;

enum class MiscellaneousArtifactName;

class MiscellaneousItem : public Item
{
private:
	std::unique_ptr<MiscellaneousArtifactData> artifactData;
	MiscellaneousItemType miscItemType;
public:
	// Miscellaneous item constructor for a miscellaneous type.
	MiscellaneousItem(MiscellaneousItemType miscItemType);

	// Miscellaneous item artifact constructor.
	MiscellaneousItem(MiscellaneousArtifactName artifactName);

	MiscellaneousItem(const MiscellaneousItem &miscItem);

	virtual ~MiscellaneousItem();

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	const MiscellaneousArtifactData *getArtifactData() const;

	const MiscellaneousItemType &getMiscellaneousItemType() const;
};

#endif