#ifndef ACCESSORY_H
#define ACCESSORY_H

#include <memory>
#include <string>

#include "Item.h"
#include "Metallic.h"

// All accessories are metal, unlike trinkets which have no metal.

class AccessoryArtifactData;

enum class AccessoryType;

class Accessory : public Item, public Metallic
{
private:
	AccessoryType accessoryType;
public:
	// Full constructor (intended for clone()).
	Accessory(AccessoryType accessoryType, MetalType metalType,
		const AccessoryArtifactData *artifactData);

	// Accessory constructor for an accessory type and metal type.
	Accessory(AccessoryType accessoryType, MetalType metalType);

	// Accessory constructor for a unique accessory.
	Accessory(const AccessoryArtifactData *artifactData);
	virtual ~Accessory() = default;

	virtual std::unique_ptr<Item> clone() const override;

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	AccessoryType getAccessoryType() const;
	int getMaxEquipCount() const;
};

#endif
