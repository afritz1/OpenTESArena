#ifndef MISCELLANEOUS_ITEM_H
#define MISCELLANEOUS_ITEM_H

#include <memory>
#include <string>

#include "Item.h"
#include "MiscellaneousItemType.h"

class MiscellaneousArtifactData;

class MiscellaneousItem : public Item
{
private:
	MiscellaneousItemType miscItemType;

	MiscellaneousItem(MiscellaneousItemType miscItemType,
		const MiscellaneousArtifactData *artifactData);
public:
	// Miscellaneous item constructor for a miscellaneous type.
	MiscellaneousItem(MiscellaneousItemType miscItemType);

	// Miscellaneous item artifact constructor.
	MiscellaneousItem(const MiscellaneousArtifactData *artifactData);
	virtual ~MiscellaneousItem();

	virtual std::unique_ptr<Item> clone() const override;

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	const MiscellaneousItemType &getMiscellaneousItemType() const;
};

#endif
