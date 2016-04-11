#ifndef CONSUMABLE_POTION_H
#define CONSUMABLE_POTION_H

#include <string>

#include "Consumable.h"

// Potions will be constructed by a given effect (not just spell effect).

class Potion : public Consumable
{
private:

public:
	Potion();
	virtual ~Potion();

	// All potions are the same weight.
	virtual double getWeight() const override;

	// The gold value is based on the effect.
	virtual int getGoldValue() const override;

	virtual std::string getDisplayName() const override;

	virtual ConsumableType getConsumableType() const override;
	virtual std::string typeToString() const override;
};

#endif
