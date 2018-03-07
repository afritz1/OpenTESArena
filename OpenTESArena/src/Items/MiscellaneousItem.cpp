#include <cassert>
#include <map>

#include "ItemType.h"
#include "MiscellaneousItem.h"
#include "MiscellaneousItemType.h"
#include "MiscellaneousArtifactData.h"

const std::map<MiscellaneousItemType, std::string> MiscellaneousItemDisplayNames =
{
	{ MiscellaneousItemType::Book, "Book" },
	{ MiscellaneousItemType::Key, "Key" },
	{ MiscellaneousItemType::StaffPiece, "Staff Piece" },
	{ MiscellaneousItemType::Unknown, "Unknown" }
};

// These values are made up.
const std::map<MiscellaneousItemType, double> MiscellaneousItemWeights =
{
	{ MiscellaneousItemType::Book, 0.50 },
	{ MiscellaneousItemType::Key, 0.10 },
	{ MiscellaneousItemType::StaffPiece, 0.0 },
	{ MiscellaneousItemType::Unknown, 0.0 }
};

// These values are made up.
const std::map<MiscellaneousItemType, int> MiscellaneousItemGoldValues =
{
	{ MiscellaneousItemType::Book, 0 },
	{ MiscellaneousItemType::Key, 0 },
	{ MiscellaneousItemType::StaffPiece, 0 }, // The value of a staff piece is debatable.
	{ MiscellaneousItemType::Unknown, 0 }
};

MiscellaneousItem::MiscellaneousItem(MiscellaneousItemType miscItemType,
	const MiscellaneousArtifactData *artifactData)
	: Item(artifactData)
{
	this->miscItemType = miscItemType;
}

MiscellaneousItem::MiscellaneousItem(MiscellaneousItemType miscItemType)
	: MiscellaneousItem(miscItemType, nullptr) { }

MiscellaneousItem::MiscellaneousItem(const MiscellaneousArtifactData *artifactData)
	: MiscellaneousItem(artifactData->getMiscellaneousItemType(), artifactData) { }

MiscellaneousItem::~MiscellaneousItem()
{

}

std::unique_ptr<Item> MiscellaneousItem::clone() const
{
	return std::make_unique<MiscellaneousItem>(this->miscItemType,
		dynamic_cast<const MiscellaneousArtifactData*>(this->getArtifactData()));
}

ItemType MiscellaneousItem::getItemType() const
{
	return ItemType::Miscellaneous;
}

double MiscellaneousItem::getWeight() const
{
	auto weight = MiscellaneousItemWeights.at(this->getMiscellaneousItemType());
	assert(weight >= 0.0);
	return weight;
}

int MiscellaneousItem::getGoldValue() const
{
	int baseValue = MiscellaneousItemGoldValues.at(this->getMiscellaneousItemType());
	return baseValue;
}

std::string MiscellaneousItem::getDisplayName() const
{
	auto displayName = MiscellaneousItemDisplayNames.at(this->getMiscellaneousItemType());
	return displayName;
}

MiscellaneousItemType MiscellaneousItem::getMiscellaneousItemType() const
{
	return this->miscItemType;
}
