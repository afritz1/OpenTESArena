#include <cassert>
#include <map>

#include "MiscellaneousItem.h"
#include "ItemType.h"
#include "MiscellaneousItemType.h"
#include "MiscellaneousArtifactData.h"
#include "MiscellaneousArtifactName.h"

const auto MiscellaneousItemDisplayNames = std::map<MiscellaneousItemType, std::string>
{
	{ MiscellaneousItemType::Book, "Book" },
	{ MiscellaneousItemType::BoneKey, "Bone Key" },
	{ MiscellaneousItemType::DiamondKey, "Diamond Key" },
	{ MiscellaneousItemType::GoldKey, "Gold Key" },
	{ MiscellaneousItemType::RubyKey, "Ruby Key" },
	{ MiscellaneousItemType::StaffPiece, "Staff Piece" },
	{ MiscellaneousItemType::Unknown, "Unknown" }
};

// These values are made up.
const auto MiscellaneousItemWeights = std::map<MiscellaneousItemType, double>
{
	{ MiscellaneousItemType::Book, 0.50 },
	{ MiscellaneousItemType::BoneKey, 0.05 },
	{ MiscellaneousItemType::DiamondKey, 0.10 },
	{ MiscellaneousItemType::GoldKey, 0.10 },
	{ MiscellaneousItemType::RubyKey, 0.10 },
	{ MiscellaneousItemType::SteelKey, 0.10 },
	{ MiscellaneousItemType::StaffPiece, 0.0 },
	{ MiscellaneousItemType::Unknown, 0.0 }
};

// These values are made up.
const auto MiscellaneousItemGoldValues = std::map<MiscellaneousItemType, int>
{
	{ MiscellaneousItemType::Book, 0 },
	{ MiscellaneousItemType::BoneKey, 0 },
	{ MiscellaneousItemType::DiamondKey, 0 },
	{ MiscellaneousItemType::GoldKey, 0 },
	{ MiscellaneousItemType::RubyKey, 0 },
	{ MiscellaneousItemType::SteelKey, 0 },
	{ MiscellaneousItemType::StaffPiece, 0 }, // The value of a staff piece is debatable.
	{ MiscellaneousItemType::Unknown, 0 }
};

MiscellaneousItem::MiscellaneousItem(MiscellaneousItemType miscItemType)
{
	this->artifactData = nullptr;
	this->miscItemType = miscItemType;

	assert(this->artifactData.get() == nullptr);
}

MiscellaneousItem::MiscellaneousItem(MiscellaneousArtifactName artifactName)
{
	this->artifactData = std::unique_ptr<MiscellaneousArtifactData>(
		new MiscellaneousArtifactData(artifactName));
	this->miscItemType = this->artifactData->getMiscellaneousItemType();

	assert(this->artifactData.get() != nullptr);
}

MiscellaneousItem::MiscellaneousItem(const MiscellaneousItem &miscItem)
{
	this->artifactData = (miscItem.getArtifactData() == nullptr) ? nullptr :
		std::unique_ptr<MiscellaneousArtifactData>(new MiscellaneousArtifactData(
			miscItem.getArtifactData()->getArtifactName()));
	this->miscItemType = miscItem.getMiscellaneousItemType();

	// This assert makes sure they're logically equivalent.
	assert((!(this->artifactData != nullptr)) ^ (miscItem.artifactData != nullptr));
}

MiscellaneousItem::~MiscellaneousItem()
{

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
	assert(displayName.size() > 0);
	return displayName;
}

const MiscellaneousArtifactData *MiscellaneousItem::getArtifactData() const
{
	return this->artifactData.get();
}

const MiscellaneousItemType &MiscellaneousItem::getMiscellaneousItemType() const
{
	return this->miscItemType;
}
