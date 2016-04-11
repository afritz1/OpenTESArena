#ifndef TRINKET_H
#define TRINKET_H

#include <string>

#include "Item.h"
#include "TrinketName.h"

// Trinkets are non-metal accessories, so they don't inherit from Metallic.

// Should a key be a trinket? They would have a max equip count of 0.

class Trinket : public Item
{
private:
	TrinketName trinketName;
public:
	Trinket(TrinketName trinketName);
	~Trinket();

	virtual ItemType getItemType() const override;
	virtual double getWeight() const override;
	virtual int getGoldValue() const override;
	virtual std::string getDisplayName() const override;

	const TrinketName &getTrinketName() const;
	int getMaxEquipCount() const;
};

#endif
