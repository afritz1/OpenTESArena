#include <cassert>

#include "Item.h"

#include "ArtifactData.h"

Item::Item(const ArtifactData *artifactData)
{
	this->artifactData = (artifactData == nullptr) ? nullptr : artifactData->clone();
}

Item::~Item()
{

}

const ArtifactData *Item::getArtifactData() const
{
	return this->artifactData.get();
}
