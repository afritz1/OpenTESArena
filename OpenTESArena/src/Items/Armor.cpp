#include <cassert>

#include "Armor.h"
#include "ArmorMaterial.h"
#include "ItemType.h"

Armor::Armor(const ArtifactData *artifactData)
	: Item(artifactData) { }

Armor::~Armor()
{

}

ItemType Armor::getItemType() const
{
	return ItemType::Armor;
}
