#ifndef ACCESSORY_H
#define ACCESSORY_H

#include <memory>
#include <string>

#include "AccessoryType.h"
#include "AccessoryArtifactName.h"
#include "Item.h"
#include "Metallic.h"

// All accessories are metal, unlike trinkets which have no metal.

class AccessoryArtifactData;

class Accessory : public Item, public Metallic
{
private:
	std::unique_ptr<AccessoryArtifactData> artifactData;
	AccessoryType accessoryType;
public:
	// Accessory constructor for an accessory type and metal type.
	Accessory(AccessoryType accessoryType, MetalType metalType);

	// Accessory artifact constructor.
	Accessory(AccessoryArtifactName artifactName);

	Accessory(const Accessory &accessory);

	~Accessory();

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	const AccessoryArtifactData *getArtifactData() const;

	const AccessoryType &getAccessoryType() const;
	int getMaxEquipCount() const;
};

#endif
