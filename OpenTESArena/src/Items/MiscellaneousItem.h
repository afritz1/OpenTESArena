#ifndef MISCELLANEOUS_ITEM_H
#define MISCELLANEOUS_ITEM_H

#include <memory>
#include <string>

#include "Item.h"

class MiscellaneousArtifactData;

enum class MiscellaneousItemType;

class MiscellaneousItem : public Item
{
private:
	MiscellaneousItemType miscItemType;
public:
	// Full constructor (intended for clone()).
	MiscellaneousItem(MiscellaneousItemType miscItemType,
		const MiscellaneousArtifactData *artifactData);

	// Miscellaneous item constructor for a miscellaneous type.
	MiscellaneousItem(MiscellaneousItemType miscItemType);

	// Miscellaneous item artifact constructor.
	MiscellaneousItem(const MiscellaneousArtifactData *artifactData);
	virtual ~MiscellaneousItem() = default;

	virtual std::unique_ptr<Item> clone() const override;

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	MiscellaneousItemType getMiscellaneousItemType() const;
};

#endif
